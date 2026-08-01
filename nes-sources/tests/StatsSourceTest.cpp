#include <gtest/gtest.h>
#include <StatsSource.hpp>
#include <Runtime/TupleBuffer.hpp>
#include <Runtime/BufferManager.hpp>
#include <Sources/SourceCatalog.hpp>
#include <Util/Statistics/NesStatistics.hpp>
#include <DataTypes/DataTypeProvider.hpp>
#include <chrono>
#include <thread>
#include <stop_token>
#include <future>
#include <BaseUnitTest.hpp>

using namespace NES;

class StatsSourceTest : public Testing::BaseUnitTest {
protected:

    void SetUp() override {
        auto& stats = NesStatistics::getInstance();
        stats.start(StatisticsWorkerType::RingBuffer);
    }

    void TearDown() override {
        auto& stats = NesStatistics::getInstance();
        stats.shutdown();
    }

    std::shared_ptr<SourceDescriptor> createDescriptor(uint64_t pollIntervalMs, int32_t maxRuntimeMs) {
        SourceCatalog sourceCatalog{};
        Schema schema{};
        schema.addField("stringField", DataTypeProvider::provideDataType(DataType::Type::VARSIZED));
        auto logicalOpt = sourceCatalog.addLogicalSource("testStats", schema);
        EXPECT_TRUE(logicalOpt.has_value());
        auto physicalOpt = sourceCatalog.addPhysicalSource(*logicalOpt, "Stats", Host("localhost"), {{"poll_interval_ms", std::to_string(pollIntervalMs)}, {"max_runtime_ms", std::to_string(maxRuntimeMs)}}, {{"type", "NATIVE"}});
        EXPECT_TRUE(physicalOpt.has_value());
        return std::make_shared<SourceDescriptor>(*physicalOpt);
    }
};

TEST_F(StatsSourceTest, TestNoEventsAndStopToken) {
    auto desc = createDescriptor(500, -1);
    StatsSource source(*desc);

    auto bm = NES::BufferManager::create(4096, 10);
    source.open(bm);

    std::stop_source stopSource;
    auto tupleBuffer = bm->getBufferBlocking();

    // stop source
    stopSource.request_stop();

    auto res = source.fillTupleBuffer(tupleBuffer, stopSource.get_token());
    EXPECT_TRUE(res.isEoS());
}

TEST_F(StatsSourceTest, TestMaxRuntimeMs) {
    auto desc = createDescriptor(10, 50);
    StatsSource source(*desc);

    auto bm = NES::BufferManager::create(4096, 10);
    source.open(bm);

    const std::stop_source stopSource;
    auto tupleBuffer = bm->getBufferBlocking();

    auto start = std::chrono::steady_clock::now();
    bool hitEos = false;

    // stop background thread to avoid getting new events
    NesStatistics::getInstance().shutdown();

    while(true) {
        auto res = source.fillTupleBuffer(tupleBuffer, stopSource.get_token());
        if (res.isEoS()) {
            hitEos = true;
            break;
        }
    }
    auto end = std::chrono::steady_clock::now();

    EXPECT_TRUE(hitEos);
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    EXPECT_GE(duration, 50);
    EXPECT_LT(duration, 100);
}

TEST_F(StatsSourceTest, TestCachedLeftoversAreReturnedAfterStop) {
    auto desc = createDescriptor(10, -1);
    StatsSource source(*desc);

    constexpr size_t bufferSize = 4096;

    auto bm = NES::BufferManager::create(bufferSize, 10);
    source.open(bm);

    std::stop_source stopSource;

    auto tupleBuffer1 = bm->getBufferBlocking();
    auto tupleBuffer2 = bm->getBufferBlocking();

    size_t exactTotalSize = 0;
    size_t expectedEventCount = 0;
    while (true) {
        NES_LOG_STAT(NesBufferAllocateEvent, QueryId::createDistributed(DistributedQueryId("1")), 100);
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        auto currentEvents = NesStatistics::getInstance().getEventsSince(0);
        size_t size = 0;
        for(const auto& event : currentEvents) {
            std::ostringstream ss;
            ss << event.seq << "," << event.ts << "," << event.queryId << "," << event.metricValue << "," << event.eventType << "\n";
            size += ss.str().size();
        }

        if (size > bufferSize) {
            exactTotalSize = size;
            expectedEventCount = currentEvents.size();
            break;  // we spilled
        }
    }

    auto res1 = source.fillTupleBuffer(tupleBuffer1, stopSource.get_token());
    EXPECT_TRUE(!res1.isEoS());
    EXPECT_EQ(res1.getNumberOfBytes(), bufferSize);  // first buffer should be full

    // request stop
    stopSource.request_stop();

    // expect leftover to still be returned
    auto res2 = source.fillTupleBuffer(tupleBuffer2, stopSource.get_token());
    EXPECT_TRUE(!res2.isEoS());
    EXPECT_EQ(res2.getNumberOfBytes(), exactTotalSize - bufferSize);

    // test the actual content
    std::string combined = std::string(tupleBuffer1.getAvailableMemoryArea<char>().data(), res1.getNumberOfBytes()) + std::string(tupleBuffer2.getAvailableMemoryArea<char>().data(), res2.getNumberOfBytes());
    EXPECT_EQ(combined.size(), exactTotalSize);
    EXPECT_EQ(combined.back(), '\n');

    EXPECT_EQ(std::count(combined.begin(), combined.end(), '\n'), expectedEventCount);
}

TEST_F(StatsSourceTest, TestIndependentCursors) {
    auto desc = createDescriptor(10, -1);
    StatsSource source1(*desc);
    StatsSource source2(*desc);

    auto bm = NES::BufferManager::create(4096, 10);
    source1.open(bm);
    source2.open(bm);

    // emit event
    NES_LOG_STAT(NesBufferAllocateEvent, QueryId::createDistributed(DistributedQueryId("1")), 42);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    std::stop_source stopSource;
    auto tupleBuffer1 = bm->getBufferBlocking();
    auto tupleBuffer2 = bm->getBufferBlocking();

    auto res1 = source1.fillTupleBuffer(tupleBuffer1, stopSource.get_token());
    EXPECT_TRUE(!res1.isEoS());

    std::string str1(tupleBuffer1.getAvailableMemoryArea<char>().data(), res1.getNumberOfBytes());
    EXPECT_TRUE(str1.find("42,BufferAllocation") != std::string::npos);

    // source2 should be able to read the same event
    auto res2 = source2.fillTupleBuffer(tupleBuffer2, stopSource.get_token());
    EXPECT_TRUE(!res2.isEoS());

    std::string str2(tupleBuffer2.getAvailableMemoryArea<char>().data(), res2.getNumberOfBytes());
    EXPECT_TRUE(str2.find("42,BufferAllocation") != std::string::npos);
}
