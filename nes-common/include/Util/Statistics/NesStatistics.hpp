#pragma once
#include <string>
#include <iostream>
#include <mutex>

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
        NesStatistics(){}

        std::mutex statsMutex;
};
