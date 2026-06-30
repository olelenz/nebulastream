#pragma once
#include <chrono>
#include <cstdint>
#include <exception>
#include <string>
#include <QueryId.hpp>
#include <chrono>

namespace NES {

// TBD: Do we need to classify metric type
enum class NesMetricType {
    Counter,
    Gauge,
    Histogram,
    Meter
};

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
    std::string getEventType() const override { return "QueryStarted"; }
    uint64_t getQueryId() const override { return 0; }
    uint64_t getMetricValue() const override { return 0; }

private:
    QueryId id;
    std::chrono::system_clock::time_point timestamp;

};

class NesQueryStoppedEvent : public NesStatisticsEvents
{
    public:
    explicit NesQueryStoppedEvent(QueryId queryId, std::chrono::system_clock::time_point timestamp);
    std::string toCSV() const override;
    std::string getEventType() const override { return "QueryStopped"; }
    uint64_t getQueryId() const override { return 0; }
    uint64_t getMetricValue() const override { return 0; }

    private:
    QueryId id;
    std::chrono::system_clock::time_point timestamp;
};

class NesQueryRegisteredEvent : public NesStatisticsEvents
{
    public:
    explicit NesQueryRegisteredEvent(QueryId queryId, std::chrono::system_clock::time_point timestamp);
    std::string toCSV() const override;
    std::string getEventType() const override { return "QueryRegistered"; }
    uint64_t getQueryId() const override { return 0; }
    uint64_t getMetricValue() const override { return 0; }

    private:
    QueryId id;
    std::chrono::system_clock::time_point timestamp;
};

class NesQueryFailedEvent : public NesStatisticsEvents
{
    public:
    explicit NesQueryFailedEvent(QueryId queryId, std::exception exception);
    std::string toCSV() const override;
    std::string getEventType() const override { return "QueryFailed"; }
    uint64_t getQueryId() const override { return 0; }
    uint64_t getMetricValue() const override { return 0; }

    private:
    QueryId id;
    std::exception exception;
};

class NesWorkerCpuTimeEvent : public NesStatisticsEvents
{
public:
    explicit NesWorkerCpuTimeEvent(QueryId queryId, uint64_t cpuTimeMicros);
    std::string toCSV() const override;
    std::string getEventType() const override { return "WorkerCpuTimeMicros"; }
    uint64_t getQueryId() const override { return 0; }
    uint64_t getMetricValue() const override { return cpuTimeMicros; }

private:
    QueryId id;
    uint64_t cpuTimeMicros;
};

class NesWorkerMemoryUsageEvent : public NesStatisticsEvents
{
public:
    explicit NesWorkerMemoryUsageEvent(QueryId queryId, uint64_t residentMemoryKb);
    std::string toCSV() const override;
    std::string getEventType() const override { return "WorkerMemoryRssKb"; }
    uint64_t getQueryId() const override { return 0; }
    uint64_t getMetricValue() const override { return residentMemoryKb; }

private:
    QueryId id;
    uint64_t residentMemoryKb;
};

class NesQueryResourceDeltaEvent : public NesStatisticsEvents
{
public:
    explicit NesQueryResourceDeltaEvent(
        QueryId queryId,
        std::chrono::system_clock::time_point startTimestamp,
        std::chrono::system_clock::time_point stopTimestamp,
        uint64_t cpuDeltaMicros,
        int64_t memoryDeltaKb);
    std::string toCSV() const override;
    std::string getEventType() const override { return "QueryResourceDelta"; }
    uint64_t getQueryId() const override { return 0; }
    uint64_t getMetricValue() const override { return cpuDeltaMicros; }

private:
    QueryId id;
    std::chrono::system_clock::time_point startTimestamp;
    std::chrono::system_clock::time_point stopTimestamp;
    uint64_t cpuDeltaMicros;
    int64_t memoryDeltaKb;
};

class NesOperatorInputTuplesEvent : public NesStatisticsEvents
{
    public:
    explicit NesOperatorInputTuplesEvent(int queryId, double executionTimeMs);
    std::string toCSV() const override;
    std::string getEventType() const override { return "OperatorInputTuples"; }
    uint64_t getQueryId() const override { return 0; }
    uint64_t getMetricValue() const override { return static_cast<uint64_t>(executionTimeMs); }

    private:
    double executionTimeMs;
};

// TODO: multiple queues? for different events?

}
