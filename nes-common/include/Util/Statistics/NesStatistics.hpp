#pragma once
#include <string>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>
#include <condition_variable>
#include <atomic>

// TODO: we should make this fast

class NesStatistics{
    public:
        static NesStatistics& getInstance(){
            static NesStatistics instance;
            return instance;
        }

        void nesStats(const std::string& msg);

        // make this a singleton
        NesStatistics(NesStatistics const&) = delete;
        void operator=(NesStatistics const&) = delete;

    private:
        NesStatistics();
        ~NesStatistics();

        void workStatsQueue();
        std::queue<std::string> statsQueue;
        std::condition_variable condVar;
        std::atomic<bool> running{true};
        std::thread workThread;
        std::mutex statsMutex;
};
