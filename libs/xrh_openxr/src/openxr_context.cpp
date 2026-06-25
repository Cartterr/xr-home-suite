#include <xrh/openxr/openxr_context.h>

#include <xrh/core/checks.h>

#include <cstring>
#include <iostream>
#include <sstream>

namespace xrh {

void OpenXrContext::initialize() {
    uint32_t extensionCount = 0;
    checkXr(xrEnumerateInstanceExtensionProperties(nullptr, 0, &extensionCount, nullptr),
            "xrEnumerateInstanceExtensionProperties(count)");
    extensions_.resize(extensionCount);
    for (auto& extension : extensions_) {
        extension.type = XR_TYPE_EXTENSION_PROPERTIES;
    }
    checkXr(xrEnumerateInstanceExtensionProperties(nullptr, extensionCount, &extensionCount, extensions_.data()),
            "xrEnumerateInstanceExtensionProperties(list)");

    std::cout << "Quest Link native MR diagnostic probe\n";
    std::cout << "No JVM / no Unity editor / no APK deploy.\n\n";
    std::cout << "Relevant runtime extensions:\n";
    for (const auto& extension : extensions_) {
        if (containsAnyInterestingToken(extension.extensionName)) {
            std::cout << "  " << extension.extensionName
                      << " specVersion=" << extension.extensionVersion << "\n";
        }
    }

    requireExtension(XR_KHR_D3D11_ENABLE_EXTENSION_NAME);
    enableOptionalExtension(XR_FB_PASSTHROUGH_EXTENSION_NAME);
    enableOptionalExtension(XR_FB_TRIANGLE_MESH_EXTENSION_NAME);
    enableOptionalExtension(XR_META_ENVIRONMENT_DEPTH_EXTENSION_NAME);
    enableOptionalExtension(XR_FB_SCENE_EXTENSION_NAME);
    enableOptionalExtension(XR_FB_SPATIAL_ENTITY_EXTENSION_NAME);
    enableOptionalExtension(XR_META_SPATIAL_ENTITY_MESH_EXTENSION_NAME);
    enableOptionalExtension(XR_EXT_HAND_TRACKING_EXTENSION_NAME);
    enableOptionalExtension("XR_METAX1_passthrough_camera_data");

    std::cout << "\nEnabled OpenXR extensions:\n";
    for (const char* name : enabledExtensions_) {
        std::cout << "  " << name << "\n";
    }

    XrInstanceCreateInfo createInfo{XR_TYPE_INSTANCE_CREATE_INFO};
    strcpy_s(createInfo.applicationInfo.applicationName, XR_MAX_APPLICATION_NAME_SIZE, "xr_home_suite_probe");
    createInfo.applicationInfo.applicationVersion = 3;
    strcpy_s(createInfo.applicationInfo.engineName, XR_MAX_ENGINE_NAME_SIZE, "xrh_native_diagnostic");
    createInfo.applicationInfo.engineVersion = 1;
    createInfo.applicationInfo.apiVersion = XR_MAKE_VERSION(1, 0, 0);
    createInfo.enabledExtensionCount = static_cast<uint32_t>(enabledExtensions_.size());
    createInfo.enabledExtensionNames = enabledExtensions_.data();
    checkXr(xrCreateInstance(&createInfo, &instance_), "xrCreateInstance");

    XrInstanceProperties instanceProperties{XR_TYPE_INSTANCE_PROPERTIES};
    checkXr(xrGetInstanceProperties(instance_, &instanceProperties), "xrGetInstanceProperties");
    runtimeName_ = instanceProperties.runtimeName;
    std::ostringstream version;
    version << XR_VERSION_MAJOR(instanceProperties.runtimeVersion) << "."
            << XR_VERSION_MINOR(instanceProperties.runtimeVersion) << "."
            << XR_VERSION_PATCH(instanceProperties.runtimeVersion);
    runtimeVersion_ = version.str();
    std::cout << "\nRuntime: " << runtimeName_ << " " << runtimeVersion_ << "\n";

    XrSystemGetInfo systemGetInfo{XR_TYPE_SYSTEM_GET_INFO};
    systemGetInfo.formFactor = kFormFactor;
    checkXr(xrGetSystem(instance_, &systemGetInfo, &systemId_), "xrGetSystem");
    std::cout << "HMD system id: " << systemId_ << "\n";

    passthroughSystemProperties_.next = &passthroughSystemProperties2_;
    passthroughSystemProperties2_.next = &environmentDepthSystemProperties_;
    if (hasExtension(XR_EXT_HAND_TRACKING_EXTENSION_NAME)) {
        environmentDepthSystemProperties_.next = &handTrackingSystemProperties_;
    }

    XrSystemProperties systemProperties{XR_TYPE_SYSTEM_PROPERTIES};
    systemProperties.next = &passthroughSystemProperties_;
    checkXr(xrGetSystemProperties(instance_, systemId_, &systemProperties), "xrGetSystemProperties");

    std::cout << "Passthrough supported: "
              << (passthroughSystemProperties_.supportsPassthrough == XR_TRUE ? "yes" : "no")
              << " capabilities=0x" << std::hex << passthroughSystemProperties2_.capabilities << std::dec << "\n";
    std::cout << "Environment depth supported: "
              << (environmentDepthSystemProperties_.supportsEnvironmentDepth == XR_TRUE ? "yes" : "no")
              << " handRemoval="
              << (environmentDepthSystemProperties_.supportsHandRemoval == XR_TRUE ? "yes" : "no") << "\n";
    std::cout << "Hand tracking supported: "
              << (handTrackingSystemProperties_.supportsHandTracking == XR_TRUE ? "yes" : "no") << "\n";
}

void OpenXrContext::requireExtension(const char* name) {
    if (!hasExtension(name)) {
        std::ostringstream stream;
        stream << "Required OpenXR extension missing: " << name;
        throw std::runtime_error(stream.str());
    }
    enabledExtensions_.push_back(name);
}

void OpenXrContext::enableOptionalExtension(const char* name) {
    if (hasExtension(name)) {
        enabledExtensions_.push_back(name);
    }
}

bool OpenXrContext::hasExtension(const char* name) const {
    return xrh::hasExtension(extensions_, name);
}

void OpenXrContext::createD3D11Session(ID3D11Device* device) {
    XrGraphicsBindingD3D11KHR graphicsBinding{XR_TYPE_GRAPHICS_BINDING_D3D11_KHR};
    graphicsBinding.device = device;

    XrSessionCreateInfo sessionCreateInfo{XR_TYPE_SESSION_CREATE_INFO};
    sessionCreateInfo.next = &graphicsBinding;
    sessionCreateInfo.systemId = systemId_;
    checkXr(xrCreateSession(instance_, &sessionCreateInfo, &session_), "xrCreateSession");
    std::cout << "OpenXR D3D11 session created.\n";
}

void OpenXrContext::createAppSpace() {
    XrReferenceSpaceCreateInfo spaceCreateInfo{XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
    spaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
    spaceCreateInfo.poseInReferenceSpace.orientation.w = 1.0f;
    checkXr(xrCreateReferenceSpace(session_, &spaceCreateInfo, &appSpace_), "xrCreateReferenceSpace");
}

void OpenXrContext::pollEvents() {
    XrEventDataBuffer event{XR_TYPE_EVENT_DATA_BUFFER};
    while (xrPollEvent(instance_, &event) == XR_SUCCESS) {
        switch (event.type) {
            case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED: {
                const auto& stateChanged = *reinterpret_cast<const XrEventDataSessionStateChanged*>(&event);
                handleSessionStateChanged(stateChanged.state);
                break;
            }
            case XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING:
                exitRequested_ = true;
                break;
#ifdef XR_TYPE_EVENT_DATA_PASSTHROUGH_STATE_CHANGED_FB
            case XR_TYPE_EVENT_DATA_PASSTHROUGH_STATE_CHANGED_FB: {
                const auto& passthroughChanged =
                    *reinterpret_cast<const XrEventDataPassthroughStateChangedFB*>(&event);
                std::cout << "Passthrough state changed flags=0x" << std::hex
                          << passthroughChanged.flags << std::dec << "\n";
                break;
            }
#endif
            default:
                break;
        }
        event = {XR_TYPE_EVENT_DATA_BUFFER};
    }
}

void OpenXrContext::handleSessionStateChanged(XrSessionState state) {
    sessionState_ = state;
    std::cout << "Session state: " << state << "\n";

    if (state == XR_SESSION_STATE_READY) {
        XrSessionBeginInfo beginInfo{XR_TYPE_SESSION_BEGIN_INFO};
        beginInfo.primaryViewConfigurationType = kViewConfig;
        checkXr(xrBeginSession(session_, &beginInfo), "xrBeginSession");
        sessionRunning_ = true;
        std::cout << "Session running.\n";
    } else if (state == XR_SESSION_STATE_STOPPING) {
        if (sessionRunning_) {
            checkXr(xrEndSession(session_), "xrEndSession");
            sessionRunning_ = false;
        }
    } else if (state == XR_SESSION_STATE_EXITING || state == XR_SESSION_STATE_LOSS_PENDING) {
        exitRequested_ = true;
    }
}

void OpenXrContext::requestExit() {
    exitRequested_ = true;
}

void OpenXrContext::cleanup() {
    if (sessionRunning_ && session_ != XR_NULL_HANDLE) {
        xrEndSession(session_);
        sessionRunning_ = false;
    }
    if (appSpace_ != XR_NULL_HANDLE) {
        xrDestroySpace(appSpace_);
        appSpace_ = XR_NULL_HANDLE;
    }
    if (session_ != XR_NULL_HANDLE) {
        xrDestroySession(session_);
        session_ = XR_NULL_HANDLE;
    }
    if (instance_ != XR_NULL_HANDLE) {
        xrDestroyInstance(instance_);
        instance_ = XR_NULL_HANDLE;
    }
}

void OpenXrContext::appendReport(ProbeReport& report) const {
    report.runtimeName = runtimeName_;
    report.runtimeVersion = runtimeVersion_;
    for (const char* extension : enabledExtensions_) {
        report.enabledExtensions.emplace_back(extension);
    }
    report.passthroughSupported = passthroughSystemProperties_.supportsPassthrough == XR_TRUE;
    report.passthroughCapabilities = passthroughSystemProperties2_.capabilities;
    report.environmentDepthSupported = environmentDepthSystemProperties_.supportsEnvironmentDepth == XR_TRUE;
    report.environmentDepthHandRemovalSupported = environmentDepthSystemProperties_.supportsHandRemoval == XR_TRUE;
    report.handTrackingSupported = handTrackingSystemProperties_.supportsHandTracking == XR_TRUE;
}

}  // namespace xrh
