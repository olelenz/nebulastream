/*
    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        https://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#pragma once

#include <Listeners/StatisticListener.hpp>
#include <Util/Overloaded.hpp>
#include <fstream>
#include <iostream>
#include <variant>
#include <mutex>
#include <filesystem>

namespace NES {

class FileStatisticListener : public StatisticListener {
public:
    explicit FileStatisticListener(const std::string& filePath) {
        std::lock_guard<std::mutex> lock(globalMutex);

        file.open(filePath, std::ios::app);
        if (!file.is_open()) {
            std::cerr << "Could not open statistics file: " << filePath << std::endl;
        } else {
            if (std::filesystem::file_size(filePath) == 0) {
                file << "EventType, QueryId, SecondaryId, DurationOrLatencyUs\n";
                file.flush();
            }
        }
    }

    void onEvent(Event event) override {
        std::lock_guard<std::mutex> lock(globalMutex);
        std::visit(Overloaded{
            [&](const BufferAcquisitionLatency& e) {
                file << "BufferAcquisitionLatency, " << e.queryId << ", " << e.originId << ", " << e.latency.count() << "\n";
            },
            [&](const PipelineExecutionDuration& e) {
                file << "PipelineExecutionDuration, " << e.queryId << ", " << e.pipelineId << ", " << e.duration.count() << "\n";
            },
            [&](const auto&) {
                // Ignore other events for now
            }
        }, event);
        file.flush();
    }

    void onEvent(SystemEvent) override {
        // Ignore system events for now
    }

private:
    std::ofstream file;
    static inline std::mutex globalMutex;
};

}
