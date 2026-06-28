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

TEST(InternalStatisticsTest, TestInMemoryLoggingAndRetieval)
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

TEST(InternalStatisticsTest, TestRollingEviction)
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

}