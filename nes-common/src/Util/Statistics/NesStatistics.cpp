#include <Util/Statistics/NesStatistics.hpp>
#include <fstream>


// TODO: how do we delete stuff from the file?

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

void NesStatistics::nesStats(const std::string& msg){
    std::cout << "NES-STAT: " << msg << std::endl;

    {
        std::lock_guard<std::mutex> lock(statsMutex);
        statsQueue.push(msg);
    }
}

void NesStatistics::workStatsQueue(){
    std::ofstream outFile("stats-test.csv", std::ios::app);
    if (!outFile.is_open()) {
        std::cout << "Could not open file \n";
        return;
    }

    while(true){
        std::string msg;
        {
            std::unique_lock<std::mutex> lock(statsMutex);
            condVar.wait(lock, [this](){
                return !statsQueue.empty() || !running;
            });
            if(statsQueue.empty() && !running){
                break;
            }
            msg = statsQueue.front();
            statsQueue.pop();
        }
        outFile << msg << "\n";
    }
    outFile.close();

}
