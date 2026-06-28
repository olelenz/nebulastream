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
    running = true;
    workerType = type;
    if(type != StatisticsWorkerType::RingBuffer){
        outFile.open(filePath, std::ios::app);
        if(!outFile.is_open()){
            std::cout << "Could not open file \n";
            return;
        }
    }
    if(type == StatisticsWorkerType::Buffered){
        workThread = std::thread(&NesStatistics::workStatsQueue, this);
    } else if(type == StatisticsWorkerType::Chunked){
        workThread = std::thread(&NesStatistics::workStatsQueueChunked, this);
    } else if(type == StatisticsWorkerType::RingBuffer)
    {
        workThread = std::thread(&NesStatistics::workStatsRingBuffer, this);
    }
}


void NesStatistics::shutdown(){
    if(!running){
        return;
    }

    if (workerType == StatisticsWorkerType::RingBuffer){
        std::cout << "\n In Memory Stats \n" << getStats() << "\n";
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
    if(!running){
        std::lock_guard<std::mutex> lock(statsMutex);
        if(!running){
            start(StatisticsWorkerType::RingBuffer, "nes-stats-default-csv");
            //start(StatisticsWorkerType::Chunked, "nes-stats-default-csv");
        }
    }

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

void NesStatistics::nesStatsSlow(std::unique_ptr<NesStatisticsEvents> event){
    std::lock_guard<std::mutex> lock(this->statsMutex);
    std::ofstream outFile("stats-test-slow.csv", std::ios::app);
    if (!outFile.is_open()) {
        std::cout << "Could not open file \n";
        return;
    }
    outFile << event->toCSV() << std::endl;
    outFile.flush();
    outFile.close();
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
void NesStatistics::workStatsQueueChunked() {
    const int CHUNK_SIZE = 1000;
    std::vector<std::unique_ptr<NesStatisticsEvents>> batch;
    batch.reserve(CHUNK_SIZE);

    while (running || !statsQueue.empty()) {
        {
            std::unique_lock<std::mutex> lock(statsMutex);
            condVar.wait(lock, [this]() {
                return !statsQueue.empty() || !running;
            });

            int count = 0;
            while (!statsQueue.empty() && count < CHUNK_SIZE) {
                batch.push_back(std::move(statsQueue.front()));
                statsQueue.pop();
                count++;
            }
        }

        if(!batch.empty()){
            for (const auto& event : batch) {
                outFile << event->toCSV() << "\n";
            }
            outFile.flush();
            batch.clear();
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
    rollingStore.rlock()->forEach([&oss](const std::unique_ptr<NesStatisticsEvents>& event) {
        if (event) {
            oss << event->toCSV() << '\n';
        }
    });
    return oss.str();
}

}
