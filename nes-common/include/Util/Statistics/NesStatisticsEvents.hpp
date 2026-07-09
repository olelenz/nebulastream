#pragma once
#include <chrono>
#include <functional>
#include <string>
#include <QueryId.hpp>

namespace NES {

// Convert QueryId to uint64_t to match the flattened StatsSource schema.
inline uint64_t getFlattenedQueryId(const QueryId queryId)
{
    return static_cast<uint64_t>(std::hash<QueryId>{}(queryId));
}

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
    explicit NesQueryStartedEvent(QueryId queryId);

    std::string toCSV() const override;
    std::string getEventType() const override { return "QueryStarted"; }
    uint64_t getQueryId() const override { return getFlattenedQueryId(id); }
    uint64_t getMetricValue() const override { return 0; }

private:
    QueryId id;
};

class NesQueryStoppedEvent : public NesStatisticsEvents
{
    public:
    explicit NesQueryStoppedEvent(QueryId queryId);
    std::string toCSV() const override;
    std::string getEventType() const override { return "QueryStopped"; }
    uint64_t getQueryId() const override { return getFlattenedQueryId(id); }
    uint64_t getMetricValue() const override { return 0; }

    private:
    QueryId id;
};

class NesQueryRegisteredEvent : public NesStatisticsEvents
{
    public:
    explicit NesQueryRegisteredEvent(QueryId queryId);
    std::string toCSV() const override;
    std::string getEventType() const override { return "QueryRegistered"; }
    uint64_t getQueryId() const override { return getFlattenedQueryId(id); }
    uint64_t getMetricValue() const override { return 0; }

    private:
    QueryId id;
};

class NesQueryFailedEvent : public NesStatisticsEvents
{
    public:
    explicit NesQueryFailedEvent(QueryId queryId, std::string exceptionMessage);
    std::string toCSV() const override;
    std::string getEventType() const override { return "QueryFailed"; }
    uint64_t getQueryId() const override { return getFlattenedQueryId(id); }
    uint64_t getMetricValue() const override { return 0; }

    private:
    QueryId id;
    std::string exceptionMessage;
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

class NesWorkerActiveQueryCountEvent : public NesStatisticsEvents
{
public:
    explicit NesWorkerActiveQueryCountEvent(QueryId queryId, uint64_t activeQueryCount);
    std::string toCSV() const override;
    std::string getEventType() const override { return "WorkerActiveQueryCount"; }
    uint64_t getQueryId() const override { return 0; }
    uint64_t getMetricValue() const override { return activeQueryCount; }

private:
    QueryId id;
    uint64_t activeQueryCount;
};

class NesWorkerBufferTotalCountEvent : public NesStatisticsEvents
{
public:
    explicit NesWorkerBufferTotalCountEvent(QueryId queryId, uint64_t bufferTotalCount);
    std::string toCSV() const override;
    std::string getEventType() const override { return "WorkerBufferTotalCount"; }
    uint64_t getQueryId() const override { return 0; }
    uint64_t getMetricValue() const override { return bufferTotalCount; }

private:
    QueryId id;
    uint64_t bufferTotalCount;
};

class NesWorkerBufferAvailableCountEvent : public NesStatisticsEvents
{
public:
    explicit NesWorkerBufferAvailableCountEvent(QueryId queryId, uint64_t bufferAvailableCount);
    std::string toCSV() const override;
    std::string getEventType() const override { return "WorkerBufferAvailableCount"; }
    uint64_t getQueryId() const override { return 0; }
    uint64_t getMetricValue() const override { return bufferAvailableCount; }

private:
    QueryId id;
    uint64_t bufferAvailableCount;
};

class NesWorkerBufferUsedCountEvent : public NesStatisticsEvents
{
public:
    explicit NesWorkerBufferUsedCountEvent(QueryId queryId, uint64_t bufferUsedCount);
    std::string toCSV() const override;
    std::string getEventType() const override { return "WorkerBufferUsedCount"; }
    uint64_t getQueryId() const override { return 0; }
    uint64_t getMetricValue() const override { return bufferUsedCount; }

private:
    QueryId id;
    uint64_t bufferUsedCount;
};

class NesWorkerBufferUsedBytesEvent : public NesStatisticsEvents
{
public:
    explicit NesWorkerBufferUsedBytesEvent(QueryId queryId, uint64_t bufferUsedBytes);
    std::string toCSV() const override;
    std::string getEventType() const override { return "WorkerBufferUsedBytes"; }
    uint64_t getQueryId() const override { return 0; }
    uint64_t getMetricValue() const override { return bufferUsedBytes; }

private:
    QueryId id;
    uint64_t bufferUsedBytes;
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
    uint64_t getQueryId() const override { return getFlattenedQueryId(id); }
    uint64_t getMetricValue() const override { return cpuDeltaMicros; }

private:
    QueryId id;
    std::chrono::system_clock::time_point startTimestamp;
    std::chrono::system_clock::time_point stopTimestamp;
    uint64_t cpuDeltaMicros;
    int64_t memoryDeltaKb;
};

}
