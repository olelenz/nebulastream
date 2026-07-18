#pragma once
#include <chrono>
#include <string>
#include <QueryId.hpp>

namespace NES {

enum class EventTypeIndex : uint8_t {
    BufferAlloc    = 0,
    CompilationTime = 1,
    QueryStarted   = 2,
    QueryStopped   = 3,
    QueryRegistered = 4,
    QueryFailed    = 5,
    QueryResourceDelta = 6,
    Other          = 7,
    COUNT
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
    virtual std::string getQueryId() const = 0;
    virtual uint64_t getMetricValue() const = 0;
    virtual EventTypeIndex getTypeIndex() const { return EventTypeIndex::Other; }
    uint64_t getTimestamp() const { return timestamp; }
private:
    uint64_t timestamp;
};

class NesBufferAllocateEvent : public NesStatisticsEvents{
public:
    static constexpr EventTypeIndex typeIndex = EventTypeIndex::BufferAlloc;

    explicit NesBufferAllocateEvent(QueryId queryId, size_t bufferSize);

    std::string toCSV() const override;

    std::string getEventType() const override {return "BufferAllocation";};
    std::string getQueryId() const override {return id.getLocalQueryId().getRawValue();}
    uint64_t getMetricValue() const override {return size;}
    EventTypeIndex getTypeIndex() const override { return typeIndex; }

private:
    QueryId id;
    size_t size;
};

class NesBufferManagerAllocateEvent : public NesStatisticsEvents{
public:
    static constexpr EventTypeIndex typeIndex = EventTypeIndex::BufferAlloc;

    explicit NesBufferManagerAllocateEvent(size_t bufferSize);

    std::string toCSV() const override;

    std::string getEventType() const override {return "BufferManagerAllocation";};
    std::string getQueryId() const override {return "";}
    uint64_t getMetricValue() const override {return size;}
    EventTypeIndex getTypeIndex() const override { return typeIndex; }

private:
    size_t size;
};

class NesCompilationTimeEvent : public NesStatisticsEvents{
public:
    static constexpr EventTypeIndex typeIndex = EventTypeIndex::CompilationTime;

    explicit NesCompilationTimeEvent(QueryId queryId, size_t bufferSize);

    std::string toCSV() const override;
    std::string getEventType() const override {return "CompilationTime";};
    std::string getQueryId() const override {return id.getLocalQueryId().getRawValue();}
    uint64_t getMetricValue() const override {return size;}
    EventTypeIndex getTypeIndex() const override { return typeIndex; }

private:
    QueryId id;
    size_t size;
};

class NesQueryStartedEvent : public NesStatisticsEvents
{
public:
    static constexpr EventTypeIndex typeIndex = EventTypeIndex::QueryStarted;

    explicit NesQueryStartedEvent(QueryId queryId);

    std::string toCSV() const override;
    std::string getEventType() const override { return "QueryStarted"; }
    std::string getQueryId() const override { return id.getLocalQueryId().getRawValue(); }
    uint64_t getMetricValue() const override { return 0; }
    EventTypeIndex getTypeIndex() const override { return typeIndex; }

private:
    QueryId id;
};

class NesQueryStoppedEvent : public NesStatisticsEvents
{
public:
    static constexpr EventTypeIndex typeIndex = EventTypeIndex::QueryStopped;

    explicit NesQueryStoppedEvent(QueryId queryId);
    std::string toCSV() const override;
    std::string getEventType() const override { return "QueryStopped"; }
    std::string getQueryId() const override { return id.getLocalQueryId().getRawValue(); }
    uint64_t getMetricValue() const override { return 0; }
    EventTypeIndex getTypeIndex() const override { return typeIndex; }

    private:
    QueryId id;
};

class NesQueryRegisteredEvent : public NesStatisticsEvents
{
public:
    static constexpr EventTypeIndex typeIndex = EventTypeIndex::QueryRegistered;

    explicit NesQueryRegisteredEvent(QueryId queryId);
    std::string toCSV() const override;
    std::string getEventType() const override { return "QueryRegistered"; }
    std::string getQueryId() const override { return id.getLocalQueryId().getRawValue(); }
    uint64_t getMetricValue() const override { return 0; }
    EventTypeIndex getTypeIndex() const override { return typeIndex; }

    private:
    QueryId id;
};

class NesQueryFailedEvent : public NesStatisticsEvents
{
public:
    static constexpr EventTypeIndex typeIndex = EventTypeIndex::QueryFailed;

    explicit NesQueryFailedEvent(QueryId queryId, std::string exceptionMessage);
    std::string toCSV() const override;
    std::string getEventType() const override { return "QueryFailed"; }
    std::string getQueryId() const override { return id.getLocalQueryId().getRawValue(); }
    uint64_t getMetricValue() const override { return 0; }
    EventTypeIndex getTypeIndex() const override { return typeIndex; }

    private:
    QueryId id;
    std::string exceptionMessage;
};

class NesWorkerCpuTimeEvent : public NesStatisticsEvents
{
public:
    explicit NesWorkerCpuTimeEvent(QueryId queryId, uint64_t cpuTimeMicros);
    std::string toCSV() const override;
    std::string getEventType() const override { return "WorkerCpuTime"; }
    std::string getQueryId() const override { return "0"; }
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
    std::string getEventType() const override { return "WorkerMemoryUsage"; }
    std::string getQueryId() const override { return "0"; }
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
    std::string getQueryId() const override { return "0"; }
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
    std::string getQueryId() const override { return "0"; }
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
    std::string getQueryId() const override { return "0"; }
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
    std::string getQueryId() const override { return "0"; }
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
    std::string getQueryId() const override { return "0"; }
    uint64_t getMetricValue() const override { return bufferUsedBytes; }

private:
    QueryId id;
    uint64_t bufferUsedBytes;
};

class NesQueryResourceDeltaEvent : public NesStatisticsEvents
{
public:
    static constexpr EventTypeIndex typeIndex = EventTypeIndex::QueryResourceDelta;

    explicit NesQueryResourceDeltaEvent(
        QueryId queryId,
        std::chrono::system_clock::time_point startTimestamp,
        std::chrono::system_clock::time_point stopTimestamp,
        uint64_t cpuDeltaMicros,
        int64_t memoryDeltaKb);
    std::string toCSV() const override;
    std::string getEventType() const override { return "QueryResourceDelta"; }
    std::string getQueryId() const override { return id.getLocalQueryId().getRawValue(); }
    uint64_t getMetricValue() const override { return cpuDeltaMicros; }
    EventTypeIndex getTypeIndex() const override { return typeIndex; }

private:
    QueryId id;
    std::chrono::system_clock::time_point startTimestamp;
    std::chrono::system_clock::time_point stopTimestamp;
    uint64_t cpuDeltaMicros;
    int64_t memoryDeltaKb;
};

}
