#include <Util/Statistics/NesStatisticsEvents.hpp>
#include <QueryId.hpp>

namespace NES {

NesBufferAllocateEvent::NesBufferAllocateEvent(int queryId, size_t bufferSize) : id(queryId), size(bufferSize){};

std::string NesBufferAllocateEvent::toCSV() const {
    return "BufferAllocation " + std::to_string(id) + " : " + std::to_string(size);  // TODO: make this actual csv
}

NesQueryStartedEvent::NesQueryStartedEvent(QueryId queryId, std::chrono::system_clock::time_point timestamp) : id(queryId), timestamp(timestamp) {};
std::string NesQueryStartedEvent::toCSV() const {
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
         timestamp.time_since_epoch()
     ).count();
    return "QueryStarted " + id.getLocalQueryId().getRawValue() + "," + std::to_string(ms);  // TODO: make this actual csv
}

}
