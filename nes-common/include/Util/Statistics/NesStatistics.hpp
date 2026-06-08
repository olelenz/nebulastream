#pragma once
#include <atomic>
#include <condition_variable>
#include <fstream>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include "NesStatisticsEvents.hpp"

// TODO: we should make this fast

namespace NES {

enum class StatisticsWorkerType {
    Buffered,
    Chunked
};

class NesStatistics{
    public:
        static NesStatistics& getInstance(){
            static NesStatistics instance;
            return instance;
        }

        void start(StatisticsWorkerType type, const std::string& filePath);
        void nesStats(std::unique_ptr<NesStatisticsEvents> event);
        void nesStatsSlow(std::unique_ptr<NesStatisticsEvents> event);
        void shutdown();

        // make this a singleton
        NesStatistics(NesStatistics const&) = delete;
        void operator=(NesStatistics const&) = delete;

    private:
        NesStatistics();
        ~NesStatistics();

        void workStatsQueue();
        void workStatsQueueChunked();
        std::queue<std::unique_ptr<NesStatisticsEvents>> statsQueue;
        std::condition_variable condVar;
        std::atomic<bool> running{true};
        std::thread workThread;
        std::mutex statsMutex;
        std::ofstream outFile;
};

template<typename EventType, typename... Args>
    void logStat(Args&&... args) {
    NesStatistics::getInstance().nesStats(
        std::make_unique<EventType>(std::forward<Args>(args)...)
    );
}
template<typename EventType, typename... Args>
    void logStatSlow(Args&&... args) {
    NesStatistics::getInstance().nesStatsSlow(
        std::make_unique<EventType>(std::forward<Args>(args)...)
    );
}

}
