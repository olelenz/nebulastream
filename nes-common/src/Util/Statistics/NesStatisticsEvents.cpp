#include <Util/Statistics/NesStatisticsEvents.hpp>

namespace NES {

NesBufferAllocateEvent::NesBufferAllocateEvent(int queryId) : id(queryId){};

std::string NesBufferAllocateEvent::toCSV() const {
    return "TEST " + std::to_string(id);
}

}
