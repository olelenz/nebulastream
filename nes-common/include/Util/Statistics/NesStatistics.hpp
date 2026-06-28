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
#include <folly/MPMCQueue.h>
#include <folly/Synchronized.h>
#include "NesStatisticsEvents.hpp"

// TODO: we should make this fast

namespace NES {

enum class StatisticsWorkerType {
    Buffered,
    Chunked,
    RingBuffer
};

template<typename T, std::size_t N>
struct CircularBuffer {
    static constexpr std::size_t capacity = N;

    std::array<T, N> slots{};
    std::size_t head{0};
    std::size_t count{0};

    void push(T&& item) {
        slots[head] = std::move(item);  // does not copy the data
        head = (head + 1) % N;
        count = std::min(count + 1, N);
    }

    template<typename Fn>
    void forEach(Fn&& fn) const {
        const std::size_t start = (head + N - count) % N;
        for (std::size_t i = 0; i < count; ++i) {
            fn(slots[(start + i) % N]);
        }
    }

    void clear(){
        for(std::size_t i = 0; i < N; i++){
            slots[i].reset();
        }
        head = 0;
        count = 0;
    }
};

class NesStatistics{
    public:
        static NesStatistics& getInstance(){
            static NesStatistics instance;
            return instance;
        }

        void start(StatisticsWorkerType type, const std::string& filePath = "");
        void nesStats(std::unique_ptr<NesStatisticsEvents> event);
        void nesStatsSlow(std::unique_ptr<NesStatisticsEvents> event);
        void shutdown();

        std::string getStats() const;  // only relevant for the ring-buffer mode

        // make this a singleton
        NesStatistics(NesStatistics const&) = delete;
        void operator=(NesStatistics const&) = delete;

    private:
        NesStatistics();
        ~NesStatistics();

        void workStatsQueue();
        void workStatsQueueChunked();
        void workStatsRingBuffer();

        std::queue<std::unique_ptr<NesStatisticsEvents>> statsQueue;
        std::condition_variable condVar;
        std::mutex statsMutex;

        static constexpr std::size_t RING_BUFFER_CAPACITY = 1 << 14; // 16384
        folly::MPMCQueue<std::unique_ptr<NesStatisticsEvents>> ringBuffer{RING_BUFFER_CAPACITY};

        static constexpr std::size_t ROLLING_STORE_CAPACITY = 4096;
        folly::Synchronized<CircularBuffer<std::unique_ptr<NesStatisticsEvents>, ROLLING_STORE_CAPACITY>> rollingStore;

        StatisticsWorkerType workerType{StatisticsWorkerType::Chunked};
        std::atomic<bool> running{true};
        std::thread workThread;
        std::ofstream outFile; // only used by Buffered / Chunked
};
// TODO: Counter event? needed natively? we can just query the statistics??

template<typename EventType, typename... Args>
    void logStat(Args&&... args) {
    NesStatistics::getInstance().nesStats(
        std::make_unique<EventType>(std::forward<Args>(args)...)
    );
}
template<typename EventType, typename... Args>
    void logStatSlow(Args&&... args) {
    NesStatistics::getInstance().nesStatsSlow(
        std::make_unique<EventType>(std::forward<Args>(args)...)
    );
}

}
