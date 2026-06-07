#pragma once
#include <atomic>
#include <condition_variable>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include "NesStatisticsEvents.hpp"

// TODO: we should make this fast

class NesStatistics{
    public:
        static NesStatistics& getInstance(){
            static NesStatistics instance;
            return instance;
        }

        void nesStats(std::unique_ptr<NesStatisticsEvents> event);

        // make this a singleton
        NesStatistics(NesStatistics const&) = delete;
        void operator=(NesStatistics const&) = delete;

    private:
        NesStatistics();
        ~NesStatistics();

        void workStatsQueue();
        std::queue<std::unique_ptr<NesStatisticsEvents>> statsQueue;
        std::condition_variable condVar;
        std::atomic<bool> running{true};
        std::thread workThread;
        std::mutex statsMutex;
};
