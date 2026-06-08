#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <gtest/gtest.h>

#include <Util/Statistics/NesStatistics.hpp>
#include <Util/Statistics/NesStatisticsEvents.hpp>

namespace NES
{
void workerTask(int threadId, int iterations) {
    for (int i = 0; i < iterations; ++i) {
        NES::logStat<NES::NesBufferAllocateEvent>(threadId * 10000 + i, 4096);
    }
}
void workerTaskSlow(int threadId, int iterations) {
    for (int i = 0; i < iterations; ++i) {
        NES::logStatSlow<NES::NesBufferAllocateEvent>(threadId * 10000 + i, 4096);
    }
}

TEST(Simple, Slow){
    const int NUM_THREADS = 10;
    const int EVENTS_PER_THREAD = 5000;
    const int TOTAL_EVENTS = NUM_THREADS * EVENTS_PER_THREAD;

    std::cout << "Spawning " << NUM_THREADS << " threads...\n";
    std::cout << "Each thread firing " << EVENTS_PER_THREAD << " events.\n";
    std::cout << "Total Events to Write: " << TOTAL_EVENTS << "\n\n";

    std::vector<std::thread> threads;

    auto startTime = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back(workerTaskSlow, i, EVENTS_PER_THREAD);
    }

    for (auto& t : threads) {
        t.join();
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> totalTime = endTime - startTime;

    std::cout << "RESULTS (" << TOTAL_EVENTS << " Events)\n";
    std::cout << "Time: " << totalTime.count() << " ms\n";

}

TEST(Simple, Fast){
    const int NUM_THREADS = 10;
    const int EVENTS_PER_THREAD = 5000;
    const int TOTAL_EVENTS = NUM_THREADS * EVENTS_PER_THREAD;

    std::cout << "Starting dual-timer benchmark...\n\n";

    std::vector<std::thread> threads;

    auto startTime = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back(workerTask, i, EVENTS_PER_THREAD);
    }
    for (auto& t : threads) {
        t.join();
    }

    NesStatistics::getInstance().shutdown();

    auto endTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> totalTime = endTime - startTime;


    std::cout << "RESULTS (" << TOTAL_EVENTS << " Events)\n";
    std::cout << "Time: " << totalTime.count() << " ms\n";
}

}
