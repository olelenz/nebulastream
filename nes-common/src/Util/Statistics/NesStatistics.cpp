#include <Util/Statistics/NesStatistics.hpp>
#include <cassert>
#include <chrono>
#include <fstream>
#include <iostream>
#include <mutex>
#include <stdexcept>

// TODO: how do we delete stuff from the file?

namespace NES {

#if defined(NES_STATISTICS_ENABLED)
NesStatistics::NesStatistics() : running(false) {
    ringBuffers.reserve(NUM_EVENT_TYPES);
    for (std::size_t i = 0; i < NUM_EVENT_TYPES; ++i) {
        ringBuffers.push_back(std::make_unique<folly::MPMCQueue<std::unique_ptr<NesStatisticsEvents>>>(RING_BUFFER_CAPACITY));
    }
    start(StatisticsWorkerType::RingBuffer);
}
NesStatistics::~NesStatistics() {
    shutdown();
}
void NesStatistics::start(StatisticsWorkerType type, const std::string& filePath){
    if(running){
        return;
    }

    // clear previous stores
    rollingStore.wlock()->clear();

    running = true;
    workerType = type;
    if(type == StatisticsWorkerType::Buffered){
        outFile.open(filePath, std::ios::app);
        if(!outFile.is_open()){
            std::cout << "Could not open file \n";
            running = false;
            return;
        }
        workThread = std::thread(&NesStatistics::workStatsQueue, this);
    } else if(type == StatisticsWorkerType::RingBuffer) {
        workThread = std::thread(&NesStatistics::workStatsRingBuffer, this);
        startResourceSampler();
    }
}


void NesStatistics::shutdown(){
    if(!running){
        return;
    }

    running = false;
    stopResourceSampler();
    if(workerType == StatisticsWorkerType::RingBuffer){
        ringBufferSem.release();
    } else {
        condVar.notify_one();
    }
    if (workThread.joinable()) {
        workThread.join();
    }
    if(outFile.is_open()){
        outFile.close();
    }
}

void NesStatistics::nesStats(std::unique_ptr<NesStatisticsEvents> event){
    if (!running.load()) {
        return;
    }
    if(workerType == StatisticsWorkerType::RingBuffer){
        const std::size_t idx = static_cast<std::size_t>(event->getTypeIndex());
        if (idx >= ringBuffers.size()) {
            return;
        }
        ringBuffers[idx]->blockingWrite(std::move(event));
        ringBufferSem.release();
        return;
    }

    bool wakeUpThread = false;
    {
        std::lock_guard<std::mutex> lock(statsMutex);
        wakeUpThread = statsQueue.empty();
        statsQueue.push(std::move(event));
    }
    if (wakeUpThread) {
        condVar.notify_one();
    }
}

void NesStatistics::nesStatsDirect(std::size_t queueIdx, std::unique_ptr<NesStatisticsEvents> event){
    if (!running.load()) {
        return;
    }
    assert(queueIdx < ringBuffers.size() && "queueIdx out of range");
    if (queueIdx >= ringBuffers.size()) {
        return;
    }
    if(workerType == StatisticsWorkerType::RingBuffer){
        ringBuffers[queueIdx]->blockingWrite(std::move(event));
        ringBufferSem.release();
        return;
    }

    bool wakeUpThread = false;
    {
        std::lock_guard<std::mutex> lock(statsMutex);
        wakeUpThread = statsQueue.empty();
        statsQueue.push(std::move(event));
    }
    if (wakeUpThread) {
        condVar.notify_one();
    }
}


void NesStatistics::recordQueryResourceStart(QueryId queryId, QueryResourceSnapshot snapshot)
{
    std::lock_guard<std::mutex> lock(queryResourceSnapshotsMutex);
    queryResourceSnapshots.insert_or_assign(queryId, snapshot);
}

std::optional<QueryResourceSnapshot> NesStatistics::consumeQueryResourceStart(QueryId queryId)
{
    std::lock_guard<std::mutex> lock(queryResourceSnapshotsMutex);
    const auto it = queryResourceSnapshots.find(queryId);
    if (it == queryResourceSnapshots.end())
    {
        return std::nullopt;
    }
    auto snapshot = it->second;
    queryResourceSnapshots.erase(it);
    return snapshot;
}

void NesStatistics::setActiveQueryCountProvider(std::function<uint64_t()> provider)
{
    std::lock_guard<std::mutex> lock(activeQueryCountProviderMutex);
    activeQueryCountProvider = std::move(provider);
}

void NesStatistics::setWorkerBufferUsageProvider(std::function<WorkerBufferUsageSnapshot()> provider)
{
    std::lock_guard<std::mutex> lock(workerBufferUsageProviderMutex);
    workerBufferUsageProvider = std::move(provider);
}

void NesStatistics::startResourceSampler()
{
    if (resourceSamplerThread.joinable())
    {
        return;
    }
    resourceSamplerThread = std::thread(&NesStatistics::sampleResourceUsagePeriodically, this);
}

void NesStatistics::stopResourceSampler()
{
    resourceSamplerCondVar.notify_one();
    if (resourceSamplerThread.joinable())
    {
        resourceSamplerThread.join();
    }
}

void NesStatistics::sampleResourceUsagePeriodically()
{
    std::unique_lock lock(resourceSamplerMutex);
    while (running.load())
    {
        if (resourceSamplerCondVar.wait_for(lock, DEFAULT_RESOURCE_SAMPLE_INTERVAL, [this] { return !running.load(); }))
        {
            break;
        }

        lock.unlock();
        const auto timestamp = std::chrono::system_clock::now();
        if (const auto resourceSnapshot = collectProcessResourceSnapshot(timestamp))
        {
            nesStats(std::make_unique<NesWorkerCpuTimeEvent>(INVALID_QUERY_ID, resourceSnapshot->cpuTimeMicros));
            nesStats(std::make_unique<NesWorkerMemoryUsageEvent>(INVALID_QUERY_ID, resourceSnapshot->residentMemoryKb));
        }
        std::function<uint64_t()> activeQueryCountProviderCopy;
        {
            std::lock_guard<std::mutex> providerLock(activeQueryCountProviderMutex);
            activeQueryCountProviderCopy = activeQueryCountProvider;
        }
        if (activeQueryCountProviderCopy)
        {
            nesStats(std::make_unique<NesWorkerActiveQueryCountEvent>(INVALID_QUERY_ID, activeQueryCountProviderCopy()));
        }
        std::function<WorkerBufferUsageSnapshot()> workerBufferUsageProviderCopy;
        {
            std::lock_guard<std::mutex> providerLock(workerBufferUsageProviderMutex);
            workerBufferUsageProviderCopy = workerBufferUsageProvider;
        }
        if (workerBufferUsageProviderCopy)
        {
            const auto bufferUsage = workerBufferUsageProviderCopy();
            const auto usedCount = bufferUsage.totalCount >= bufferUsage.availableCount ? bufferUsage.totalCount - bufferUsage.availableCount : 0;
            nesStats(std::make_unique<NesWorkerBufferTotalCountEvent>(INVALID_QUERY_ID, bufferUsage.totalCount));
            nesStats(std::make_unique<NesWorkerBufferAvailableCountEvent>(INVALID_QUERY_ID, bufferUsage.availableCount));
            nesStats(std::make_unique<NesWorkerBufferUsedCountEvent>(INVALID_QUERY_ID, usedCount));
            nesStats(std::make_unique<NesWorkerBufferUsedBytesEvent>(INVALID_QUERY_ID, usedCount * bufferUsage.bufferSize));
        }
        lock.lock();
    }
}

void NesStatistics::workStatsQueue(){
    while(running || !statsQueue.empty()){
        std::unique_ptr<NesStatisticsEvents> currentEvent;
        {
            std::unique_lock<std::mutex> lock(statsMutex);
            condVar.wait(lock, [this](){
                return !statsQueue.empty() || !running;
            });
            if(statsQueue.empty() && !running){
                break;
            }
            currentEvent = std::move(statsQueue.front());
            statsQueue.pop();
        }
        if(currentEvent){
            outFile << currentEvent->toCSV() << "\n";
        }
    }
    outFile.close();

}

#if defined(NES_COLLECT_STATISTICS_ENABLED)
void NesStatistics::startCsvCollection(const std::string& filePath)
{
    if (filePath.empty())
    {
        throw std::invalid_argument("Statistics CSV output path must not be empty");
    }
    std::lock_guard lock(csvMutex);
    if (csvFile.is_open())
    {
        throw std::logic_error("Statistics CSV collection is already active");
    }
    csvFile.clear();
    csvFile.open(filePath, std::ios::out | std::ios::trunc);
    if (!csvFile.is_open())
    {
        throw std::runtime_error("Could not open statistics CSV file: " + filePath);
    }
    csvFile << "seq,ts,queryId,metricValue,eventType\n";
}

void NesStatistics::stopCsvCollection()
{
    std::lock_guard lock(csvMutex);
    if (!csvFile.is_open())
    {
        return;
    }
    csvFile.flush();
    csvFile.close();
}
#endif

void NesStatistics::workStatsRingBuffer() {
#if defined(NES_COLLECT_STATISTICS_ENABLED)
    const auto consumeEvent = [this](std::unique_ptr<NesStatisticsEvents> event) {
        if (!event) {
            return;
        }

        bool collectCsv;
        {
            std::lock_guard lock(csvMutex);
            collectCsv = csvFile.is_open();
        }
        if (!collectCsv)
        {
            rollingStore.wlock()->push(std::move(event));
            return;
        }

        const auto timestamp = event->getTimestamp();
        const auto queryId = event->getQueryId();
        const auto metricValue = event->getMetricValue();
        const auto eventType = event->getEventType();
        uint64_t sequenceNumber;
        {
            auto store = rollingStore.wlock();
            sequenceNumber = store->nextSeq;
            store->push(std::move(event));
        }

        std::lock_guard lock(csvMutex);
        if (csvFile.is_open()) {
            csvFile << sequenceNumber << ',' << timestamp << ',' << queryId << ',' << metricValue << ',' << eventType << '\n';
        }
    };
#endif

    while (true) {
        ringBufferSem.acquire();

        for (auto& q : ringBuffers) {
            std::unique_ptr<NesStatisticsEvents> event;
            while (q->read(event)) {
#if defined(NES_COLLECT_STATISTICS_ENABLED)
                consumeEvent(std::move(event));
#else
                if (event) {
                    rollingStore.wlock()->push(std::move(event));
                }
#endif
            }
        }

        // drain the queues on shutdown
        if (!running) {
            bool any = true;
            while (any) {
                any = false;
                for (auto& q : ringBuffers) {
                    std::unique_ptr<NesStatisticsEvents> event;
                    while (q->read(event)) {
                        any = true;
#if defined(NES_COLLECT_STATISTICS_ENABLED)
                        consumeEvent(std::move(event));
#else
                        if (event) {
                            rollingStore.wlock()->push(std::move(event));
                        }
#endif
                    }
                }
            }
            break;
        }
    }
#if defined(NES_COLLECT_STATISTICS_ENABLED)
    stopCsvCollection();
#endif
}

std::string NesStatistics::getStats() const {
    std::ostringstream oss;
    rollingStore.rlock()->forEach([&oss](const uint64_t, const std::unique_ptr<NesStatisticsEvents>& event) {
        if (event) {
            oss << event->toCSV() << '\n';
        }
    });
    return oss.str();
}

std::vector<RawEventData> NesStatistics::getEventsSince(uint64_t sequenceNumber) const
{
    std::vector<RawEventData> events;
    rollingStore.rlock()->forEach([&events, sequenceNumber](const uint64_t seq, const std::unique_ptr<NesStatisticsEvents>& event)
    {
        if (event && seq >= sequenceNumber)
        {
            events.push_back({seq, event->getTimestamp(), event->getQueryId(), event->getMetricValue(), event->getEventType()});
        }
    });
    return events;
}
#else
NesStatistics::NesStatistics() {}
NesStatistics::~NesStatistics() {}
void NesStatistics::start(StatisticsWorkerType, const std::string&) {}
void NesStatistics::shutdown() {}
void NesStatistics::nesStats(std::unique_ptr<NesStatisticsEvents>) {}
void NesStatistics::nesStatsDirect(std::size_t, std::unique_ptr<NesStatisticsEvents>) {}
void NesStatistics::recordQueryResourceStart(QueryId, QueryResourceSnapshot) {}
std::optional<QueryResourceSnapshot> NesStatistics::consumeQueryResourceStart(QueryId) { return std::nullopt; }
void NesStatistics::setActiveQueryCountProvider(std::function<uint64_t()>) {}
void NesStatistics::setWorkerBufferUsageProvider(std::function<WorkerBufferUsageSnapshot()>) {}
void NesStatistics::startResourceSampler() {}
void NesStatistics::stopResourceSampler() {}
void NesStatistics::sampleResourceUsagePeriodically() {}
void NesStatistics::workStatsQueue() {}
void NesStatistics::workStatsRingBuffer() {}
std::string NesStatistics::getStats() const { return ""; }
std::vector<RawEventData> NesStatistics::getEventsSince(uint64_t) const { return {}; }
#endif

}
