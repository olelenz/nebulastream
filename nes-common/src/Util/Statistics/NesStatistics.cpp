#include <Util/Statistics/NesStatistics.hpp>
#include <chrono>
#include <fstream>
#include <iostream>
#include <mutex>

// TODO: how do we delete stuff from the file?

namespace NES {

NesStatistics::NesStatistics() : running(false){}
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
        ringBuffer.blockingWrite(nullptr);
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
    if(workerType == StatisticsWorkerType::RingBuffer){
        ringBuffer.blockingWrite(std::move(event));
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
        std::unique_ptr<NesStatisticsEvents> event;
        ringBuffer.blockingRead(event);
        if (!event) {
            break;
        }
        rollingStore.wlock()->push(std::move(event));
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
