#pragma once
#include <atomic>
#include <condition_variable>
#include <fstream>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <array>
#include <folly/MPMCQueue.h>
#include <folly/Synchronized.h>
#include <optional>
#include <unordered_map>
#include <chrono>
#include <functional>
#include "NesStatisticsEvents.hpp"
#include "NesResourceUsage.hpp"

// TODO: we should make this fast

namespace NES {

enum class StatisticsWorkerType {
    Buffered,
    RingBuffer
};

template<typename T, std::size_t N>
struct CircularBuffer {
    static constexpr std::size_t capacity = N;

    struct Entry
    {
        uint64_t seq;
        T item;
    };

    std::array<Entry, N> slots{};
    std::size_t head{0};
    std::size_t count{0};
    uint64_t nextSeq{0};

    void push(T&& item) {
        slots[head] = Entry{nextSeq++, std::move(item)};  // does not copy the data
        head = (head + 1) % N;
        count = std::min(count + 1, N);
    }

    template<typename Fn>
    void forEach(Fn&& fn) const {
        const std::size_t start = (head + N - count) % N;
        for (std::size_t i = 0; i < count; ++i) {
            const auto& entry = slots[(start + i) % N];
            fn(entry.seq, entry.item);
        }
    }

    void clear(){
        for(std::size_t i = 0; i < N; i++){
            slots[i].item.reset();
            slots[i].seq = 0;
        }
        head = 0;
        count = 0;
        nextSeq = 0;
    }
};

struct RawEventData
{
    uint64_t seq;
    uint64_t ts;
    uint64_t queryId;
    uint64_t metricValue;
    std::string eventType;
};

struct WorkerBufferUsageSnapshot
{
    uint64_t totalCount;
    uint64_t availableCount;
    uint64_t bufferSize;
};

class NesStatistics{
    public:
        static NesStatistics& getInstance(){
            static NesStatistics instance;
            return instance;
        }

        void start(StatisticsWorkerType type, const std::string& filePath = "");
        void nesStats(std::unique_ptr<NesStatisticsEvents> event);
        void shutdown();
        void recordQueryResourceStart(QueryId queryId, QueryResourceSnapshot snapshot);
        std::optional<QueryResourceSnapshot> consumeQueryResourceStart(QueryId queryId);
        void setActiveQueryCountProvider(std::function<uint64_t()> provider);
        void setWorkerBufferUsageProvider(std::function<WorkerBufferUsageSnapshot()> provider);

        std::string getStats() const;  // only relevant for the ring-buffer mode
        std::vector<RawEventData> getEventsSince(uint64_t sequenceNumber) const;

        // make this a singleton
        NesStatistics(NesStatistics const&) = delete;
        void operator=(NesStatistics const&) = delete;

    private:
        NesStatistics();
        ~NesStatistics();

        void workStatsQueue();
        void workStatsRingBuffer();
        void startResourceSampler();
        void stopResourceSampler();
        void sampleResourceUsagePeriodically();

        std::queue<std::unique_ptr<NesStatisticsEvents>> statsQueue;
        std::condition_variable condVar;
        std::mutex statsMutex;

        static constexpr std::size_t RING_BUFFER_CAPACITY = 1 << 14; // 16384
        folly::MPMCQueue<std::unique_ptr<NesStatisticsEvents>> ringBuffer{RING_BUFFER_CAPACITY};

        static constexpr std::size_t ROLLING_STORE_CAPACITY = 4096;
        folly::Synchronized<CircularBuffer<std::unique_ptr<NesStatisticsEvents>, ROLLING_STORE_CAPACITY>> rollingStore;

        StatisticsWorkerType workerType{StatisticsWorkerType::RingBuffer};
        std::atomic<bool> running{true};
        std::thread workThread;
        std::ofstream outFile; // only used by Buffered / Chunked

        static constexpr std::chrono::milliseconds DEFAULT_RESOURCE_SAMPLE_INTERVAL{1000};
        std::thread resourceSamplerThread;
        std::condition_variable resourceSamplerCondVar;
        std::mutex resourceSamplerMutex;

        std::mutex queryResourceSnapshotsMutex;
        std::unordered_map<QueryId, QueryResourceSnapshot> queryResourceSnapshots;

        std::mutex activeQueryCountProviderMutex;
        std::function<uint64_t()> activeQueryCountProvider;

        std::mutex workerBufferUsageProviderMutex;
        std::function<WorkerBufferUsageSnapshot()> workerBufferUsageProvider;
};
// TODO: Counter event? needed natively? we can just query the statistics??

template<typename EventType, typename... Args>
    void logStat(Args&&... args) {
    NesStatistics::getInstance().nesStats(
        std::make_unique<EventType>(std::forward<Args>(args)...)
    );
}

}
