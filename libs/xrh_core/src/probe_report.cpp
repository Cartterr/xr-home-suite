#include <xrh/core/probe_report.h>

#include <chrono>
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

template <typename T>
void writeNumber(std::ofstream& file, const char* name, T value, bool comma = true) {
    file << "  \"" << name << "\": " << value << (comma ? "," : "") << "\n";
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
    writeNumber(file, "submittedFrames", report.submittedFrames);
    writeString(file, "runtimeName", report.runtimeName);
    writeString(file, "runtimeVersion", report.runtimeVersion);
    writeString(file, "gpuAdapterName", report.gpuAdapterName);
    writeString(file, "gpuAdapterLuid", report.gpuAdapterLuid);
    file << "  \"enabledExtensions\": [";
    for (size_t i = 0; i < report.enabledExtensions.size(); ++i) {
        file << (i == 0 ? "" : ", ") << "\"" << jsonEscape(report.enabledExtensions[i]) << "\"";
    }
    file << "],\n";
    writeBool(file, "passthroughSupported", report.passthroughSupported);
    writeBool(file, "passthroughReady", report.passthroughReady);
    writeNumber(file, "passthroughCapabilities", report.passthroughCapabilities);
    writeBool(file, "environmentDepthSupported", report.environmentDepthSupported);
    writeBool(file, "environmentDepthHandRemovalSupported", report.environmentDepthHandRemovalSupported);
    writeBool(file, "environmentDepthReady", report.environmentDepthReady);
    writeNumber(file, "environmentDepthWidth", report.environmentDepthWidth);
    writeNumber(file, "environmentDepthHeight", report.environmentDepthHeight);
    writeNumber(file, "environmentDepthFrames", report.environmentDepthFrames);
    writeString(file, "lastEnvironmentDepthResult", report.lastEnvironmentDepthResult);
    writeNumber(file, "lastEnvironmentDepthNearZ", report.lastEnvironmentDepthNearZ);
    writeNumber(file, "lastEnvironmentDepthFarZ", report.lastEnvironmentDepthFarZ);
    writeBool(file, "handTrackingSupported", report.handTrackingSupported);
    writeBool(file, "handTrackingReady", report.handTrackingReady);
    writeNumber(file, "handTrackingFrames", report.handTrackingFrames);
    writeNumber(file, "activeHandCount", report.activeHandCount);
    writeNumber(file, "leftValidJoints", report.leftValidJoints);
    writeNumber(file, "rightValidJoints", report.rightValidJoints);
    writeNumber(file, "privateCameraSourceCount", report.privateCameraSourceCount, false);
    file << "}\n";
}

}  // namespace xrh
