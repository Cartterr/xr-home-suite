#include <xrh/core/probe_report.h>

#include <chrono>
#include <cmath>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace xrh {

namespace {

std::string jsonEscape(const std::string& value) {
    std::ostringstream out;
    for (const char c : value) {
        switch (c) {
            case '\\': out << "\\\\"; break;
            case '"': out << "\\\""; break;
            case '\n': out << "\\n"; break;
            case '\r': out << "\\r"; break;
            case '\t': out << "\\t"; break;
            default: out << c; break;
        }
    }
    return out.str();
}

std::string utcNow() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t time = std::chrono::system_clock::to_time_t(now);
    std::tm utc{};
    gmtime_s(&utc, &time);
    std::ostringstream stream;
    stream << std::put_time(&utc, "%Y-%m-%dT%H:%M:%SZ");
    return stream.str();
}

void writeString(std::ofstream& file, const char* name, const std::string& value, bool comma = true) {
    file << "  \"" << name << "\": \"" << jsonEscape(value) << "\"" << (comma ? "," : "") << "\n";
}

void writeBool(std::ofstream& file, const char* name, bool value) {
    file << "  \"" << name << "\": " << (value ? "true" : "false") << ",\n";
}

void writeStringArray(std::ofstream& file, const char* name, const std::vector<std::string>& values, bool comma = true) {
    file << "  \"" << name << "\": [";
    for (size_t i = 0; i < values.size(); ++i) {
        file << (i == 0 ? "" : ", ") << "\"" << jsonEscape(values[i]) << "\"";
    }
    file << "]" << (comma ? "," : "") << "\n";
}

template <typename T>
void writeNumber(std::ofstream& file, const char* name, T value, bool comma = true) {
    file << "  \"" << name << "\": " << value << (comma ? "," : "") << "\n";
}

void writeNumber(std::ofstream& file, const char* name, double value, bool comma = true) {
    file << "  \"" << name << "\": ";
    if (std::isfinite(value)) {
        file << value;
    } else {
        file << "null";
    }
    file << (comma ? "," : "") << "\n";
}

void writeNumber(std::ofstream& file, const char* name, float value, bool comma = true) {
    writeNumber(file, name, static_cast<double>(value), comma);
}

}  // namespace

void writeProbeReport(const std::filesystem::path& path, const ProbeReport& report) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream file(path);
    if (!file) {
        throw std::runtime_error("Could not open probe report path for writing.");
    }

    file << "{\n";
    writeString(file, "generatedAtUtc", utcNow());
    writeNumber(file, "exitCode", report.exitCode);
    writeNumber(file, "elapsedSeconds", report.elapsedSeconds);
    writeNumber(file, "submittedFrames", report.submittedFrames);
    writeNumber(file, "submittedFramesPerSecond", report.submittedFramesPerSecond);
    writeString(file, "runtimeName", report.runtimeName);
    writeString(file, "runtimeVersion", report.runtimeVersion);
    writeString(file, "gpuAdapterName", report.gpuAdapterName);
    writeString(file, "gpuAdapterLuid", report.gpuAdapterLuid);
    writeStringArray(file, "enabledExtensions", report.enabledExtensions);
    writeNumber(file, "eyeSwapchainWidth", report.eyeSwapchainWidth);
    writeNumber(file, "eyeSwapchainHeight", report.eyeSwapchainHeight);
    writeNumber(file, "eyeSwapchainImageCount", report.eyeSwapchainImageCount);
    writeNumber(file, "eyeSwapchainFormat", report.eyeSwapchainFormat);
    writeBool(file, "passthroughSupported", report.passthroughSupported);
    writeBool(file, "passthroughReady", report.passthroughReady);
    writeNumber(file, "passthroughCapabilities", report.passthroughCapabilities);
    writeBool(file, "environmentDepthSupported", report.environmentDepthSupported);
    writeBool(file, "environmentDepthHandRemovalSupported", report.environmentDepthHandRemovalSupported);
    writeBool(file, "environmentDepthReady", report.environmentDepthReady);
    writeNumber(file, "environmentDepthWidth", report.environmentDepthWidth);
    writeNumber(file, "environmentDepthHeight", report.environmentDepthHeight);
    writeNumber(file, "environmentDepthFrames", report.environmentDepthFrames);
    writeNumber(file, "environmentDepthFramesPerSecond", report.environmentDepthFramesPerSecond);
    writeString(file, "lastEnvironmentDepthResult", report.lastEnvironmentDepthResult);
    writeNumber(file, "lastEnvironmentDepthNearZ", report.lastEnvironmentDepthNearZ);
    writeNumber(file, "lastEnvironmentDepthFarZ", report.lastEnvironmentDepthFarZ);
    writeBool(file, "handTrackingSupported", report.handTrackingSupported);
    writeBool(file, "handTrackingReady", report.handTrackingReady);
    writeNumber(file, "handTrackingFrames", report.handTrackingFrames);
    writeNumber(file, "handTrackingFramesPerSecond", report.handTrackingFramesPerSecond);
    writeNumber(file, "activeHandCount", report.activeHandCount);
    writeNumber(file, "leftValidJoints", report.leftValidJoints);
    writeNumber(file, "rightValidJoints", report.rightValidJoints);
    writeNumber(file, "privateCameraSourceCount", report.privateCameraSourceCount);
    writeStringArray(file, "screenshotPaths", report.screenshotPaths, false);
    file << "}\n";
}

}  // namespace xrh
