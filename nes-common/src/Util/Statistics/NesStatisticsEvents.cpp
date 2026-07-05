#include <Util/Statistics/NesStatisticsEvents.hpp>
#include <QueryId.hpp>

namespace NES {

NesBufferAllocateEvent::NesBufferAllocateEvent(int queryId, size_t bufferSize) : id(queryId), size(bufferSize){};

std::string NesBufferAllocateEvent::toCSV() const {
    return "BufferAllocation " + std::to_string(id) + " : " + std::to_string(size);  // TODO: make this actual csv
}

// Query level events
NesQueryStartedEvent::NesQueryStartedEvent(QueryId queryId, std::chrono::system_clock::time_point timestamp) : id(queryId), timestamp(timestamp) {};
std::string NesQueryStartedEvent::toCSV() const {
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
         timestamp.time_since_epoch()
     ).count();
    return "QueryStarted ," + id.getLocalQueryId().getRawValue() + "," + std::to_string(ms);
}

NesQueryStoppedEvent::NesQueryStoppedEvent(QueryId queryId, std::chrono::system_clock::time_point timestamp) : id(queryId), timestamp(timestamp) {};
std::string NesQueryStoppedEvent::toCSV() const
{
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        timestamp.time_since_epoch()).count();
    return "QueryStopped ," + id.getLocalQueryId().getRawValue() + "," + std::to_string(ms);
}

NesQueryRegisteredEvent::NesQueryRegisteredEvent(QueryId queryId, std::chrono::system_clock::time_point timestamp) : id(queryId), timestamp(timestamp) {};
std::string NesQueryRegisteredEvent::toCSV() const
{    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        timestamp.time_since_epoch()).count();
    return "QueryRegistered ," + id.getLocalQueryId().getRawValue() + "," + std::to_string(ms);
}

NesQueryFailedEvent::NesQueryFailedEvent(QueryId queryId, std::exception exception) : id(queryId), exception(exception) {};
std::string NesQueryFailedEvent::toCSV() const
{
    return "QueryFailed ," + id.getLocalQueryId().getRawValue() + "," + exception.what();
}

NesWorkerCpuTimeEvent::NesWorkerCpuTimeEvent(QueryId queryId, uint64_t cpuTimeMicros) : id(queryId), cpuTimeMicros(cpuTimeMicros) {};
std::string NesWorkerCpuTimeEvent::toCSV() const
{
    return "WorkerCpuTimeMicros ," + id.getLocalQueryId().getRawValue() + "," + std::to_string(cpuTimeMicros);
}

NesWorkerMemoryUsageEvent::NesWorkerMemoryUsageEvent(QueryId queryId, uint64_t residentMemoryKb)
    : id(queryId), residentMemoryKb(residentMemoryKb) {};
std::string NesWorkerMemoryUsageEvent::toCSV() const
{
    return "WorkerMemoryRssKb ," + id.getLocalQueryId().getRawValue() + "," + std::to_string(residentMemoryKb);
}

NesWorkerActiveQueryCountEvent::NesWorkerActiveQueryCountEvent(QueryId queryId, uint64_t activeQueryCount)
    : id(queryId), activeQueryCount(activeQueryCount) {};
std::string NesWorkerActiveQueryCountEvent::toCSV() const
{
    return "WorkerActiveQueryCount ," + id.getLocalQueryId().getRawValue() + "," + std::to_string(activeQueryCount);
}

NesWorkerBufferTotalCountEvent::NesWorkerBufferTotalCountEvent(QueryId queryId, uint64_t bufferTotalCount)
    : id(queryId), bufferTotalCount(bufferTotalCount) {};
std::string NesWorkerBufferTotalCountEvent::toCSV() const
{
    return "WorkerBufferTotalCount ," + id.getLocalQueryId().getRawValue() + "," + std::to_string(bufferTotalCount);
}

NesWorkerBufferAvailableCountEvent::NesWorkerBufferAvailableCountEvent(QueryId queryId, uint64_t bufferAvailableCount)
    : id(queryId), bufferAvailableCount(bufferAvailableCount) {};
std::string NesWorkerBufferAvailableCountEvent::toCSV() const
{
    return "WorkerBufferAvailableCount ," + id.getLocalQueryId().getRawValue() + "," + std::to_string(bufferAvailableCount);
}

NesWorkerBufferUsedCountEvent::NesWorkerBufferUsedCountEvent(QueryId queryId, uint64_t bufferUsedCount)
    : id(queryId), bufferUsedCount(bufferUsedCount) {};
std::string NesWorkerBufferUsedCountEvent::toCSV() const
{
    return "WorkerBufferUsedCount ," + id.getLocalQueryId().getRawValue() + "," + std::to_string(bufferUsedCount);
}

NesWorkerBufferUsedBytesEvent::NesWorkerBufferUsedBytesEvent(QueryId queryId, uint64_t bufferUsedBytes)
    : id(queryId), bufferUsedBytes(bufferUsedBytes) {};
std::string NesWorkerBufferUsedBytesEvent::toCSV() const
{
    return "WorkerBufferUsedBytes ," + id.getLocalQueryId().getRawValue() + "," + std::to_string(bufferUsedBytes);
}

NesQueryResourceDeltaEvent::NesQueryResourceDeltaEvent(
    QueryId queryId,
    std::chrono::system_clock::time_point startTimestamp,
    std::chrono::system_clock::time_point stopTimestamp,
    uint64_t cpuDeltaMicros,
    int64_t memoryDeltaKb)
    : id(queryId)
    , startTimestamp(startTimestamp)
    , stopTimestamp(stopTimestamp)
    , cpuDeltaMicros(cpuDeltaMicros)
    , memoryDeltaKb(memoryDeltaKb)
{
}

}
