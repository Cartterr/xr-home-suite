#include <xrh/meta/meta_features.h>

#include <xrh/core/checks.h>

#include <iostream>

namespace xrh {

void MetaFeatures::createEnvironmentDepth(OpenXrContext& context) {
    if (!context.hasExtension(XR_META_ENVIRONMENT_DEPTH_EXTENSION_NAME) ||
        context.depthProperties().supportsEnvironmentDepth != XR_TRUE) {
        std::cout << "Environment depth/spatial data: not reported as supported over this Link session.\n";
        return;
    }

    xrCreateEnvironmentDepthProviderMETA_ =
        loadXrFunction<PFN_xrCreateEnvironmentDepthProviderMETA>(
            context.instance(), "xrCreateEnvironmentDepthProviderMETA", true);
    xrDestroyEnvironmentDepthProviderMETA_ =
        loadXrFunction<PFN_xrDestroyEnvironmentDepthProviderMETA>(
            context.instance(), "xrDestroyEnvironmentDepthProviderMETA", true);
    xrStartEnvironmentDepthProviderMETA_ =
        loadXrFunction<PFN_xrStartEnvironmentDepthProviderMETA>(
            context.instance(), "xrStartEnvironmentDepthProviderMETA", true);
    xrStopEnvironmentDepthProviderMETA_ =
        loadXrFunction<PFN_xrStopEnvironmentDepthProviderMETA>(
            context.instance(), "xrStopEnvironmentDepthProviderMETA", true);
    xrCreateEnvironmentDepthSwapchainMETA_ =
        loadXrFunction<PFN_xrCreateEnvironmentDepthSwapchainMETA>(
            context.instance(), "xrCreateEnvironmentDepthSwapchainMETA", true);
    xrDestroyEnvironmentDepthSwapchainMETA_ =
        loadXrFunction<PFN_xrDestroyEnvironmentDepthSwapchainMETA>(
            context.instance(), "xrDestroyEnvironmentDepthSwapchainMETA", true);
    xrEnumerateEnvironmentDepthSwapchainImagesMETA_ =
        loadXrFunction<PFN_xrEnumerateEnvironmentDepthSwapchainImagesMETA>(
            context.instance(), "xrEnumerateEnvironmentDepthSwapchainImagesMETA", true);
    xrGetEnvironmentDepthSwapchainStateMETA_ =
        loadXrFunction<PFN_xrGetEnvironmentDepthSwapchainStateMETA>(
            context.instance(), "xrGetEnvironmentDepthSwapchainStateMETA", true);
    xrAcquireEnvironmentDepthImageMETA_ =
        loadXrFunction<PFN_xrAcquireEnvironmentDepthImageMETA>(
            context.instance(), "xrAcquireEnvironmentDepthImageMETA", true);
    xrSetEnvironmentDepthHandRemovalMETA_ =
        loadXrFunction<PFN_xrSetEnvironmentDepthHandRemovalMETA>(
            context.instance(), "xrSetEnvironmentDepthHandRemovalMETA", false);

    XrEnvironmentDepthProviderCreateInfoMETA providerCreateInfo{
        XR_TYPE_ENVIRONMENT_DEPTH_PROVIDER_CREATE_INFO_META};
    const XrResult providerResult =
        xrCreateEnvironmentDepthProviderMETA_(context.session(), &providerCreateInfo, &environmentDepthProvider_);
    if (XR_FAILED(providerResult)) {
        std::cout << "Environment depth provider create: " << resultString(providerResult) << "\n";
        return;
    }

    XrEnvironmentDepthSwapchainCreateInfoMETA swapchainCreateInfo{
        XR_TYPE_ENVIRONMENT_DEPTH_SWAPCHAIN_CREATE_INFO_META};
    const XrResult swapchainResult =
        xrCreateEnvironmentDepthSwapchainMETA_(environmentDepthProvider_, &swapchainCreateInfo, &environmentDepthSwapchain_);
    if (XR_FAILED(swapchainResult)) {
        std::cout << "Environment depth swapchain create: " << resultString(swapchainResult) << "\n";
        return;
    }

    XrEnvironmentDepthSwapchainStateMETA state{XR_TYPE_ENVIRONMENT_DEPTH_SWAPCHAIN_STATE_META};
    const XrResult stateResult = xrGetEnvironmentDepthSwapchainStateMETA_(environmentDepthSwapchain_, &state);
    if (XR_SUCCEEDED(stateResult)) {
        environmentDepthWidth_ = state.width;
        environmentDepthHeight_ = state.height;
        std::cout << "Environment depth swapchain: " << state.width << "x" << state.height << "\n";
    } else {
        std::cout << "Environment depth swapchain state: " << resultString(stateResult) << "\n";
    }

    uint32_t depthImageCount = 0;
    const XrResult imageCountResult =
        xrEnumerateEnvironmentDepthSwapchainImagesMETA_(environmentDepthSwapchain_, 0, &depthImageCount, nullptr);
    if (XR_SUCCEEDED(imageCountResult) && depthImageCount > 0) {
        environmentDepthImages_.resize(depthImageCount, {XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR});
        const XrResult imagesResult =
            xrEnumerateEnvironmentDepthSwapchainImagesMETA_(
                environmentDepthSwapchain_,
                depthImageCount,
                &depthImageCount,
                reinterpret_cast<XrSwapchainImageBaseHeader*>(environmentDepthImages_.data()));
        std::cout << "Environment depth images: " << resultString(imagesResult)
                  << " count=" << depthImageCount << "\n";
    }

    if (xrSetEnvironmentDepthHandRemovalMETA_ && context.depthProperties().supportsHandRemoval == XR_TRUE) {
        XrEnvironmentDepthHandRemovalSetInfoMETA handRemoval{XR_TYPE_ENVIRONMENT_DEPTH_HAND_REMOVAL_SET_INFO_META};
        handRemoval.enabled = XR_TRUE;
        const XrResult handResult = xrSetEnvironmentDepthHandRemovalMETA_(environmentDepthProvider_, &handRemoval);
        std::cout << "Environment depth hand removal: " << resultString(handResult) << "\n";
    }

    const XrResult startResult = xrStartEnvironmentDepthProviderMETA_(environmentDepthProvider_);
    if (XR_SUCCEEDED(startResult)) {
        environmentDepthReady_ = true;
        std::cout << "Environment depth/spatial data provider: running.\n";
    } else {
        std::cout << "Environment depth provider start: " << resultString(startResult) << "\n";
    }
}

void MetaFeatures::acquireEnvironmentDepth(OpenXrContext& context, XrTime displayTime) {
    if (!environmentDepthReady_ || !xrAcquireEnvironmentDepthImageMETA_) {
        return;
    }

    XrEnvironmentDepthImageTimestampMETA timestamp{XR_TYPE_ENVIRONMENT_DEPTH_IMAGE_TIMESTAMP_META};
    XrEnvironmentDepthImageMETA image{XR_TYPE_ENVIRONMENT_DEPTH_IMAGE_META};
    image.next = &timestamp;
    image.views[0].type = XR_TYPE_ENVIRONMENT_DEPTH_IMAGE_VIEW_META;
    image.views[1].type = XR_TYPE_ENVIRONMENT_DEPTH_IMAGE_VIEW_META;

    XrEnvironmentDepthImageAcquireInfoMETA acquireInfo{XR_TYPE_ENVIRONMENT_DEPTH_IMAGE_ACQUIRE_INFO_META};
    acquireInfo.space = context.appSpace();
    acquireInfo.displayTime = displayTime;

    lastEnvironmentDepthResult_ =
        xrAcquireEnvironmentDepthImageMETA_(environmentDepthProvider_, &acquireInfo, &image);
    if (XR_SUCCEEDED(lastEnvironmentDepthResult_)) {
        ++environmentDepthFrameCount_;
        lastEnvironmentDepthNearZ_ = image.nearZ;
        lastEnvironmentDepthFarZ_ = image.farZ;
    }
}

}  // namespace xrh
