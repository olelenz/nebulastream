#include "StatsSource.hpp"
#include <chrono>
#include <cstddef>
#include <exception>
#include <iostream>
#include <string>
#include <thread>
#include <unordered_map>
#include <Configurations/Descriptor.hpp>
#include <Runtime/AbstractBufferProvider.hpp>
#include <Runtime/TupleBuffer.hpp>
#include <Sources/Source.hpp>
#include <Sources/SourceDescriptor.hpp>
#include <Util/Logger/Logger.hpp>
#include <FixedGeneratorRate.hpp>
#include <Generator.hpp>
#include <GeneratorRate.hpp>
#include <SinusGeneratorRate.hpp>
#include <SourceRegistry.hpp>
#include <SourceValidationRegistry.hpp>
#include <StatsSource.hpp>

#include "Util/Statistics/NesStatistics.hpp"

namespace NES
{
StatsSource::StatsSource(const SourceDescriptor& sourceDescriptor)
: pollIntervalMs(sourceDescriptor.getFromConfig(ConfigParametersStats::POLL_INTERVAL_MS))
, maxRuntime(sourceDescriptor.getFromConfig(ConfigParametersStats::MAX_RUNTIME_MS))
{
    NES_TRACE("Init StatsSource with poll interval of {} ms.", pollIntervalMs);
}

void StatsSource::open(std::shared_ptr<AbstractBufferProvider>)
{
    this->lastSequenceNumber = 0;
    this->leftoverData.clear();
    this->startTime = std::chrono::system_clock::now();
    NES_TRACE("Opening StatsSource.");
}

void StatsSource::close()
{
    NES_TRACE("Closing StatsSource.");
}

Source::FillTupleBufferResult StatsSource::fillTupleBuffer(TupleBuffer& tupleBuffer, const std::stop_token& stopToken)
{
    try
    {
        const auto elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - startTime).count();
        if (maxRuntime >= 0 && elapsedTime >= maxRuntime)
        {
            NES_INFO("Reached max runtime! Stopping StatsSource");
            return FillTupleBufferResult::eos();
        }
        size_t bufferSize = tupleBuffer.getBufferSize();
        char* bufferStart = tupleBuffer.getAvailableMemoryArea<char>().data();
        size_t writtenBytes = 0;

        // check if there is still data left to process before getting new
        if (!leftoverData.empty())
        {
            if (leftoverData.size() <= bufferSize)
            {
                std::memcpy(bufferStart, leftoverData.data(), leftoverData.size());
                writtenBytes = leftoverData.size();
                leftoverData.clear();
            }
            else
            {
                std::memcpy(bufferStart, leftoverData.data(), bufferSize);
                leftoverData = leftoverData.substr(bufferSize);
                return FillTupleBufferResult::withBytes(bufferSize);
            }
        }

        auto& stats = NesStatistics::getInstance();
        std::vector<RawEventData> newEvents;
        while (newEvents.empty() && !stopToken.stop_requested())
        {
            if (maxRuntime >= 0)
            {
                auto currentElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - startTime).count();
                if (currentElapsed >= maxRuntime)
                {
                    NES_INFO("Reached max runtime during polling! Stopping StatsSource");
                    if (writtenBytes > 0) return FillTupleBufferResult::withBytes(writtenBytes);
                    return FillTupleBufferResult::eos();
                }
            }
            newEvents = stats.getEventsSince(lastSequenceNumber);
            if (newEvents.empty())
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(pollIntervalMs));
            }
        }

        if (stopToken.stop_requested() && newEvents.empty())
        {
            if (writtenBytes > 0) return FillTupleBufferResult::withBytes(writtenBytes);
            return FillTupleBufferResult::eos();
        }

        std::ostringstream ss;
        for (const auto& event : newEvents)
        {
            ss  << event.seq << ","
                << event.ts << ","
                << event.queryId << ","
                << event.metricValue << ","
                << event.eventType << "\n";
            if (event.seq >= lastSequenceNumber)
            {
                lastSequenceNumber = event.seq + 1;
            }
        }

        std::string csvData = ss.str();
        size_t rem_space = bufferSize - writtenBytes;

        if (csvData.size() <= rem_space)
        {
            std::memcpy(bufferStart + writtenBytes, csvData.data(), csvData.size());
            writtenBytes += csvData.size();
        }
        else
        {
            std::memcpy(bufferStart + writtenBytes, csvData.data(), rem_space);
            leftoverData = csvData.substr(rem_space);
            writtenBytes = bufferSize;
        }

        return FillTupleBufferResult::withBytes(writtenBytes);
    }
    catch (const std::exception& ex)
    {
        NES_ERROR("Failed to fill the TupleBuffer. Error: {}", ex.what());
        throw;
    }
}

std::ostream& StatsSource::toString(std::ostream& str) const
{
    str << "\nStatsSource(";
    str << "\n\tpollIntervalMs: " << this->pollIntervalMs;
    str << "\n\tlastSequenceNumber: " << this->lastSequenceNumber;
    str << ")\n";
    return str;
}

DescriptorConfig::Config StatsSource::validateAndFormat(std::unordered_map<std::string, std::string> config)
{
    return DescriptorConfig::validateAndFormat<ConfigParametersStats>(std::move(config), NAME);
}

SourceValidationRegistryReturnType
///NOLINTNEXTLINE (performance-unnecessary-value-param)
RegisterStatsSourceValidation(SourceValidationRegistryArguments sourceConfig)
{
    return StatsSource::validateAndFormat(sourceConfig.config);
}

///NOLINTNEXTLINE (performance-unnecessary-value-param)
SourceRegistryReturnType SourceGeneratedRegistrar::RegisterStatsSource(SourceRegistryArguments sourceRegistryArguments)
{
    return std::make_unique<StatsSource>(sourceRegistryArguments.sourceDescriptor);
}

}
