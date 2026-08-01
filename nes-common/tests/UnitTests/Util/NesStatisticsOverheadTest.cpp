#include <gtest/gtest.h>
#include <Util/Statistics/NesStatistics.hpp>

#include <algorithm>
#include <barrier>
#include <chrono>
#include <cmath>
#include <iostream>
#include <memory>
#include <numeric>
#include <thread>
#include <vector>
#include <fstream>

namespace NES {

// minimal event type for this test
class MockOverheadEvent : public NesStatisticsEvents {
public:
    explicit MockOverheadEvent(uint64_t value) : value(value) {}

    std::string toString() const override { return "MockOverheadEvent"; }
    std::string getEventType() const override { return "MockOverheadEvent"; }
    std::string getQueryId() const override { return "0"; }
    uint64_t getMetricValue() const override { return value; }

private:
    uint64_t value;
};

// lateny results
struct LatencyResult {
    double averageInsertionNs = 0.0;
    double maximumInsertionNs = 0.0;
    double slowInsertionPercent = 0.0;
};

class NesStatisticsOverheadTest : public ::testing::Test {
protected:
    // calc mean and std
    static std::pair<double, double> calculateStats(const std::vector<double>& values) {
        const double sum = std::accumulate(values.begin(), values.end(), 0.0);
        const double mean = sum / static_cast<double>(values.size());

        double squaredDifferenceSum = 0.0;
        for (const double value : values) {
            const double difference = value - mean;
            squaredDifferenceSum += difference * difference;
        }

        const double standardDeviation = values.size() > 1
            ? std::sqrt(squaredDifferenceSum / static_cast<double>(values.size() - 1))
            : 0.0;

        return {mean, standardDeviation};
    }

    static void startCollector() {
        NesStatistics::getInstance().start(StatisticsWorkerType::RingBuffer);
    }

    static void stopCollector() {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        NesStatistics::getInstance().shutdown();
    }

    // how many events are submitted per second
    static double runThroughputPhase(size_t numberOfThreads, uint64_t totalEvents) {
        const uint64_t eventsPerThread = totalEvents / numberOfThreads;

        startCollector();

        std::barrier startBarrier(static_cast<std::ptrdiff_t>(numberOfThreads + 1));
        std::vector<std::thread> threads;
        threads.reserve(numberOfThreads);

        for (size_t threadIndex = 0; threadIndex < numberOfThreads; threadIndex++) {
            threads.emplace_back([&, threadIndex]() {
                startBarrier.arrive_and_wait();

                for (uint64_t eventIndex = 0; eventIndex < eventsPerThread; eventIndex++) {
                    auto event = std::make_unique<MockOverheadEvent>(eventIndex + threadIndex * eventsPerThread);
                    NesStatistics::getInstance().nesStatsDirect(0, std::move(event));
                }
            });
        }

        const auto start = std::chrono::steady_clock::now();
        // release all threads
        startBarrier.arrive_and_wait();

        for (auto& thread : threads) {
            thread.join();
        }

        const auto end = std::chrono::steady_clock::now();
        stopCollector();

        const double durationSeconds = std::chrono::duration<double>(end - start).count();
        return static_cast<double>(totalEvents) / durationSeconds;
    }

