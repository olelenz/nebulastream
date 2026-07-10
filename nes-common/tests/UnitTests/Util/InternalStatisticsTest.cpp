#include <filesystem>
#include <fstream>
#include <string>

#include <Util/Files.hpp>
#include <gtest/gtest.h>
#include "Util/Statistics/NesStatistics.hpp"

namespace NES
{
class MockStatsEvent : public NesStatisticsEvents
{
public:
    explicit MockStatsEvent(int value) : val(value){}
    std::string toCSV() const override
    {
        return "val_" + std::to_string(val);
    }
    std::string getEventType() const override { return "MockEvent"; }
    std::string getQueryId() const override { return "0"; }
    uint64_t getMetricValue() const override { return val; }
private:
    int val;
};

class InternalStatisticsTest : public ::testing::Test
{
    protected:
    void SetUp() override
    {
        NesStatistics::getInstance().shutdown();
    }
    void TearDown() override
    {
        NesStatistics::getInstance().shutdown();
    }

};

TEST_F(InternalStatisticsTest, TestInMemoryLoggingAndRetieval)
{
    auto& stats = NesStatistics::getInstance();
    stats.start(StatisticsWorkerType::RingBuffer);

    stats.nesStats(std::make_unique<MockStatsEvent>(13));
    stats.nesStats(std::make_unique<MockStatsEvent>(33));

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    std::string res = stats.getStats();
    EXPECT_NE(res.find("val_13"), std::string::npos);
    EXPECT_NE(res.find("val_33"), std::string::npos);
}

TEST_F(InternalStatisticsTest, TestRollingEviction)
{
    auto& stats = NesStatistics::getInstance();
    stats.start(StatisticsWorkerType::RingBuffer);

    const int totalEvents = 10000;
    for (int i = 0; i < totalEvents; i++){
        stats.nesStats(std::make_unique<MockStatsEvent>(i));
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    std::string res = stats.getStats();

    int lineCount = 0;
    std::stringstream ss(res);
    std::string line;
    while (std::getline(ss, line)){
        if (!line.empty()){
            lineCount++;
        }
    }

    EXPECT_EQ(lineCount, 4096);
    EXPECT_EQ(res.find("val_0\n"), std::string::npos);
    EXPECT_NE(res.find("val_9999\n"), std::string::npos);
}

TEST_F(InternalStatisticsTest, TestRollingEvictionBoundary)
{
    auto& stats = NesStatistics::getInstance();
    stats.start(StatisticsWorkerType::RingBuffer);

    const int totalEvents = 4096;
    for (int i = 0; i < totalEvents; i++){
        stats.nesStats(std::make_unique<MockStatsEvent>(i));
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    std::string res = stats.getStats();

    int lineCount = 0;
    std::stringstream ss(res);
    std::string line;
    while (std::getline(ss, line)){
        if (!line.empty()){
            lineCount++;
        }
    }

    EXPECT_EQ(lineCount, 4096);
    EXPECT_NE(res.find("val_0\n"), std::string::npos);
    EXPECT_NE(res.find("val_4095\n"), std::string::npos);
    EXPECT_EQ(res.find("val_4096\n"), std::string::npos);

    stats.nesStats(std::make_unique<MockStatsEvent>(totalEvents));
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    std::string res2 = stats.getStats();
    EXPECT_EQ(res2.find("val_0\n"), std::string::npos);
    EXPECT_NE(res2.find("val_4095\n"), std::string::npos);
    EXPECT_NE(res2.find("val_4096\n"), std::string::npos);
}

TEST_F(InternalStatisticsTest, TestEmptyBuffer)
{
    auto& stats = NesStatistics::getInstance();
    stats.start(StatisticsWorkerType::RingBuffer);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    std::string res = stats.getStats();
    EXPECT_EQ(res, "");
}

TEST_F(InternalStatisticsTest, TestReset)
{
    auto& stats = NesStatistics::getInstance();
    stats.start(StatisticsWorkerType::RingBuffer);
    stats.nesStats(std::make_unique<MockStatsEvent>(0));
    stats.nesStats(std::make_unique<MockStatsEvent>(1));
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    std::string res = stats.getStats();
    EXPECT_NE(res.find("val_0\n"), std::string::npos);
    EXPECT_NE(res.find("val_1\n"), std::string::npos);

    stats.shutdown();
    stats.start(StatisticsWorkerType::RingBuffer);
    stats.nesStats(std::make_unique<MockStatsEvent>(2));
    stats.nesStats(std::make_unique<MockStatsEvent>(3));
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    std::string res2 = stats.getStats();
    EXPECT_EQ(res2.find("val_0\n"), std::string::npos);
    EXPECT_EQ(res2.find("val_1\n"), std::string::npos);
    EXPECT_NE(res2.find("val_2\n"), std::string::npos);
    EXPECT_NE(res2.find("val_3\n"), std::string::npos);
}

TEST_F(InternalStatisticsTest, TestStopWithoutStart){
    auto& stats = NesStatistics::getInstance();
    stats.shutdown();
}

TEST_F(InternalStatisticsTest, TestConcurrentLogging){
    auto& stats = NesStatistics::getInstance();
    stats.start(StatisticsWorkerType::RingBuffer);

    const int numThreads = 8;
    const int eventsPerThread = 10000;
    std::vector<std::thread> threads;
    for (int t = 0; t < numThreads; t++)
    {
        threads.emplace_back([&stats, t]
        {
           for (int i = 0; i < eventsPerThread; i++)
           {
               stats.nesStats(std::make_unique<MockStatsEvent>(t*eventsPerThread+i));
           }
        });
    }

    for (auto& thread : threads)
    {
        thread.join();
    }

    stats.shutdown();
    std::string res = stats.getStats();

    int lineCount = 0;
    std::stringstream ss(res);
    std::string line;
    while (std::getline(ss, line))
    {
        if (!line.empty())
        {
            lineCount++;
        }
    }
    EXPECT_EQ(lineCount, 4096);
}

TEST_F(InternalStatisticsTest, TestWriteRead)
{
    auto& stats = NesStatistics::getInstance();
    stats.start(StatisticsWorkerType::RingBuffer);
    std::atomic<bool> writingDone{false};
    std::thread reader([&stats, &writingDone]()
    {
        while (!writingDone)
        {
            std::string curStats = stats.getStats();
            std::this_thread::sleep_for(std::chrono::milliseconds(3));
        }
    });
    const int numThreads = 4;
    const int eventsPerThread = 10000;
    std::vector<std::thread> writers;
    for (int t = 0; t < numThreads; t++)
    {
        writers.emplace_back([&stats, t]()
        {
            for (int i = 0; i < eventsPerThread; i++)
            {
                stats.nesStats(std::make_unique<MockStatsEvent>(t*eventsPerThread+i));
            }
        });
    }

    for (auto& writer : writers)
    {
        writer.join();
    }

    writingDone = true;
    reader.join();

    stats.shutdown();
    std::string res = stats.getStats();
    int lineCount = 0;
    std::stringstream ss(res);
    std::string line;
    while (std::getline(ss, line))
    {
        if (!line.empty())
        {
            lineCount++;
        }
    }
    EXPECT_EQ(lineCount, 4096);
}

TEST_F(InternalStatisticsTest, TestGetEventsSince)
{
    auto& stats = NesStatistics::getInstance();
    stats.start(StatisticsWorkerType::RingBuffer);

    stats.nesStats(std::make_unique<MockStatsEvent>(0));
    stats.nesStats(std::make_unique<MockStatsEvent>(1));
    stats.nesStats(std::make_unique<MockStatsEvent>(2));
    stats.nesStats(std::make_unique<MockStatsEvent>(3));
    stats.nesStats(std::make_unique<MockStatsEvent>(4));

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    stats.shutdown();

    std::vector<RawEventData> allEvents = stats.getEventsSince(0);
    ASSERT_EQ(allEvents.size(), 5);

    for (size_t i = 0; i < allEvents.size(); i++) {
        EXPECT_EQ(allEvents[i].seq, i);
        EXPECT_EQ(allEvents[i].metricValue, i);
        EXPECT_EQ(allEvents[i].eventType, "MockEvent");
        EXPECT_GT(allEvents[i].ts, 0);
    }

    std::vector<RawEventData> partialEvents = stats.getEventsSince(3);
    ASSERT_EQ(partialEvents.size(), 2);
    
    EXPECT_EQ(partialEvents[0].seq, 3);
    EXPECT_EQ(partialEvents[0].metricValue, 3);

    EXPECT_EQ(partialEvents[1].seq, 4);
    EXPECT_EQ(partialEvents[1].metricValue, 4);

    std::vector<RawEventData> emptyEvents = stats.getEventsSince(10);
    EXPECT_TRUE(emptyEvents.empty());
}

}