#include <Util/Statistics/NesStatistics.hpp>
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
    }
}


void NesStatistics::shutdown(){
    if(!running){
        return;
    }

    running = false;
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
