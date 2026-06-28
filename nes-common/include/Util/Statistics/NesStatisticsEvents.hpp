#pragma once
#include <string>
#include <chrono>

namespace NES {

class NesStatisticsEvents{
public:
    NesStatisticsEvents()
    {
        timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    };
    virtual ~NesStatisticsEvents() = default;
    virtual std::string toCSV() const = 0;

    virtual std::string getEventType() const = 0;
    virtual uint64_t getQueryId() const = 0;
    virtual uint64_t getMetricValue() const = 0;
    uint64_t getTimestamp() const { return timestamp; }
private:
    uint64_t timestamp;
};

class NesBufferAllocateEvent : public NesStatisticsEvents{
public:
    explicit NesBufferAllocateEvent(int queryId, size_t bufferSize);

    std::string toCSV() const override;

    std::string getEventType() const override {return "BufferAllocation";};
    uint64_t getQueryId() const override {return id;}
    uint64_t getMetricValue() const override {return size;}

private:
    int id;
    size_t size;
};

class NesCompilationTimeEvent : public NesStatisticsEvents{
public:
    explicit NesCompilationTimeEvent(int queryId, size_t bufferSize);

    std::string toCSV() const override;
    std::string getEventType() const override {return "CompilationTime";};
    uint64_t getQueryId() const override {return id;}
    uint64_t getMetricValue() const override {return size;}

private:
    int id;
    size_t size;
};

// TODO: multiple queues? for different events?

}
