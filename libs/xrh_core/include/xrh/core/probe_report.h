#pragma once

#include <xrh/core/app_options.h>

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace xrh {

struct ProbeReport {
    int exitCode{0};
    uint64_t submittedFrames{0};
    std::string runtimeName;
    std::string runtimeVersion;
    std::string gpuAdapterName;
    std::string gpuAdapterLuid;
    std::vector<std::string> enabledExtensions;
    bool passthroughSupported{false};
    bool passthroughReady{false};
    uint64_t passthroughCapabilities{0};
    bool environmentDepthSupported{false};
    bool environmentDepthHandRemovalSupported{false};
    bool environmentDepthReady{false};
    uint32_t environmentDepthWidth{0};
    uint32_t environmentDepthHeight{0};
    uint64_t environmentDepthFrames{0};
    std::string lastEnvironmentDepthResult;
    float lastEnvironmentDepthNearZ{0.0f};
    float lastEnvironmentDepthFarZ{0.0f};
    bool handTrackingSupported{false};
    bool handTrackingReady{false};
    uint64_t handTrackingFrames{0};
    uint32_t activeHandCount{0};
    uint32_t leftValidJoints{0};
    uint32_t rightValidJoints{0};
    uint32_t privateCameraSourceCount{0};
};

void writeProbeReport(const std::filesystem::path& path, const ProbeReport& report);

}  // namespace xrh
