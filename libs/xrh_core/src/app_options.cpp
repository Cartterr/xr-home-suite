#include <xrh/core/app_options.h>

#include <cstdlib>
#include <cstring>

namespace xrh {

namespace {

int parsePositiveInt(const char* value, int fallback) {
    const int parsed = std::atoi(value);
    return parsed > 0 ? parsed : fallback;
}

}  // namespace

AppOptions parseAppOptions(int argc, char** argv) {
    AppOptions options{};
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--seconds") == 0 && i + 1 < argc) {
            options.runSeconds = parsePositiveInt(argv[++i], options.runSeconds);
        } else if (std::strcmp(argv[i], "--no-depth") == 0) {
            options.enableEnvironmentDepth = false;
            options.environmentDepthHz = 0;
        } else if (std::strcmp(argv[i], "--no-hands") == 0) {
            options.enableHandTracking = false;
            options.handTrackingHz = 0;
        } else if (std::strcmp(argv[i], "--depth-hz") == 0 && i + 1 < argc) {
            options.environmentDepthHz = parsePositiveInt(argv[++i], options.environmentDepthHz);
            options.enableEnvironmentDepth = true;
        } else if (std::strcmp(argv[i], "--hand-hz") == 0 && i + 1 < argc) {
            options.handTrackingHz = parsePositiveInt(argv[++i], options.handTrackingHz);
            options.enableHandTracking = true;
        } else if (std::strcmp(argv[i], "--capture-dir") == 0 && i + 1 < argc) {
            options.screenshotDir = std::filesystem::path(argv[++i]);
        } else if (std::strcmp(argv[i], "--capture-count") == 0 && i + 1 < argc) {
            options.screenshotCount = parsePositiveInt(argv[++i], options.screenshotCount);
        } else if (std::strcmp(argv[i], "--capture-every") == 0 && i + 1 < argc) {
            options.screenshotIntervalSeconds = parsePositiveInt(argv[++i], options.screenshotIntervalSeconds);
        } else if (std::strcmp(argv[i], "--capture-width") == 0 && i + 1 < argc) {
            options.screenshotMaxWidth = parsePositiveInt(argv[++i], options.screenshotMaxWidth);
        } else if (std::strcmp(argv[i], "--report") == 0 && i + 1 < argc) {
            options.reportPath = std::filesystem::path(argv[++i]);
        }
    }
    return options;
}

}  // namespace xrh
