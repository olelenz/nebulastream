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
#include <sstream>
#include <vector>
#include <folly/MPMCQueue.h>
#include <folly/Synchronized.h>
#include "NesStatisticsEvents.hpp"

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

class NesStatistics{
    public:
        static NesStatistics& getInstance(){
            static NesStatistics instance;
            return instance;
        }

        void start(StatisticsWorkerType type, const std::string& filePath = "");
        void nesStats(std::unique_ptr<NesStatisticsEvents> event);
        void nesStatsDirect(size_t queueIdx, std::unique_ptr<NesStatisticsEvents> event);
        void shutdown();

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

        std::queue<std::unique_ptr<NesStatisticsEvents>> statsQueue;
        std::condition_variable condVar;
        std::mutex statsMutex;

        static constexpr std::size_t RING_BUFFER_CAPACITY = 1 << 14; // 16384
        static constexpr std::size_t NUM_EVENT_TYPES = static_cast<std::size_t>(EventTypeIndex::COUNT);
        std::vector<folly::MPMCQueue<std::unique_ptr<NesStatisticsEvents>>> ringBuffers;
        std::counting_semaphore<> ringBufferSem{0};

        static constexpr std::size_t ROLLING_STORE_CAPACITY = 4096;
        folly::Synchronized<CircularBuffer<std::unique_ptr<NesStatisticsEvents>, ROLLING_STORE_CAPACITY>> rollingStore;

        StatisticsWorkerType workerType{StatisticsWorkerType::RingBuffer};
        std::atomic<bool> running{false};  // set to true by start()
        std::thread workThread;
        std::ofstream outFile; // only used by Buffered / Chunked
};

template<typename EventType, typename... Args>
void logStat(Args&&... args) {
    // resolve the queue
    constexpr size_t idx = static_cast<size_t>(EventType::typeIndex);
    static_assert(
        EventType::typeIndex != EventTypeIndex::Other,
        "logStat<T>: EventType::typeIndex must not be EventTypeIndex::Other. "
        "Add a dedicated enumerator to EventTypeIndex for this event type."
    );
    NesStatistics::getInstance().nesStatsDirect(
        idx, std::make_unique<EventType>(std::forward<Args>(args)...)
    );
}

}
