#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <gtest/gtest.h>

#include <Util/Statistics/NesStatistics.hpp>
#include <Util/Statistics/NesStatisticsEvents.hpp>

namespace NES
{

const int NUM_THREADS = 10;

void workerTaskSlow(int threadId, int eventsPerThread) {
    for (int i = 0; i < eventsPerThread; ++i) {
        NES::logStatSlow<NES::NesBufferAllocateEvent>(threadId * 10000 + i, 4096);
    }
}

void workerTaskAsync(int threadId, int eventsPerThread) {
    for (int i = 0; i < eventsPerThread; ++i) {
        NES::logStat<NES::NesBufferAllocateEvent>(threadId * 10000 + i, 4096);
    }
}

void runBenchmark(const std::string& name, int totalEvents, std::function<void(int, int)> task, bool useWorker, NES::StatisticsWorkerType workerType = NES::StatisticsWorkerType::Buffered) {
    if (totalEvents % NUM_THREADS != 0) {
        return;
    }
    const int eventsPerThread = totalEvents / NUM_THREADS;

    if (useWorker) {
        std::string fileName = "stats-test-" + name + ".csv";
        NES::NesStatistics::getInstance().start(workerType, fileName);
    }

    std::vector<std::thread> threads;
    auto startTime = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back(task, i, eventsPerThread);
    }
    for (auto& t : threads) {
        t.join();
    }

    if (useWorker) {
        NES::NesStatistics::getInstance().shutdown();
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration = endTime - startTime;

    std::cout << name << "," << totalEvents << "," << duration.count() << std::endl;
}


TEST(Bench, One){
    std::vector<int> eventCounts = {10, 100, 1000, 10000, 50000, 100000, 250000, 500000};
    std::ofstream("stats-test-Slow.csv", std::ofstream::trunc).close();
    std::ofstream("stats-test-Buffered.csv", std::ofstream::trunc).close();
    std::ofstream("stats-test-Chunked.csv", std::ofstream::trunc).close();

    std::cout << "Strategy,EventCount,TimeMS" << std::endl;
    for (int count : eventCounts) {
        runBenchmark("Slow", count, workerTaskSlow, false);
        runBenchmark("Buffered", count, workerTaskAsync, true, NES::StatisticsWorkerType::Buffered);
        runBenchmark("Chunked", count, workerTaskAsync, true, NES::StatisticsWorkerType::Chunked);
    }

    std::cout << "\nComplete. " << std::endl;
}

}
