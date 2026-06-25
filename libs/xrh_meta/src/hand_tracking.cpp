#include <xrh/meta/meta_features.h>

#include <xrh/core/checks.h>

#include <iostream>

namespace xrh {

void MetaFeatures::createHandTracking(OpenXrContext& context) {
    if (!context.hasExtension(XR_EXT_HAND_TRACKING_EXTENSION_NAME) ||
        context.handProperties().supportsHandTracking != XR_TRUE) {
        std::cout << "Hand tracking: not reported as supported over this Link session.\n";
        return;
    }

    xrCreateHandTrackerEXT_ =
        loadXrFunction<PFN_xrCreateHandTrackerEXT>(context.instance(), "xrCreateHandTrackerEXT", true);
    xrDestroyHandTrackerEXT_ =
        loadXrFunction<PFN_xrDestroyHandTrackerEXT>(context.instance(), "xrDestroyHandTrackerEXT", true);
    xrLocateHandJointsEXT_ =
        loadXrFunction<PFN_xrLocateHandJointsEXT>(context.instance(), "xrLocateHandJointsEXT", true);

    createHandTracker(context.session(), 0, XR_HAND_LEFT_EXT, "left");
    createHandTracker(context.session(), 1, XR_HAND_RIGHT_EXT, "right");
    handTrackingReady_ = handTrackers_[0] != XR_NULL_HANDLE || handTrackers_[1] != XR_NULL_HANDLE;

    std::cout << "Hand tracking trackers: "
              << (handTrackers_[0] != XR_NULL_HANDLE ? "left " : "")
              << (handTrackers_[1] != XR_NULL_HANDLE ? "right" : "")
              << (handTrackingReady_ ? "\n" : "none\n");
}

void MetaFeatures::createHandTracker(XrSession session, size_t index, XrHandEXT hand, const char* label) {
    XrHandTrackerCreateInfoEXT createInfo{XR_TYPE_HAND_TRACKER_CREATE_INFO_EXT};
    createInfo.hand = hand;
    createInfo.handJointSet = XR_HAND_JOINT_SET_DEFAULT_EXT;

    const XrResult result = xrCreateHandTrackerEXT_(session, &createInfo, &handTrackers_[index]);
    if (XR_FAILED(result)) {
        handTrackers_[index] = XR_NULL_HANDLE;
        std::cout << "Hand tracking " << label << " tracker create: " << resultString(result) << "\n";
    }
}

void MetaFeatures::locateHands(OpenXrContext& context, XrTime displayTime) {
    activeHandCount_ = 0;
    if (!handTrackingReady_ || !xrLocateHandJointsEXT_) {
        return;
    }

    uint32_t activeHands = 0;
    uint32_t validJoints = 0;
    for (size_t hand = 0; hand < handTrackers_.size(); ++hand) {
        handActive_[hand] = false;
        handValidJointCounts_[hand] = 0;
        if (handTrackers_[hand] == XR_NULL_HANDLE) {
            continue;
        }

        XrHandJointsLocateInfoEXT locateInfo{XR_TYPE_HAND_JOINTS_LOCATE_INFO_EXT};
        locateInfo.baseSpace = context.appSpace();
        locateInfo.time = displayTime;

        XrHandJointLocationsEXT locations{XR_TYPE_HAND_JOINT_LOCATIONS_EXT};
        locations.jointCount = XR_HAND_JOINT_COUNT_EXT;
        locations.jointLocations = handJointLocations_[hand].data();

        lastHandTrackingResults_[hand] =
            xrLocateHandJointsEXT_(handTrackers_[hand], &locateInfo, &locations);
        if (XR_FAILED(lastHandTrackingResults_[hand]) || locations.isActive != XR_TRUE) {
            continue;
        }

        handActive_[hand] = true;
        ++activeHands;
        for (const auto& joint : handJointLocations_[hand]) {
            if ((joint.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT) != 0) {
                ++handValidJointCounts_[hand];
                ++validJoints;
            }
        }
    }

    activeHandCount_ = activeHands;
    if (activeHands > 0 && validJoints > 0) {
        ++handTrackingFrameCount_;
    }
}

}  // namespace xrh
