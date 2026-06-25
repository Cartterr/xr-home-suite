#include <xrh/meta/meta_features.h>

#include <xrh/core/checks.h>

#include <iostream>

namespace xrh {

void MetaFeatures::createPassthrough(OpenXrContext& context) {
    if (!context.hasExtension(XR_FB_PASSTHROUGH_EXTENSION_NAME) ||
        context.passthroughProperties().supportsPassthrough != XR_TRUE) {
        std::cout << "Passthrough compositor layer not available.\n";
        return;
    }

    xrCreatePassthroughFB_ =
        loadXrFunction<PFN_xrCreatePassthroughFB>(context.instance(), "xrCreatePassthroughFB", true);
    xrDestroyPassthroughFB_ =
        loadXrFunction<PFN_xrDestroyPassthroughFB>(context.instance(), "xrDestroyPassthroughFB", true);
    xrPassthroughStartFB_ =
        loadXrFunction<PFN_xrPassthroughStartFB>(context.instance(), "xrPassthroughStartFB", true);
    xrPassthroughPauseFB_ =
        loadXrFunction<PFN_xrPassthroughPauseFB>(context.instance(), "xrPassthroughPauseFB", true);
    xrCreatePassthroughLayerFB_ =
        loadXrFunction<PFN_xrCreatePassthroughLayerFB>(context.instance(), "xrCreatePassthroughLayerFB", true);
    xrDestroyPassthroughLayerFB_ =
        loadXrFunction<PFN_xrDestroyPassthroughLayerFB>(context.instance(), "xrDestroyPassthroughLayerFB", true);
    xrPassthroughLayerResumeFB_ =
        loadXrFunction<PFN_xrPassthroughLayerResumeFB>(context.instance(), "xrPassthroughLayerResumeFB", true);
    xrPassthroughLayerPauseFB_ =
        loadXrFunction<PFN_xrPassthroughLayerPauseFB>(context.instance(), "xrPassthroughLayerPauseFB", true);
    xrPassthroughLayerSetStyleFB_ =
        loadXrFunction<PFN_xrPassthroughLayerSetStyleFB>(context.instance(), "xrPassthroughLayerSetStyleFB", true);

    XrPassthroughCreateInfoFB passthroughCreateInfo{XR_TYPE_PASSTHROUGH_CREATE_INFO_FB};
    checkXr(xrCreatePassthroughFB_(context.session(), &passthroughCreateInfo, &passthrough_),
            "xrCreatePassthroughFB");
    checkXr(xrPassthroughStartFB_(passthrough_), "xrPassthroughStartFB");

    XrPassthroughLayerCreateInfoFB layerCreateInfo{XR_TYPE_PASSTHROUGH_LAYER_CREATE_INFO_FB};
    layerCreateInfo.passthrough = passthrough_;
    layerCreateInfo.purpose = XR_PASSTHROUGH_LAYER_PURPOSE_RECONSTRUCTION_FB;
    checkXr(xrCreatePassthroughLayerFB_(context.session(), &layerCreateInfo, &passthroughLayer_),
            "xrCreatePassthroughLayerFB");
    checkXr(xrPassthroughLayerResumeFB_(passthroughLayer_), "xrPassthroughLayerResumeFB");

    XrPassthroughStyleFB style{XR_TYPE_PASSTHROUGH_STYLE_FB};
    style.textureOpacityFactor = 1.0f;
    style.edgeColor = XrColor4f{0.0f, 0.85f, 1.0f, 0.18f};
    const XrResult styleResult = xrPassthroughLayerSetStyleFB_(passthroughLayer_, &style);
    if (XR_FAILED(styleResult)) {
        std::cout << "xrPassthroughLayerSetStyleFB: " << resultString(styleResult) << "\n";
    }

    passthroughReady_ = true;
    std::cout << "Passthrough compositor layer: running.\n";
}

}  // namespace xrh
