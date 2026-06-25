#pragma once

#include <xrh/core/app_options.h>
#include <xrh/core/openxr_platform.h>
#include <xrh/core/overlay_state.h>
#include <xrh/core/probe_report.h>
#include <xrh/openxr/openxr_context.h>

#include <array>
#include <vector>

namespace xrh {

using PFN_xrEnumeratePassthroughCameraSourcePropertiesMETAX1 =
    XrResult(XRAPI_PTR*)(XrSession session,
                         uint32_t cameraSourceCapacityInput,
                         uint32_t* cameraSourceCountOutput,
                         void* cameraSourceProperties);

class MetaFeatures {
public:
    void initialize(OpenXrContext& context, const AppOptions& options);
    void acquireEnvironmentDepth(OpenXrContext& context, XrTime displayTime);
    void locateHands(OpenXrContext& context, XrTime displayTime);
    void appendPassthroughLayer(std::vector<const XrCompositionLayerBaseHeader*>& layers,
                                XrCompositionLayerPassthroughFB& layerInfo) const;
    OverlayState overlayState() const;
    void cleanup();
    void appendReport(ProbeReport& report) const;

private:
    void createPassthrough(OpenXrContext& context);
    void createEnvironmentDepth(OpenXrContext& context);
    void createHandTracking(OpenXrContext& context);
    void createHandTracker(XrSession session, size_t index, XrHandEXT hand, const char* label);
    void probePrivateCameraSources(OpenXrContext& context);

    PFN_xrCreatePassthroughFB xrCreatePassthroughFB_{nullptr};
    PFN_xrDestroyPassthroughFB xrDestroyPassthroughFB_{nullptr};
    PFN_xrPassthroughStartFB xrPassthroughStartFB_{nullptr};
    PFN_xrPassthroughPauseFB xrPassthroughPauseFB_{nullptr};
    PFN_xrCreatePassthroughLayerFB xrCreatePassthroughLayerFB_{nullptr};
    PFN_xrDestroyPassthroughLayerFB xrDestroyPassthroughLayerFB_{nullptr};
    PFN_xrPassthroughLayerResumeFB xrPassthroughLayerResumeFB_{nullptr};
    PFN_xrPassthroughLayerPauseFB xrPassthroughLayerPauseFB_{nullptr};
    PFN_xrPassthroughLayerSetStyleFB xrPassthroughLayerSetStyleFB_{nullptr};
    XrPassthroughFB passthrough_{XR_NULL_HANDLE};
    XrPassthroughLayerFB passthroughLayer_{XR_NULL_HANDLE};
    bool passthroughReady_{false};

    PFN_xrCreateEnvironmentDepthProviderMETA xrCreateEnvironmentDepthProviderMETA_{nullptr};
    PFN_xrDestroyEnvironmentDepthProviderMETA xrDestroyEnvironmentDepthProviderMETA_{nullptr};
    PFN_xrStartEnvironmentDepthProviderMETA xrStartEnvironmentDepthProviderMETA_{nullptr};
    PFN_xrStopEnvironmentDepthProviderMETA xrStopEnvironmentDepthProviderMETA_{nullptr};
    PFN_xrCreateEnvironmentDepthSwapchainMETA xrCreateEnvironmentDepthSwapchainMETA_{nullptr};
    PFN_xrDestroyEnvironmentDepthSwapchainMETA xrDestroyEnvironmentDepthSwapchainMETA_{nullptr};
    PFN_xrEnumerateEnvironmentDepthSwapchainImagesMETA xrEnumerateEnvironmentDepthSwapchainImagesMETA_{nullptr};
    PFN_xrGetEnvironmentDepthSwapchainStateMETA xrGetEnvironmentDepthSwapchainStateMETA_{nullptr};
    PFN_xrAcquireEnvironmentDepthImageMETA xrAcquireEnvironmentDepthImageMETA_{nullptr};
    PFN_xrSetEnvironmentDepthHandRemovalMETA xrSetEnvironmentDepthHandRemovalMETA_{nullptr};
    XrEnvironmentDepthProviderMETA environmentDepthProvider_{XR_NULL_HANDLE};
    XrEnvironmentDepthSwapchainMETA environmentDepthSwapchain_{XR_NULL_HANDLE};
    std::vector<XrSwapchainImageD3D11KHR> environmentDepthImages_;
    bool environmentDepthReady_{false};
    uint32_t environmentDepthWidth_{0};
    uint32_t environmentDepthHeight_{0};
    uint64_t environmentDepthFrameCount_{0};
    XrResult lastEnvironmentDepthResult_{XR_SUCCESS};
    float lastEnvironmentDepthNearZ_{0.0f};
    float lastEnvironmentDepthFarZ_{0.0f};

    PFN_xrCreateHandTrackerEXT xrCreateHandTrackerEXT_{nullptr};
    PFN_xrDestroyHandTrackerEXT xrDestroyHandTrackerEXT_{nullptr};
    PFN_xrLocateHandJointsEXT xrLocateHandJointsEXT_{nullptr};
    std::array<XrHandTrackerEXT, 2> handTrackers_{XR_NULL_HANDLE, XR_NULL_HANDLE};
    std::array<std::array<XrHandJointLocationEXT, XR_HAND_JOINT_COUNT_EXT>, 2> handJointLocations_{};
    std::array<XrResult, 2> lastHandTrackingResults_{XR_SUCCESS, XR_SUCCESS};
    std::array<uint32_t, 2> handValidJointCounts_{0, 0};
    std::array<bool, 2> handActive_{false, false};
    bool handTrackingReady_{false};
    uint32_t activeHandCount_{0};
    uint64_t handTrackingFrameCount_{0};

    uint32_t privateCameraSourceCount_{0};
};

}  // namespace xrh
