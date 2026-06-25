#pragma once

#include <xrh/core/openxr_platform.h>

#include <array>
#include <cstdint>

namespace xrh {

struct OverlayState {
    bool passthroughReady{false};
    bool depthLive{false};
    bool handTrackingReady{false};
    bool handsLive{false};
    uint32_t privateCameraSourceCount{0};
    uint32_t activeHandCount{0};
    uint64_t environmentDepthFrameCount{0};
    uint64_t handTrackingFrameCount{0};
    XrResult lastEnvironmentDepthResult{XR_SUCCESS};
    const std::array<std::array<XrHandJointLocationEXT, XR_HAND_JOINT_COUNT_EXT>, 2>* handJointLocations{nullptr};
    const std::array<bool, 2>* handActive{nullptr};
};

}  // namespace xrh