    // measure latency
    static LatencyResult runLatencyPhase(size_t numberOfThreads, uint64_t totalEvents) {
        const uint64_t eventsPerThread = totalEvents / numberOfThreads;

        startCollector();

        std::barrier startBarrier(static_cast<std::ptrdiff_t>(numberOfThreads + 1));
        std::vector<std::thread> threads;
        threads.reserve(numberOfThreads);

        std::vector<uint64_t> totalTimes(numberOfThreads, 0);
        std::vector<uint64_t> maximumTimes(numberOfThreads, 0);
        std::vector<uint64_t> slowCounts(numberOfThreads, 0);

        for (size_t threadIndex = 0; threadIndex < numberOfThreads; threadIndex++) {
            threads.emplace_back([&, threadIndex]() {
                uint64_t localTotal = 0;
                uint64_t localMaximum = 0;
                uint64_t localSlowCount = 0;

                startBarrier.arrive_and_wait();

                for (uint64_t eventIndex = 0; eventIndex < eventsPerThread; eventIndex++) {
                    auto event = std::make_unique<MockOverheadEvent>(eventIndex + threadIndex * eventsPerThread);

                    const auto start = std::chrono::steady_clock::now();
                    NesStatistics::getInstance().nesStatsDirect(0, std::move(event));
                    const auto end = std::chrono::steady_clock::now();

                    const auto durationNs = static_cast<uint64_t>(
                        std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count());

                    localTotal += durationNs;
                    localMaximum = std::max(localMaximum, durationNs);

                    if (durationNs > 10'000) {  // count insertions slower than 10 microseconds
                        localSlowCount++;
                    }
                }

                totalTimes[threadIndex] = localTotal;
                maximumTimes[threadIndex] = localMaximum;
                slowCounts[threadIndex] = localSlowCount;
            });
        }

        startBarrier.arrive_and_wait();

        for (auto& thread : threads) {
            thread.join();
        }

        stopCollector();

        // calc results
        const uint64_t totalInsertionTime = std::accumulate(totalTimes.begin(), totalTimes.end(), uint64_t{0});
        const uint64_t maximumInsertionTime = *std::max_element(maximumTimes.begin(), maximumTimes.end());
        const uint64_t slowInsertionCount = std::accumulate(slowCounts.begin(), slowCounts.end(), uint64_t{0});

        LatencyResult result;
        result.averageInsertionNs = static_cast<double>(totalInsertionTime) / static_cast<double>(totalEvents);
        result.maximumInsertionNs = static_cast<double>(maximumInsertionTime);
        result.slowInsertionPercent = static_cast<double>(slowInsertionCount) * 100.0 / static_cast<double>(totalEvents);
        return result;
    }

    // run throughput / latency
    static void runConfiguration(size_t numberOfThreads, uint64_t totalEvents, size_t numberOfRuns, const std::string& outputFile) {
        ASSERT_GT(numberOfThreads, 0);
        ASSERT_GT(numberOfRuns, 1);
        ASSERT_EQ(totalEvents % numberOfThreads, 0);

        // warm-up
        runThroughputPhase(numberOfThreads, totalEvents / 10);

        std::vector<double> throughputs;
        std::vector<double> averageTimes;
        std::vector<double> maximumTimes;
        std::vector<double> slowPercentages;

        for (size_t run = 0; run < numberOfRuns; run++) {
            const double throughput = runThroughputPhase(numberOfThreads, totalEvents);
            const auto latency = runLatencyPhase(numberOfThreads, totalEvents);

            throughputs.push_back(throughput);
            averageTimes.push_back(latency.averageInsertionNs);
            maximumTimes.push_back(latency.maximumInsertionNs);
            slowPercentages.push_back(latency.slowInsertionPercent);

            std::ofstream out(outputFile, std::ios::app);
            ASSERT_TRUE(out.is_open());

            out << numberOfThreads << "," << run + 1 << "," << throughput << "," << latency.averageInsertionNs << "," << latency.maximumInsertionNs << "," << latency.slowInsertionPercent << "\n";
        }

        const auto throughputStats = calculateStats(throughputs);
        const auto averageStats = calculateStats(averageTimes);
        const auto maximumStats = calculateStats(maximumTimes);
        const auto slowStats = calculateStats(slowPercentages);

        std::cout << "[ " << numberOfThreads << " Thread(s), " << numberOfRuns << " Runs ]\n";
        std::cout << "  Throughput:       " << throughputStats.first << " +/- " << throughputStats.second << " events/s\n";
        std::cout << "  Avg insertion:    " << averageStats.first << " +/- " << averageStats.second << " ns\n";
        std::cout << "  Max insertion:    " << maximumStats.first << " +/- " << maximumStats.second << " ns\n";
        std::cout << "  Insertions >10us: " << slowStats.first << "% +/- " << slowStats.second << "%\n";
        std::cout << "---\n";

    }
};

TEST_F(NesStatisticsOverheadTest, StatisticsSubmissionScaling) {
#if defined(NES_STATISTICS_ENABLED)
    constexpr uint64_t totalEvents = 1'000'000;
    constexpr size_t numberOfRuns = 10;

    std::cout << "NesStatistics Microbenchmark\n";
    std::cout << "Total events per phase: " << totalEvents << "\n\n";

    const std::string outputFile = "nes_statistics_benchmark.csv";
    std::ofstream out(outputFile, std::ios::trunc);
    ASSERT_TRUE(out.is_open());
    out << "threads,run,throughput_events_s,avg_insertion_ns,max_insertion_ns,slow_insertions_pct\n";
    out.close();

    for (const size_t numberOfThreads : {1, 2, 4, 8}) {
        runConfiguration(numberOfThreads, totalEvents, numberOfRuns, outputFile);
    }
#else
    GTEST_SKIP() << "NES_STATISTICS_ENABLED is not defined.";
#endif
}

}
