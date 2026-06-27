#include <xrh/core/app_options.h>
#include <xrh/core/probe_report.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <string>

namespace {

int expect(bool value, const char* label) {
    if (value) {
        return 0;
    }
    std::cerr << "FAILED: " << label << "\n";
    return 1;
}

int testParseOptions() {
    const char* argv[] = {
        "openxr_probe",
        "--seconds",
        "7",
        "--no-depth",
        "--hand-hz",
        "15",
        "--capture-dir",
        "C:/XRHomeSuite/artifacts/captures/test",
        "--capture-count",
        "2",
        "--capture-every",
        "3",
        "--capture-width",
        "640",
        "--report",
        "C:/XRHomeSuite/artifacts/reports/test.json",
    };
    xrh::AppOptions options = xrh::parseAppOptions(static_cast<int>(std::size(argv)), const_cast<char**>(argv));
    int failures = 0;
    failures += expect(options.runSeconds == 7, "seconds");
    failures += expect(!options.enableEnvironmentDepth, "no-depth");
    failures += expect(options.environmentDepthHz == 0, "depth-hz disabled");
    failures += expect(options.enableHandTracking, "hand tracking enabled");
    failures += expect(options.handTrackingHz == 15, "hand-hz");
    failures += expect(options.screenshotDir.has_value(), "capture dir exists");
    failures += expect(options.screenshotCount == 2, "capture count");
    failures += expect(options.screenshotIntervalSeconds == 3, "capture every");
    failures += expect(options.screenshotMaxWidth == 640, "capture width");
    failures += expect(options.reportPath.has_value(), "report path exists");
    return failures;
}

int testReportWriter() {
    const auto path = std::filesystem::temp_directory_path() / "xrh-core-test-report.json";
    xrh::ProbeReport report{};
    report.exitCode = 0;
    report.runtimeName = "TestRuntime";
    report.runtimeVersion = "1.2.3";
    report.enabledExtensions = {"XR_TEST_one", "XR_TEST_two"};
    report.eyeSwapchainWidth = 640;
    report.eyeSwapchainHeight = 480;
    report.screenshotPaths = {"C:/XRHomeSuite/artifacts/captures/test/frame.bmp"};
    report.lastEnvironmentDepthFarZ = std::numeric_limits<float>::infinity();
    xrh::writeProbeReport(path, report);
    const bool exists = std::filesystem::exists(path);
    std::ifstream input(path);
    const std::string text((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    input.close();
    std::filesystem::remove(path);
    int failures = 0;
    failures += expect(exists, "report writer created file");
    failures += expect(text.find("\"lastEnvironmentDepthFarZ\": null") != std::string::npos,
                       "report writer emits valid JSON for infinity");
    failures += expect(text.find("\"screenshotPaths\": [\"C:/XRHomeSuite/artifacts/captures/test/frame.bmp\"]") !=
                           std::string::npos,
                       "report writer emits screenshot paths");
    return failures;
}

}  // namespace

int main() {
    int failures = 0;
    failures += testParseOptions();
    failures += testReportWriter();
    return failures == 0 ? 0 : 1;
}
