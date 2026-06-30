#include <Util/Statistics/NesResourceUsage.hpp>

#include <fstream>
#include <limits>
#include <string>
#include <sys/resource.h>

namespace NES
{
namespace
{

std::optional<uint64_t> getProcessCpuTimeMicros()
{
    rusage usage{};
    if (getrusage(RUSAGE_SELF, &usage) != 0)
    {
        return std::nullopt;
    }

    const auto toMicros = [](const timeval& time)
    {
        return static_cast<uint64_t>(time.tv_sec) * 1'000'000ULL + static_cast<uint64_t>(time.tv_usec);
    };
    return toMicros(usage.ru_utime) + toMicros(usage.ru_stime);
}

std::optional<uint64_t> getProcessResidentMemoryKb()
{
    std::ifstream statusFile("/proc/self/status");
    if (!statusFile.is_open())
    {
        return std::nullopt;
    }

    std::string key;
    while (statusFile >> key)
    {
        if (key == "VmRSS:")
        {
            uint64_t residentMemoryKb = 0;
            std::string unit;
            if (statusFile >> residentMemoryKb >> unit)
            {
                return residentMemoryKb;
            }
            return std::nullopt;
        }
        statusFile.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
    return std::nullopt;
}

}

std::optional<QueryResourceSnapshot> collectProcessResourceSnapshot(std::chrono::system_clock::time_point timestamp)
{
    const auto cpuTimeMicros = getProcessCpuTimeMicros();
    const auto residentMemoryKb = getProcessResidentMemoryKb();
    if (!cpuTimeMicros || !residentMemoryKb)
    {
        return std::nullopt;
    }
    return QueryResourceSnapshot{
        .timestamp = timestamp,
        .cpuTimeMicros = *cpuTimeMicros,
        .residentMemoryKb = *residentMemoryKb,
    };
}

}
