#include <xrh/core/app_options.h>
#include <xrh/core/probe_report.h>

#include <filesystem>
#include <iostream>
#include <iterator>

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
    xrh::writeProbeReport(path, report);
    const bool exists = std::filesystem::exists(path);
    std::filesystem::remove(path);
    return expect(exists, "report writer created file");
}

}  // namespace

int main() {
    int failures = 0;
    failures += testParseOptions();
    failures += testReportWriter();
    return failures == 0 ? 0 : 1;
}
