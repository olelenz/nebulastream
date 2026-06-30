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
