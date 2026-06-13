#pragma once
#include <string>
#include <QueryId.hpp>
#include <chrono>

namespace NES {

class NesStatisticsEvents{
public:
    NesStatisticsEvents()
    {
        timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
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

class NesQueryStartedEvent : public NesStatisticsEvents
{
public:
    explicit NesQueryStartedEvent(QueryId queryId, std::chrono::system_clock::time_point timestamp);

    std::string toCSV() const override;

private:
    QueryId id;
    std::chrono::system_clock::time_point timestamp;

};

class NesOperatorInputTuplesEvent : public NesStatisticsEvents
{
    public:
    explicit NesOperatorInputTuplesEvent(int queryId, double executionTimeMs);
    std::string toCSV() const override;

    private:
    double executionTimeMs;
};

// TODO: multiple queues? for different events?

}
