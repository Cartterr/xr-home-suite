#include <xrh/core/app_options.h>
#include <xrh/core/checks.h>
#include <xrh/core/probe_report.h>
#include <xrh/core/shutdown_signal.h>
#include <xrh/d3d11/d3d11_renderer.h>
#include <xrh/meta/meta_features.h>
#include <xrh/openxr/openxr_context.h>

#include <chrono>
#include <iostream>
#include <thread>
#include <utility>
#include <vector>

namespace xrh {

namespace {

bool shouldPollSensor(XrTime displayTime, XrTime& lastPollTime, int hz) {
    if (hz <= 0) {
        return false;
    }
    constexpr XrTime kNanosecondsPerSecond = 1000000000;
    const XrTime interval = kNanosecondsPerSecond / hz;
    if (lastPollTime == 0 || displayTime - lastPollTime >= interval) {
        lastPollTime = displayTime;
        return true;
    }
    return false;
}

class OpenXrProbeApp {
public:
    explicit OpenXrProbeApp(AppOptions options) : options_(std::move(options)) {}

    int run() {
        int exitCode = 0;
        try {
            if (!shutdownSignal_.install()) {
                std::cout << "Warning: failed to install Ctrl+C shutdown handler.\n";
            }
            initialize();
            mainLoop();
            cleanup();
            std::cout << "OpenXR shutdown complete.\n";
        } catch (const std::exception& error) {
            exitCode = 1;
            std::cerr << "Fatal: " << error.what() << "\n";
            cleanup();
        }
        shutdownSignal_.uninstall();
        writeReportIfRequested(exitCode);
        return exitCode;
    }

private:
    void initialize() {
        context_.initialize();
        renderer_.initializeDevice(context_);
        context_.createD3D11Session(renderer_.device());
        context_.createAppSpace();
        if (options_.screenshotDir && options_.screenshotCount > 0) {
            renderer_.configureScreenshotCapture(ScreenshotCaptureConfig{
                *options_.screenshotDir,
                static_cast<uint32_t>(options_.screenshotCount),
                static_cast<double>(options_.screenshotIntervalSeconds),
                static_cast<uint32_t>(options_.screenshotMaxWidth),
            });
        }
        renderer_.createRenderResources(context_);
        meta_.initialize(context_, options_);
    }

    void observeShutdownRequest() {
        if (!shutdownSignal_.requested()) {
            return;
        }
        context_.requestExit();
        if (!shutdownNoticePrinted_) {
            std::cout << "\nCtrl+C received. Finishing the current OpenXR frame and shutting down cleanly.\n";
            shutdownNoticePrinted_ = true;
        }
    }

    void mainLoop() {
        startTime_ = std::chrono::steady_clock::now();
        std::cout << "\nPut the headset in Quest Link now. Running for " << options_.runSeconds
                  << "s. You should see live passthrough with a GPU-rendered overlay.\n";
        std::cout << "Sensor polling caps: depth="
                  << (options_.enableEnvironmentDepth ? std::to_string(options_.environmentDepthHz) + " Hz" : "off")
                  << " hands="
                  << (options_.enableHandTracking ? std::to_string(options_.handTrackingHz) + " Hz" : "off")
                  << "\n";
        if (options_.screenshotDir && options_.screenshotCount > 0) {
            std::cout << "Overlay capture: count=" << options_.screenshotCount
                      << " every=" << options_.screenshotIntervalSeconds
                      << "s width=" << options_.screenshotMaxWidth
                      << " dir=" << options_.screenshotDir->string() << "\n";
        }

        while (!context_.exitRequested()) {
            observeShutdownRequest();
            if (context_.exitRequested()) {
                break;
            }
            context_.pollEvents();
            observeShutdownRequest();

            const auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::steady_clock::now() - startTime_).count();
            if (elapsed >= options_.runSeconds) {
                context_.requestExit();
            }

            if (!context_.sessionRunning()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                continue;
            }

            XrFrameState frameState{XR_TYPE_FRAME_STATE};
            checkXr(xrWaitFrame(context_.session(), nullptr, &frameState), "xrWaitFrame");
            checkXr(xrBeginFrame(context_.session(), nullptr), "xrBeginFrame");
            observeShutdownRequest();

            std::vector<const XrCompositionLayerBaseHeader*> layers;
            XrCompositionLayerPassthroughFB passthroughLayerInfo{XR_TYPE_COMPOSITION_LAYER_PASSTHROUGH_FB};
            XrCompositionLayerProjection projectionLayer{XR_TYPE_COMPOSITION_LAYER_PROJECTION};

            if (!context_.exitRequested() && frameState.shouldRender == XR_TRUE) {
                renderFrame(frameState, layers, passthroughLayerInfo, projectionLayer);
            }

            XrFrameEndInfo endInfo{XR_TYPE_FRAME_END_INFO};
            endInfo.displayTime = frameState.predictedDisplayTime;
            endInfo.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
            endInfo.layerCount = static_cast<uint32_t>(layers.size());
            endInfo.layers = layers.empty() ? nullptr : layers.data();
            checkXr(xrEndFrame(context_.session(), &endInfo), "xrEndFrame");

            ++frameIndex_;
            if (frameIndex_ % 120 == 0) {
                const OverlayState state = meta_.overlayState();
                std::cout << "frames=" << frameIndex_
                          << " depthFrames=" << state.environmentDepthFrameCount
                          << " handFrames=" << state.handTrackingFrameCount
                          << " activeHands=" << state.activeHandCount
                          << " lastDepth=" << resultString(state.lastEnvironmentDepthResult)
                          << "\n";
            }
        }
    }

