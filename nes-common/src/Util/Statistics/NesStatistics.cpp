#include <Util/Statistics/NesStatistics.hpp>
#include <fstream>

// TODO: how do we delete stuff from the file?

void NesStatistics::nesStats(const std::string& msg){
    std::cout << "NES-STAT: " << msg << std::endl;
    std::lock_guard<std::mutex> lock(this->statsMutex);
    std::ofstream outFile("stats-test.csv", std::ios::app);
    if (!outFile.is_open()) {
        std::cout << "Could not open file \n";
        return;
    }
    outFile << msg << "\n";
    outFile.close();
}
