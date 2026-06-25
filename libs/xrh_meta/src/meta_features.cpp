#include <xrh/meta/meta_features.h>

#include <xrh/core/checks.h>

#include <iostream>

namespace xrh {

void MetaFeatures::initialize(OpenXrContext& context, const AppOptions& options) {
    if (options.enableHandTracking) {
        createHandTracking(context);
    } else {
        std::cout << "Hand tracking: disabled by command line.\n";
    }
    createPassthrough(context);
    if (options.enableEnvironmentDepth) {
        createEnvironmentDepth(context);
    } else {
        std::cout << "Environment depth/spatial data: disabled by command line.\n";
    }
    probePrivateCameraSources(context);
}

void MetaFeatures::appendPassthroughLayer(std::vector<const XrCompositionLayerBaseHeader*>& layers,
                                          XrCompositionLayerPassthroughFB& layerInfo) const {
    if (!passthroughReady_ || passthroughLayer_ == XR_NULL_HANDLE) {
        return;
    }
    layerInfo.flags = XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT;
    layerInfo.space = XR_NULL_HANDLE;
    layerInfo.layerHandle = passthroughLayer_;
    layers.push_back(reinterpret_cast<const XrCompositionLayerBaseHeader*>(&layerInfo));
}

OverlayState MetaFeatures::overlayState() const {
    OverlayState state{};
    state.passthroughReady = passthroughReady_;
    state.depthLive = environmentDepthFrameCount_ > 0 && XR_SUCCEEDED(lastEnvironmentDepthResult_);
    state.handTrackingReady = handTrackingReady_;
    state.handsLive = activeHandCount_ > 0 && handTrackingFrameCount_ > 0;
    state.privateCameraSourceCount = privateCameraSourceCount_;
    state.activeHandCount = activeHandCount_;
    state.environmentDepthFrameCount = environmentDepthFrameCount_;
    state.handTrackingFrameCount = handTrackingFrameCount_;
    state.lastEnvironmentDepthResult = lastEnvironmentDepthResult_;
    state.handJointLocations = &handJointLocations_;
    state.handActive = &handActive_;
    return state;
}

void MetaFeatures::cleanup() {
    for (auto& handTracker : handTrackers_) {
        if (handTracker != XR_NULL_HANDLE && xrDestroyHandTrackerEXT_) {
            xrDestroyHandTrackerEXT_(handTracker);
            handTracker = XR_NULL_HANDLE;
        }
    }
    handTrackingReady_ = false;

    if (environmentDepthProvider_ != XR_NULL_HANDLE && xrStopEnvironmentDepthProviderMETA_) {
        xrStopEnvironmentDepthProviderMETA_(environmentDepthProvider_);
    }
    if (environmentDepthSwapchain_ != XR_NULL_HANDLE && xrDestroyEnvironmentDepthSwapchainMETA_) {
        xrDestroyEnvironmentDepthSwapchainMETA_(environmentDepthSwapchain_);
        environmentDepthSwapchain_ = XR_NULL_HANDLE;
    }
    if (environmentDepthProvider_ != XR_NULL_HANDLE && xrDestroyEnvironmentDepthProviderMETA_) {
        xrDestroyEnvironmentDepthProviderMETA_(environmentDepthProvider_);
        environmentDepthProvider_ = XR_NULL_HANDLE;
    }

    if (passthroughLayer_ != XR_NULL_HANDLE && xrPassthroughLayerPauseFB_) {
        xrPassthroughLayerPauseFB_(passthroughLayer_);
    }
    if (passthrough_ != XR_NULL_HANDLE && xrPassthroughPauseFB_) {
        xrPassthroughPauseFB_(passthrough_);
    }
    if (passthroughLayer_ != XR_NULL_HANDLE && xrDestroyPassthroughLayerFB_) {
        xrDestroyPassthroughLayerFB_(passthroughLayer_);
        passthroughLayer_ = XR_NULL_HANDLE;
    }
    if (passthrough_ != XR_NULL_HANDLE && xrDestroyPassthroughFB_) {
        xrDestroyPassthroughFB_(passthrough_);
        passthrough_ = XR_NULL_HANDLE;
    }
}

void MetaFeatures::appendReport(ProbeReport& report) const {
    report.passthroughReady = passthroughReady_;
    report.environmentDepthReady = environmentDepthReady_;
    report.environmentDepthWidth = environmentDepthWidth_;
    report.environmentDepthHeight = environmentDepthHeight_;
    report.environmentDepthFrames = environmentDepthFrameCount_;
    report.lastEnvironmentDepthResult = resultString(lastEnvironmentDepthResult_);
    report.lastEnvironmentDepthNearZ = lastEnvironmentDepthNearZ_;
    report.lastEnvironmentDepthFarZ = lastEnvironmentDepthFarZ_;
    report.handTrackingReady = handTrackingReady_;
    report.handTrackingFrames = handTrackingFrameCount_;
    report.activeHandCount = activeHandCount_;
    report.leftValidJoints = handValidJointCounts_[0];
    report.rightValidJoints = handValidJointCounts_[1];
    report.privateCameraSourceCount = privateCameraSourceCount_;
}

}  // namespace xrh