    void renderFrame(const XrFrameState& frameState,
                     std::vector<const XrCompositionLayerBaseHeader*>& layers,
                     XrCompositionLayerPassthroughFB& passthroughLayerInfo,
                     XrCompositionLayerProjection& projectionLayer) {
        if (!renderer_.locateViews(context_, frameState.predictedDisplayTime)) {
            return;
        }
        if (shouldPollSensor(frameState.predictedDisplayTime, lastEnvironmentDepthPollTime_, options_.environmentDepthHz)) {
            meta_.acquireEnvironmentDepth(context_, frameState.predictedDisplayTime);
        }
        if (shouldPollSensor(frameState.predictedDisplayTime, lastHandTrackingPollTime_, options_.handTrackingHz)) {
            meta_.locateHands(context_, frameState.predictedDisplayTime);
        }

        renderer_.renderEyes(meta_.overlayState(), startTime_, frameIndex_);
        meta_.appendPassthroughLayer(layers, passthroughLayerInfo);

        const auto& projectionViews = renderer_.projectionViews();
        projectionLayer.layerFlags = XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT;
        projectionLayer.space = context_.appSpace();
        projectionLayer.viewCount = static_cast<uint32_t>(projectionViews.size());
        projectionLayer.views = projectionViews.data();
        layers.push_back(reinterpret_cast<const XrCompositionLayerBaseHeader*>(&projectionLayer));
    }

    void cleanup() {
        meta_.cleanup();
        renderer_.cleanup();
        context_.cleanup();
    }

    void writeReportIfRequested(int exitCode) {
        if (!options_.reportPath) {
            return;
        }
        ProbeReport report{};
        report.exitCode = exitCode;
        report.submittedFrames = frameIndex_;
        report.elapsedSeconds = elapsedSeconds();
        context_.appendReport(report);
        renderer_.appendReport(report);
        meta_.appendReport(report);
        if (report.elapsedSeconds > 0.0) {
            report.submittedFramesPerSecond =
                static_cast<double>(report.submittedFrames) / report.elapsedSeconds;
            report.environmentDepthFramesPerSecond =
                static_cast<double>(report.environmentDepthFrames) / report.elapsedSeconds;
            report.handTrackingFramesPerSecond =
                static_cast<double>(report.handTrackingFrames) / report.elapsedSeconds;
        }
        writeProbeReport(*options_.reportPath, report);
        std::cout << "Probe report: " << options_.reportPath->string() << "\n";
    }

    double elapsedSeconds() const {
        if (startTime_ == std::chrono::steady_clock::time_point{}) {
            return 0.0;
        }
        return std::chrono::duration<double>(std::chrono::steady_clock::now() - startTime_).count();
    }

    AppOptions options_{};
    ShutdownSignal shutdownSignal_{};
    OpenXrContext context_{};
    D3D11Renderer renderer_{};
    MetaFeatures meta_{};
    XrTime lastEnvironmentDepthPollTime_{0};
    XrTime lastHandTrackingPollTime_{0};
    uint64_t frameIndex_{0};
    bool shutdownNoticePrinted_{false};
    std::chrono::steady_clock::time_point startTime_{};
};

}  // namespace

}  // namespace xrh

int main(int argc, char** argv) {
    xrh::OpenXrProbeApp app(xrh::parseAppOptions(argc, argv));
    return app.run();
}
