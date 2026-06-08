#include <Util/Statistics/NesStatistics.hpp>
#include <fstream>

// TODO: how do we delete stuff from the file?

namespace NES {

NesStatistics::NesStatistics() {
    workThread = std::thread(&NesStatistics::workStatsQueue, this);
}
NesStatistics::~NesStatistics() {
    running = false;
    condVar.notify_one();
    if (workThread.joinable()) {
        workThread.join();
    }
}

void NesStatistics::nesStats(std::unique_ptr<NesStatisticsEvents> event){
    std::cout << "NES-STAT: " << event->toCSV() << std::endl;
    {
        std::lock_guard<std::mutex> lock(statsMutex);
        statsQueue.push(std::move(event));
    }
    condVar.notify_one();
}

void NesStatistics::nesStatsSlow(std::unique_ptr<NesStatisticsEvents> event){
    std::cout << "NES-STAT: " << event->toCSV() << std::endl;
    std::lock_guard<std::mutex> lock(this->statsMutex);
    std::ofstream outFile("stats-test.csv", std::ios::app);
    if (!outFile.is_open()) {
        std::cout << "Could not open file \n";
        return;
    }
    outFile << event->toCSV() << "\n";
    outFile.close();
}

void NesStatistics::workStatsQueue(){
    std::ofstream outFile("stats-test.csv", std::ios::app);
    if (!outFile.is_open()) {
        std::cout << "Could not open file \n";
        return;
    }

    while(true){
        std::string msg;
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
        outFile << currentEvent->toCSV() << "\n";  // TODO: write in blocks
    }
    outFile.close();

}

}
