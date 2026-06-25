#include <xrh/meta/meta_features.h>

#include <xrh/core/checks.h>

#include <iostream>

namespace xrh {

void MetaFeatures::probePrivateCameraSources(OpenXrContext& context) {
    if (!context.hasExtension("XR_METAX1_passthrough_camera_data")) {
        return;
    }

    auto enumerateCameraSources =
        loadXrFunction<PFN_xrEnumeratePassthroughCameraSourcePropertiesMETAX1>(
            context.instance(), "xrEnumeratePassthroughCameraSourcePropertiesMETAX1", false);
    if (!enumerateCameraSources) {
        return;
    }

    uint32_t count = 0;
    const XrResult result = enumerateCameraSources(context.session(), 0, &count, nullptr);
    if (XR_SUCCEEDED(result)) {
        privateCameraSourceCount_ = count;
    }
    std::cout << "Private Link camera-source count probe: " << resultString(result)
              << " count=" << count << "\n";
}

}  // namespace xrh
