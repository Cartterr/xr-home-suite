#pragma once

#include <filesystem>
#include <optional>

namespace xrh {

struct AppOptions {
    int runSeconds{30};
    bool enableEnvironmentDepth{true};
    bool enableHandTracking{true};
    int environmentDepthHz{30};
    int handTrackingHz{30};
    std::optional<std::filesystem::path> screenshotDir;
    int screenshotCount{0};
    int screenshotIntervalSeconds{2};
    int screenshotMaxWidth{960};
    std::optional<std::filesystem::path> reportPath;
};

AppOptions parseAppOptions(int argc, char** argv);

}  // namespace xrh
