#include <Util/Statistics/NesStatistics.hpp>
#include <cassert>
#include <chrono>
#include <fstream>
#include <iostream>
#include <mutex>

// TODO: how do we delete stuff from the file?

namespace NES {

NesStatistics::NesStatistics() : running(false) {
#if defined(NES_STATISTICS_ENABLED)
    ringBuffers.reserve(NUM_EVENT_TYPES);
    for (std::size_t i = 0; i < NUM_EVENT_TYPES; ++i) {
        ringBuffers.emplace_back(RING_BUFFER_CAPACITY);
    }
    start(StatisticsWorkerType::RingBuffer);
#endif
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
        ringBuffers[idx].blockingWrite(std::move(event));
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
    if(workerType == StatisticsWorkerType::RingBuffer){
        ringBuffers[queueIdx].blockingWrite(std::move(event));
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

void NesStatistics::workStatsRingBuffer() {
    while (true) {
        ringBufferSem.acquire();

        for (auto& q : ringBuffers) {
            std::unique_ptr<NesStatisticsEvents> event;
            while (q.read(event)) {
                if (event) {
                    rollingStore.wlock()->push(std::move(event));
                }
            }
        }

        // drain the queues on shutdown
        if (!running) {
            bool any = true;
            while (any) {
                any = false;
                for (auto& q : ringBuffers) {
                    std::unique_ptr<NesStatisticsEvents> event;
                    while (q.read(event)) {
                        any = true;
                        if (event) {
                            rollingStore.wlock()->push(std::move(event));
                        }
                    }
                }
            }
            break;
        }
    }
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

}
