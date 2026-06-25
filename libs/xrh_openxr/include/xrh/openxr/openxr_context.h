#pragma once

#include <xrh/core/openxr_platform.h>
#include <xrh/core/probe_report.h>

#include <string>
#include <vector>

namespace xrh {

constexpr XrFormFactor kFormFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
constexpr XrViewConfigurationType kViewConfig = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;

class OpenXrContext {
public:
    void initialize();
    void createD3D11Session(ID3D11Device* device);
    void createAppSpace();
    void pollEvents();
    void requestExit();
    void cleanup();
    void appendReport(ProbeReport& report) const;

    bool hasExtension(const char* name) const;
    bool sessionRunning() const { return sessionRunning_; }
    bool exitRequested() const { return exitRequested_; }

    XrInstance instance() const { return instance_; }
    XrSystemId systemId() const { return systemId_; }
    XrSession session() const { return session_; }
    XrSpace appSpace() const { return appSpace_; }

    const std::vector<XrExtensionProperties>& extensions() const { return extensions_; }
    const std::vector<const char*>& enabledExtensions() const { return enabledExtensions_; }
    const XrSystemPassthroughPropertiesFB& passthroughProperties() const { return passthroughSystemProperties_; }
    const XrSystemPassthroughProperties2FB& passthroughProperties2() const { return passthroughSystemProperties2_; }
    const XrSystemEnvironmentDepthPropertiesMETA& depthProperties() const { return environmentDepthSystemProperties_; }
    const XrSystemHandTrackingPropertiesEXT& handProperties() const { return handTrackingSystemProperties_; }

private:
    void requireExtension(const char* name);
    void enableOptionalExtension(const char* name);
    void handleSessionStateChanged(XrSessionState state);

    std::vector<XrExtensionProperties> extensions_;
    std::vector<const char*> enabledExtensions_;
    std::string runtimeName_;
    std::string runtimeVersion_;

    XrInstance instance_{XR_NULL_HANDLE};
    XrSystemId systemId_{XR_NULL_SYSTEM_ID};
    XrSession session_{XR_NULL_HANDLE};
    XrSpace appSpace_{XR_NULL_HANDLE};
    XrSessionState sessionState_{XR_SESSION_STATE_UNKNOWN};
    bool sessionRunning_{false};
    bool exitRequested_{false};

    XrSystemPassthroughPropertiesFB passthroughSystemProperties_{XR_TYPE_SYSTEM_PASSTHROUGH_PROPERTIES_FB};
    XrSystemPassthroughProperties2FB passthroughSystemProperties2_{XR_TYPE_SYSTEM_PASSTHROUGH_PROPERTIES2_FB};
    XrSystemEnvironmentDepthPropertiesMETA environmentDepthSystemProperties_{
        XR_TYPE_SYSTEM_ENVIRONMENT_DEPTH_PROPERTIES_META};
    XrSystemHandTrackingPropertiesEXT handTrackingSystemProperties_{
        XR_TYPE_SYSTEM_HAND_TRACKING_PROPERTIES_EXT};
};

}  // namespace xrh
