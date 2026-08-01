#pragma once

#include <chrono>
#include <cstdint>
#include <optional>

namespace NES
{

struct QueryResourceSnapshot
{
    std::chrono::system_clock::time_point timestamp;
    uint64_t cpuTimeMicros;
    uint64_t residentMemoryKb;
};

std::optional<QueryResourceSnapshot> collectProcessResourceSnapshot(std::chrono::system_clock::time_point timestamp);

}
