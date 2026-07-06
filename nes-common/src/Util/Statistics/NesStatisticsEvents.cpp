#include <Util/Statistics/NesStatisticsEvents.hpp>

namespace NES {

NesBufferAllocateEvent::NesBufferAllocateEvent(int queryId, size_t bufferSize) : id(queryId), size(bufferSize){};

std::string NesBufferAllocateEvent::toCSV() const {
    return "BufferAllocation " + std::to_string(id) + " : " + std::to_string(size);  // TODO: make this actual csv
}

NesCompilationTimeEvent::NesCompilationTimeEvent(int queryId, size_t compilationTimeMs) : id(queryId), size(compilationTimeMs){};

std::string NesCompilationTimeEvent::toCSV() const {
    return "CompilationTime " + std::to_string(id) + " : " + std::to_string(size);  // TODO: make csv
}

}
