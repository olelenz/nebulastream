#include <Util/Statistics/NesStatisticsEvents.hpp>
#include <QueryId.hpp>

#include <utility>

namespace NES {

NesBufferAllocateEvent::NesBufferAllocateEvent(int queryId, size_t bufferSize) : id(queryId), size(bufferSize){};

std::string NesBufferAllocateEvent::toCSV() const {
    return "BufferAllocation " + std::to_string(id) + " : " + std::to_string(size);  // TODO: make this actual csv
}

NesCompilationTimeEvent::NesCompilationTimeEvent(int queryId, size_t compilationTimeMs) : id(queryId), size(compilationTimeMs){};

std::string NesCompilationTimeEvent::toCSV() const {
    return "CompilationTime " + std::to_string(id) + " : " + std::to_string(size);  // TODO: make csv
}

// Query level events
NesQueryStartedEvent::NesQueryStartedEvent(QueryId queryId) : id(queryId) {};
std::string NesQueryStartedEvent::toCSV() const {
    return "QueryStarted ," + id.getLocalQueryId().getRawValue() + "," + std::to_string(getTimestamp());
}

NesQueryStoppedEvent::NesQueryStoppedEvent(QueryId queryId) : id(queryId) {};
std::string NesQueryStoppedEvent::toCSV() const
{
    return "QueryStopped ," + id.getLocalQueryId().getRawValue() + "," + std::to_string(getTimestamp());
}

NesQueryRegisteredEvent::NesQueryRegisteredEvent(QueryId queryId) : id(queryId) {};
std::string NesQueryRegisteredEvent::toCSV() const
{
    return "QueryRegistered ," + id.getLocalQueryId().getRawValue() + "," + std::to_string(getTimestamp());
}

NesQueryFailedEvent::NesQueryFailedEvent(QueryId queryId, std::string exceptionMessage)
    : id(queryId), exceptionMessage(std::move(exceptionMessage)) {};
std::string NesQueryFailedEvent::toCSV() const
{
    return "QueryFailed ," + id.getLocalQueryId().getRawValue() + "," + exceptionMessage;
}

NesWorkerCpuTimeEvent::NesWorkerCpuTimeEvent(QueryId queryId, uint64_t cpuTimeMicros) : id(queryId), cpuTimeMicros(cpuTimeMicros) {};
std::string NesWorkerCpuTimeEvent::toCSV() const
{
    return "WorkerCpuTime ," + id.getLocalQueryId().getRawValue() + "," + std::to_string(cpuTimeMicros);
}

NesWorkerMemoryUsageEvent::NesWorkerMemoryUsageEvent(QueryId queryId, uint64_t residentMemoryKb)
    : id(queryId), residentMemoryKb(residentMemoryKb) {};
std::string NesWorkerMemoryUsageEvent::toCSV() const
{
    return "WorkerMemoryUsage ," + id.getLocalQueryId().getRawValue() + "," + std::to_string(residentMemoryKb);
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

std::string NesQueryResourceDeltaEvent::toCSV() const
{
    const auto startMs = std::chrono::duration_cast<std::chrono::milliseconds>(startTimestamp.time_since_epoch()).count();
    const auto stopMs = std::chrono::duration_cast<std::chrono::milliseconds>(stopTimestamp.time_since_epoch()).count();
    return "QueryResourceDelta ," + id.getLocalQueryId().getRawValue() + "," + std::to_string(startMs) + "," + std::to_string(stopMs)
        + "," + std::to_string(cpuDeltaMicros) + "," + std::to_string(memoryDeltaKb);
}

}
