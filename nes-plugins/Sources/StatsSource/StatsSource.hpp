#pragma once


#include <chrono>
#include <cstddef>
#include <cstdint>
#include <ostream>
#include <string>
#include <string_view>
#include <unordered_map>

#include <Configurations/Descriptor.hpp>
#include <Configurations/Enums/EnumWrapper.hpp>
#include <Runtime/AbstractBufferProvider.hpp>
#include <Runtime/TupleBuffer.hpp>
#include <Sources/Source.hpp>
#include <Sources/SourceDescriptor.hpp>
#include <Util/Logger/Logger.hpp>
#include <Util/Strings.hpp>
#include <ErrorHandling.hpp>
#include <FixedGeneratorRate.hpp>
#include <Generator.hpp>
#include <GeneratorFields.hpp>
#include <GeneratorRate.hpp>
#include <SinusGeneratorRate.hpp>

namespace NES
{
class StatsSource : public Source
{
public:
    constexpr static std::string_view NAME = "Stats";

    explicit StatsSource(const SourceDescriptor& descriptor);
    ~StatsSource() override = default;

    StatsSource(const StatsSource&) = delete;
    StatsSource& operator=(const StatsSource&) = delete;
    StatsSource(StatsSource&&) = delete;
    StatsSource& operator=(StatsSource&&) = delete;

    FillTupleBufferResult fillTupleBuffer(TupleBuffer& tupleBuffer, const std::stop_token& stopToken) override;
    [[nodiscard]] std::ostream& toString(std::ostream& str) const override;

    void open(std::shared_ptr<AbstractBufferProvider> buffer_provider) override;
    void close() override;

    static DescriptorConfig::Config validateAndFormat(std::unordered_map<std::string, std::string> config);

private:
    uint64_t pollIntervalMs;
    uint64_t lastSequenceNumber{0};
    std::string leftoverData;
    int32_t maxRuntime;
    std::chrono::time_point<std::chrono::system_clock> startTime;
};

struct ConfigParametersStats
{
    static inline const DescriptorConfig::ConfigParameter<uint64_t> POLL_INTERVAL_MS{
        "poll_interval_ms",
        50,
        [](const std::unordered_map<std::string, std::string>& config)
        {
            return DescriptorConfig::tryGet(POLL_INTERVAL_MS, config);
        }
    };
    static inline const DescriptorConfig::ConfigParameter<int32_t> MAX_RUNTIME_MS{
        "max_runtime_ms",
        -1,
        [](const std::unordered_map<std::string, std::string>& config) { return DescriptorConfig::tryGet(MAX_RUNTIME_MS, config); }};
    static inline std::unordered_map<std::string, DescriptorConfig::ConfigParameterContainer> parameterMap
        = DescriptorConfig::createConfigParameterContainerMap(
            SourceDescriptor::parameterMap, POLL_INTERVAL_MS, MAX_RUNTIME_MS
        );
};
}
