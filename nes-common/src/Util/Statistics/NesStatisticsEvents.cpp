#include <Util/Statistics/NesStatisticsEvents.hpp>
#include <QueryId.hpp>

#include <utility>

namespace NES {

NesBufferAllocateEvent::NesBufferAllocateEvent(QueryId queryId, size_t bufferSize) : id(queryId), size(bufferSize){};

std::string NesBufferAllocateEvent::toString() const {
    return "BufferAllocation " + id.getLocalQueryId().getRawValue() + " : " + std::to_string(size);
}

NesBufferManagerAllocateEvent::NesBufferManagerAllocateEvent(size_t bufferSize) : size(bufferSize){};

std::string NesBufferManagerAllocateEvent::toString() const {
    return "BufferManagerAllocation : " + std::to_string(size);
}

NesCompilationTimeEvent::NesCompilationTimeEvent(QueryId queryId, size_t compilationTimeMs) : id(queryId), size(compilationTimeMs){};

std::string NesCompilationTimeEvent::toString() const {
    return "CompilationTime " + id.getLocalQueryId().getRawValue() + " : " + std::to_string(size);
}

// Query level events
NesQueryStartedEvent::NesQueryStartedEvent(QueryId queryId) : id(queryId) {};
std::string NesQueryStartedEvent::toString() const {
    return "QueryStarted ," + id.getLocalQueryId().getRawValue() + "," + std::to_string(getTimestamp());
}

NesQueryStoppedEvent::NesQueryStoppedEvent(QueryId queryId) : id(queryId) {};
std::string NesQueryStoppedEvent::toString() const
{
    return "QueryStopped ," + id.getLocalQueryId().getRawValue() + "," + std::to_string(getTimestamp());
}

NesQueryRegisteredEvent::NesQueryRegisteredEvent(QueryId queryId) : id(queryId) {};
std::string NesQueryRegisteredEvent::toString() const
{
    return "QueryRegistered ," + id.getLocalQueryId().getRawValue() + "," + std::to_string(getTimestamp());
}

NesQueryFailedEvent::NesQueryFailedEvent(QueryId queryId, std::string exceptionMessage)
    : id(queryId), exceptionMessage(std::move(exceptionMessage)) {};
std::string NesQueryFailedEvent::toString() const
{
    return "QueryFailed ," + id.getLocalQueryId().getRawValue() + "," + exceptionMessage;
}

NesWorkerCpuTimeEvent::NesWorkerCpuTimeEvent(QueryId queryId, uint64_t cpuTimeMicros) : id(queryId), cpuTimeMicros(cpuTimeMicros) {};
std::string NesWorkerCpuTimeEvent::toString() const
{
    return "WorkerCpuTime ," + id.getLocalQueryId().getRawValue() + "," + std::to_string(cpuTimeMicros);
}

NesWorkerMemoryUsageEvent::NesWorkerMemoryUsageEvent(QueryId queryId, uint64_t residentMemoryKb)
    : id(queryId), residentMemoryKb(residentMemoryKb) {};
std::string NesWorkerMemoryUsageEvent::toString() const
{
    return "WorkerMemoryUsage ," + id.getLocalQueryId().getRawValue() + "," + std::to_string(residentMemoryKb);
}

NesWorkerActiveQueryCountEvent::NesWorkerActiveQueryCountEvent(QueryId queryId, uint64_t activeQueryCount)
    : id(queryId), activeQueryCount(activeQueryCount) {};
std::string NesWorkerActiveQueryCountEvent::toString() const
{
    return "WorkerActiveQueryCount ," + id.getLocalQueryId().getRawValue() + "," + std::to_string(activeQueryCount);
}

NesWorkerBufferTotalCountEvent::NesWorkerBufferTotalCountEvent(QueryId queryId, uint64_t bufferTotalCount)
    : id(queryId), bufferTotalCount(bufferTotalCount) {};
std::string NesWorkerBufferTotalCountEvent::toString() const
{
    return "WorkerBufferTotalCount ," + id.getLocalQueryId().getRawValue() + "," + std::to_string(bufferTotalCount);
}

NesWorkerBufferAvailableCountEvent::NesWorkerBufferAvailableCountEvent(QueryId queryId, uint64_t bufferAvailableCount)
    : id(queryId), bufferAvailableCount(bufferAvailableCount) {};
std::string NesWorkerBufferAvailableCountEvent::toString() const
{
    return "WorkerBufferAvailableCount ," + id.getLocalQueryId().getRawValue() + "," + std::to_string(bufferAvailableCount);
}

NesWorkerBufferUsedCountEvent::NesWorkerBufferUsedCountEvent(QueryId queryId, uint64_t bufferUsedCount)
    : id(queryId), bufferUsedCount(bufferUsedCount) {};
std::string NesWorkerBufferUsedCountEvent::toString() const
{
    return "WorkerBufferUsedCount ," + id.getLocalQueryId().getRawValue() + "," + std::to_string(bufferUsedCount);
}

NesWorkerBufferUsedBytesEvent::NesWorkerBufferUsedBytesEvent(QueryId queryId, uint64_t bufferUsedBytes)
    : id(queryId), bufferUsedBytes(bufferUsedBytes) {};
std::string NesWorkerBufferUsedBytesEvent::toString() const
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

std::string NesQueryResourceDeltaEvent::toString() const
{
    const auto startMs = std::chrono::duration_cast<std::chrono::milliseconds>(startTimestamp.time_since_epoch()).count();
    const auto stopMs = std::chrono::duration_cast<std::chrono::milliseconds>(stopTimestamp.time_since_epoch()).count();
    return "QueryResourceDelta ," + id.getLocalQueryId().getRawValue() + "," + std::to_string(startMs) + "," + std::to_string(stopMs)
        + "," + std::to_string(cpuDeltaMicros) + "," + std::to_string(memoryDeltaKb);
}

}
