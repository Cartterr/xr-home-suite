#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define XR_USE_PLATFORM_WIN32
#define XR_USE_GRAPHICS_API_D3D11

#include <windows.h>

#include <d3d11.h>
#include <d3dcompiler.h>
#include <dxgi1_2.h>
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#include <wrl/client.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace {

using Microsoft::WRL::ComPtr;

constexpr XrFormFactor kFormFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
constexpr XrViewConfigurationType kViewConfig = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
constexpr float kPi = 3.14159265358979323846f;

using PFN_xrEnumeratePassthroughCameraSourcePropertiesMETAX1 =
    XrResult(XRAPI_PTR*)(XrSession session,
                         uint32_t cameraSourceCapacityInput,
                         uint32_t* cameraSourceCountOutput,
                         void* cameraSourceProperties);

struct Vertex {
    float x;
    float y;
    float r;
    float g;
    float b;
    float a;
};

struct Color {
    float r;
    float g;
    float b;
    float a;
};

struct Vec2 {
    float x;
    float y;
};

struct SwapchainBundle {
    XrSwapchain handle{XR_NULL_HANDLE};
    int32_t width{0};
    int32_t height{0};
    std::vector<XrSwapchainImageD3D11KHR> images;
    std::vector<ComPtr<ID3D11RenderTargetView>> renderTargets;
};

std::string toLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

bool containsAnyInterestingToken(const std::string& name) {
    const std::string lower = toLower(name);
    return lower.find("camera") != std::string::npos ||
           lower.find("passthrough") != std::string::npos ||
           lower.find("spatial") != std::string::npos ||
           lower.find("scene") != std::string::npos ||
           lower.find("depth") != std::string::npos ||
           lower.find("meta") != std::string::npos ||
           lower.find("fb_") != std::string::npos ||
           lower.find("oculus") != std::string::npos;
}

const char* resultName(XrResult result) {
    switch (result) {
        case XR_SUCCESS: return "XR_SUCCESS";
        case XR_TIMEOUT_EXPIRED: return "XR_TIMEOUT_EXPIRED";
        case XR_SESSION_LOSS_PENDING: return "XR_SESSION_LOSS_PENDING";
        case XR_EVENT_UNAVAILABLE: return "XR_EVENT_UNAVAILABLE";
        case XR_SPACE_BOUNDS_UNAVAILABLE: return "XR_SPACE_BOUNDS_UNAVAILABLE";
        case XR_SESSION_NOT_FOCUSED: return "XR_SESSION_NOT_FOCUSED";
        case XR_FRAME_DISCARDED: return "XR_FRAME_DISCARDED";
        case XR_ERROR_VALIDATION_FAILURE: return "XR_ERROR_VALIDATION_FAILURE";
        case XR_ERROR_RUNTIME_FAILURE: return "XR_ERROR_RUNTIME_FAILURE";
        case XR_ERROR_OUT_OF_MEMORY: return "XR_ERROR_OUT_OF_MEMORY";
        case XR_ERROR_API_VERSION_UNSUPPORTED: return "XR_ERROR_API_VERSION_UNSUPPORTED";
        case XR_ERROR_INITIALIZATION_FAILED: return "XR_ERROR_INITIALIZATION_FAILED";
        case XR_ERROR_FUNCTION_UNSUPPORTED: return "XR_ERROR_FUNCTION_UNSUPPORTED";
        case XR_ERROR_FEATURE_UNSUPPORTED: return "XR_ERROR_FEATURE_UNSUPPORTED";
#ifdef XR_ERROR_PERMISSION_INSUFFICIENT
        case XR_ERROR_PERMISSION_INSUFFICIENT: return "XR_ERROR_PERMISSION_INSUFFICIENT";
#endif
        case XR_ERROR_EXTENSION_NOT_PRESENT: return "XR_ERROR_EXTENSION_NOT_PRESENT";
        case XR_ERROR_LIMIT_REACHED: return "XR_ERROR_LIMIT_REACHED";
        case XR_ERROR_SIZE_INSUFFICIENT: return "XR_ERROR_SIZE_INSUFFICIENT";
        case XR_ERROR_HANDLE_INVALID: return "XR_ERROR_HANDLE_INVALID";
        case XR_ERROR_INSTANCE_LOST: return "XR_ERROR_INSTANCE_LOST";
        case XR_ERROR_SESSION_RUNNING: return "XR_ERROR_SESSION_RUNNING";
        case XR_ERROR_SESSION_NOT_RUNNING: return "XR_ERROR_SESSION_NOT_RUNNING";
        case XR_ERROR_SESSION_LOST: return "XR_ERROR_SESSION_LOST";
        case XR_ERROR_SYSTEM_INVALID: return "XR_ERROR_SYSTEM_INVALID";
        case XR_ERROR_PATH_INVALID: return "XR_ERROR_PATH_INVALID";
        case XR_ERROR_PATH_COUNT_EXCEEDED: return "XR_ERROR_PATH_COUNT_EXCEEDED";
        case XR_ERROR_PATH_FORMAT_INVALID: return "XR_ERROR_PATH_FORMAT_INVALID";
        case XR_ERROR_PATH_UNSUPPORTED: return "XR_ERROR_PATH_UNSUPPORTED";
        case XR_ERROR_LAYER_INVALID: return "XR_ERROR_LAYER_INVALID";
        case XR_ERROR_LAYER_LIMIT_EXCEEDED: return "XR_ERROR_LAYER_LIMIT_EXCEEDED";
        case XR_ERROR_SWAPCHAIN_RECT_INVALID: return "XR_ERROR_SWAPCHAIN_RECT_INVALID";
        case XR_ERROR_SWAPCHAIN_FORMAT_UNSUPPORTED: return "XR_ERROR_SWAPCHAIN_FORMAT_UNSUPPORTED";
        case XR_ERROR_ACTION_TYPE_MISMATCH: return "XR_ERROR_ACTION_TYPE_MISMATCH";
        case XR_ERROR_SESSION_NOT_READY: return "XR_ERROR_SESSION_NOT_READY";
        case XR_ERROR_SESSION_NOT_STOPPING: return "XR_ERROR_SESSION_NOT_STOPPING";
        case XR_ERROR_TIME_INVALID: return "XR_ERROR_TIME_INVALID";
        case XR_ERROR_REFERENCE_SPACE_UNSUPPORTED: return "XR_ERROR_REFERENCE_SPACE_UNSUPPORTED";
        case XR_ERROR_FILE_ACCESS_ERROR: return "XR_ERROR_FILE_ACCESS_ERROR";
        case XR_ERROR_FILE_CONTENTS_INVALID: return "XR_ERROR_FILE_CONTENTS_INVALID";
        case XR_ERROR_FORM_FACTOR_UNSUPPORTED: return "XR_ERROR_FORM_FACTOR_UNSUPPORTED";
        case XR_ERROR_FORM_FACTOR_UNAVAILABLE: return "XR_ERROR_FORM_FACTOR_UNAVAILABLE";
        case XR_ERROR_API_LAYER_NOT_PRESENT: return "XR_ERROR_API_LAYER_NOT_PRESENT";
        case XR_ERROR_CALL_ORDER_INVALID: return "XR_ERROR_CALL_ORDER_INVALID";
        case XR_ERROR_GRAPHICS_DEVICE_INVALID: return "XR_ERROR_GRAPHICS_DEVICE_INVALID";
        case XR_ERROR_POSE_INVALID: return "XR_ERROR_POSE_INVALID";
        case XR_ERROR_INDEX_OUT_OF_RANGE: return "XR_ERROR_INDEX_OUT_OF_RANGE";
        case XR_ERROR_VIEW_CONFIGURATION_TYPE_UNSUPPORTED: return "XR_ERROR_VIEW_CONFIGURATION_TYPE_UNSUPPORTED";
        case XR_ERROR_ENVIRONMENT_BLEND_MODE_UNSUPPORTED: return "XR_ERROR_ENVIRONMENT_BLEND_MODE_UNSUPPORTED";
        case XR_ERROR_NAME_DUPLICATED: return "XR_ERROR_NAME_DUPLICATED";
        case XR_ERROR_NAME_INVALID: return "XR_ERROR_NAME_INVALID";
        case XR_ERROR_ACTIONSET_NOT_ATTACHED: return "XR_ERROR_ACTIONSET_NOT_ATTACHED";
        case XR_ERROR_ACTIONSETS_ALREADY_ATTACHED: return "XR_ERROR_ACTIONSETS_ALREADY_ATTACHED";
        case XR_ERROR_LOCALIZED_NAME_DUPLICATED: return "XR_ERROR_LOCALIZED_NAME_DUPLICATED";
        case XR_ERROR_LOCALIZED_NAME_INVALID: return "XR_ERROR_LOCALIZED_NAME_INVALID";
#ifdef XR_ERROR_GRAPHICS_REQUIREMENTS_CALL_MISSING
        case XR_ERROR_GRAPHICS_REQUIREMENTS_CALL_MISSING: return "XR_ERROR_GRAPHICS_REQUIREMENTS_CALL_MISSING";
#endif
        default: return "XR_RESULT_UNKNOWN";
    }
}

std::string resultString(XrResult result) {
    std::ostringstream stream;
    stream << resultName(result) << " (" << result << ")";
    return stream.str();
}

void checkXr(XrResult result, const char* label) {
    if (XR_FAILED(result)) {
        std::ostringstream stream;
        stream << label << " failed: " << resultString(result);
        throw std::runtime_error(stream.str());
    }
}

void checkHr(HRESULT hr, const char* label) {
    if (FAILED(hr)) {
        std::ostringstream stream;
        stream << label << " failed: HRESULT=0x" << std::hex << static_cast<unsigned long>(hr);
        throw std::runtime_error(stream.str());
    }
}

bool hasExtension(const std::vector<XrExtensionProperties>& extensions, const char* name) {
    return std::any_of(extensions.begin(), extensions.end(), [name](const XrExtensionProperties& extension) {
        return std::strcmp(extension.extensionName, name) == 0;
    });
}

std::string formatLuid(const LUID& luid) {
    std::ostringstream stream;
    stream << "0x" << std::hex << std::setfill('0')
           << std::setw(8) << static_cast<unsigned long>(luid.HighPart)
           << std::setw(8) << static_cast<unsigned long>(luid.LowPart);
    return stream.str();
}

bool sameLuid(const LUID& left, const LUID& right) {
    return left.HighPart == right.HighPart && left.LowPart == right.LowPart;
}

template <typename Fn>
Fn loadXrFunction(XrInstance instance, const char* name, bool required) {
    PFN_xrVoidFunction function = nullptr;
    const XrResult result = xrGetInstanceProcAddr(instance, name, &function);
    if ((XR_FAILED(result) || function == nullptr) && required) {
        std::ostringstream stream;
        stream << name << " unavailable: " << resultString(result);
        throw std::runtime_error(stream.str());
    }
    return reinterpret_cast<Fn>(function);
}

void appendRect(std::vector<Vertex>& vertices, float x0, float y0, float x1, float y1, Color color) {
    const Vertex a{x0, y0, color.r, color.g, color.b, color.a};
    const Vertex b{x1, y0, color.r, color.g, color.b, color.a};
    const Vertex c{x1, y1, color.r, color.g, color.b, color.a};
    const Vertex d{x0, y1, color.r, color.g, color.b, color.a};
    vertices.insert(vertices.end(), {a, b, c, a, c, d});
}

void appendLine(std::vector<Vertex>& vertices, Vec2 a, Vec2 b, float halfWidth, Color color) {
    const float dx = b.x - a.x;
    const float dy = b.y - a.y;
    const float length = std::sqrt(dx * dx + dy * dy);
    if (length <= std::numeric_limits<float>::epsilon()) {
        appendRect(vertices, a.x - halfWidth, a.y - halfWidth, a.x + halfWidth, a.y + halfWidth, color);
        return;
    }

    const float nx = -dy / length * halfWidth;
    const float ny = dx / length * halfWidth;
    vertices.insert(vertices.end(), {
        {a.x + nx, a.y + ny, color.r, color.g, color.b, color.a},
        {b.x + nx, b.y + ny, color.r, color.g, color.b, color.a},
        {b.x - nx, b.y - ny, color.r, color.g, color.b, color.a},
        {a.x + nx, a.y + ny, color.r, color.g, color.b, color.a},
        {b.x - nx, b.y - ny, color.r, color.g, color.b, color.a},
        {a.x - nx, a.y - ny, color.r, color.g, color.b, color.a},
    });
}

void appendRotatedQuad(std::vector<Vertex>& vertices, float cx, float cy, float radius, float angle, Color color) {
    std::array<Vertex, 4> corners{};
    for (size_t i = 0; i < corners.size(); ++i) {
        const float localAngle = angle + kPi * 0.25f + static_cast<float>(i) * kPi * 0.5f;
        corners[i] = Vertex{
            cx + std::cos(localAngle) * radius,
            cy + std::sin(localAngle) * radius,
            color.r,
            color.g,
            color.b,
            color.a,
        };
    }
    vertices.insert(vertices.end(), {corners[0], corners[1], corners[2], corners[0], corners[2], corners[3]});
}

XrVector3f rotateByInverse(const XrQuaternionf& q, XrVector3f v) {
    const XrQuaternionf inverse{-q.x, -q.y, -q.z, q.w};
    const XrVector3f u{inverse.x, inverse.y, inverse.z};
    const float s = inverse.w;
    const float dotUV = u.x * v.x + u.y * v.y + u.z * v.z;
    const float dotUU = u.x * u.x + u.y * u.y + u.z * u.z;
    const XrVector3f cross{
        u.y * v.z - u.z * v.y,
        u.z * v.x - u.x * v.z,
        u.x * v.y - u.y * v.x,
    };
    return XrVector3f{
        2.0f * dotUV * u.x + (s * s - dotUU) * v.x + 2.0f * s * cross.x,
        2.0f * dotUV * u.y + (s * s - dotUU) * v.y + 2.0f * s * cross.y,
        2.0f * dotUV * u.z + (s * s - dotUU) * v.z + 2.0f * s * cross.z,
    };
}

class LiveLinkApp {
public:
    explicit LiveLinkApp(int runSeconds) : runSeconds_(runSeconds) {}

    int run() {
        try {
            initializeOpenXr();
            initializeD3D11();
            createSession();
            createAppSpace();
            createHandTracking();
            createRenderResources();
            createPassthrough();
            createEnvironmentDepth();
            probePrivateCameraSources();
            mainLoop();
            cleanup();
            return 0;
        } catch (const std::exception& error) {
            std::cerr << "Fatal: " << error.what() << "\n";
            cleanup();
            return 1;
        }
    }

private:
    void initializeOpenXr() {
        uint32_t extensionCount = 0;
        checkXr(xrEnumerateInstanceExtensionProperties(nullptr, 0, &extensionCount, nullptr),
                "xrEnumerateInstanceExtensionProperties(count)");
        extensions_.resize(extensionCount);
        for (auto& extension : extensions_) {
            extension.type = XR_TYPE_EXTENSION_PROPERTIES;
        }
        checkXr(xrEnumerateInstanceExtensionProperties(nullptr, extensionCount, &extensionCount, extensions_.data()),
                "xrEnumerateInstanceExtensionProperties(list)");

        std::cout << "Quest Link native MR live test\n";
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
        strcpy_s(createInfo.applicationInfo.applicationName,
                 XR_MAX_APPLICATION_NAME_SIZE,
                 "xr_home_suite_link_mr_probe");
        createInfo.applicationInfo.applicationVersion = 2;
        strcpy_s(createInfo.applicationInfo.engineName,
                 XR_MAX_ENGINE_NAME_SIZE,
                 "minimal_d3d11_openxr");
        createInfo.applicationInfo.engineVersion = 1;
        createInfo.applicationInfo.apiVersion = XR_MAKE_VERSION(1, 0, 0);
        createInfo.enabledExtensionCount = static_cast<uint32_t>(enabledExtensions_.size());
        createInfo.enabledExtensionNames = enabledExtensions_.data();
        checkXr(xrCreateInstance(&createInfo, &instance_), "xrCreateInstance");

        XrInstanceProperties instanceProperties{XR_TYPE_INSTANCE_PROPERTIES};
        checkXr(xrGetInstanceProperties(instance_, &instanceProperties), "xrGetInstanceProperties");
        std::cout << "\nRuntime: " << instanceProperties.runtimeName
                  << " " << XR_VERSION_MAJOR(instanceProperties.runtimeVersion)
                  << "." << XR_VERSION_MINOR(instanceProperties.runtimeVersion)
                  << "." << XR_VERSION_PATCH(instanceProperties.runtimeVersion) << "\n";

        XrSystemGetInfo systemGetInfo{XR_TYPE_SYSTEM_GET_INFO};
        systemGetInfo.formFactor = kFormFactor;
        checkXr(xrGetSystem(instance_, &systemGetInfo, &systemId_), "xrGetSystem");
        std::cout << "HMD system id: " << systemId_ << "\n";

        passthroughSystemProperties_.type = XR_TYPE_SYSTEM_PASSTHROUGH_PROPERTIES_FB;
        passthroughSystemProperties2_.type = XR_TYPE_SYSTEM_PASSTHROUGH_PROPERTIES2_FB;
        environmentDepthSystemProperties_.type = XR_TYPE_SYSTEM_ENVIRONMENT_DEPTH_PROPERTIES_META;
        handTrackingSystemProperties_.type = XR_TYPE_SYSTEM_HAND_TRACKING_PROPERTIES_EXT;
        passthroughSystemProperties_.next = &passthroughSystemProperties2_;
        passthroughSystemProperties2_.next = &environmentDepthSystemProperties_;
        if (hasExtension(extensions_, XR_EXT_HAND_TRACKING_EXTENSION_NAME)) {
            environmentDepthSystemProperties_.next = &handTrackingSystemProperties_;
        }

        XrSystemProperties systemProperties{XR_TYPE_SYSTEM_PROPERTIES};
        systemProperties.next = &passthroughSystemProperties_;
        checkXr(xrGetSystemProperties(instance_, systemId_, &systemProperties), "xrGetSystemProperties");

        std::cout << "Passthrough supported: "
                  << (passthroughSystemProperties_.supportsPassthrough == XR_TRUE ? "yes" : "no")
                  << " capabilities=0x" << std::hex << passthroughSystemProperties2_.capabilities << std::dec
                  << "\n";
        std::cout << "Environment depth supported: "
                  << (environmentDepthSystemProperties_.supportsEnvironmentDepth == XR_TRUE ? "yes" : "no")
                  << " handRemoval="
                  << (environmentDepthSystemProperties_.supportsHandRemoval == XR_TRUE ? "yes" : "no") << "\n";
        std::cout << "Hand tracking supported: "
                  << (handTrackingSystemProperties_.supportsHandTracking == XR_TRUE ? "yes" : "no") << "\n";
    }

    void requireExtension(const char* name) {
        if (!hasExtension(extensions_, name)) {
            std::ostringstream stream;
            stream << "Required OpenXR extension missing: " << name;
            throw std::runtime_error(stream.str());
        }
        enabledExtensions_.push_back(name);
    }

    void enableOptionalExtension(const char* name) {
        if (hasExtension(extensions_, name)) {
            enabledExtensions_.push_back(name);
        }
    }

    void initializeD3D11() {
        auto getD3D11GraphicsRequirements =
            loadXrFunction<PFN_xrGetD3D11GraphicsRequirementsKHR>(
                instance_, "xrGetD3D11GraphicsRequirementsKHR", true);

        XrGraphicsRequirementsD3D11KHR graphicsRequirements{XR_TYPE_GRAPHICS_REQUIREMENTS_D3D11_KHR};
        checkXr(getD3D11GraphicsRequirements(instance_, systemId_, &graphicsRequirements),
                "xrGetD3D11GraphicsRequirementsKHR");
        std::cout << "Runtime D3D11 adapter LUID: " << formatLuid(graphicsRequirements.adapterLuid) << "\n";

        ComPtr<IDXGIFactory1> factory;
        checkHr(CreateDXGIFactory1(IID_PPV_ARGS(&factory)), "CreateDXGIFactory1");

        ComPtr<IDXGIAdapter1> selectedAdapter;
        for (UINT adapterIndex = 0;; ++adapterIndex) {
            ComPtr<IDXGIAdapter1> adapter;
            if (factory->EnumAdapters1(adapterIndex, &adapter) == DXGI_ERROR_NOT_FOUND) {
                break;
            }

            DXGI_ADAPTER_DESC1 desc{};
            if (SUCCEEDED(adapter->GetDesc1(&desc)) && sameLuid(desc.AdapterLuid, graphicsRequirements.adapterLuid)) {
                selectedAdapter = adapter;
                std::wcout << L"D3D11 adapter: " << desc.Description << L"\n";
                break;
            }
        }

        if (!selectedAdapter) {
            throw std::runtime_error("Could not find the D3D11 adapter requested by the OpenXR runtime.");
        }

        constexpr D3D_FEATURE_LEVEL featureLevels[] = {
            D3D_FEATURE_LEVEL_12_1,
            D3D_FEATURE_LEVEL_12_0,
            D3D_FEATURE_LEVEL_11_1,
            D3D_FEATURE_LEVEL_11_0,
        };

        D3D_FEATURE_LEVEL createdFeatureLevel{};
        checkHr(D3D11CreateDevice(selectedAdapter.Get(),
                                  D3D_DRIVER_TYPE_UNKNOWN,
                                  nullptr,
                                  0,
                                  featureLevels,
                                  static_cast<UINT>(std::size(featureLevels)),
                                  D3D11_SDK_VERSION,
                                  &device_,
                                  &createdFeatureLevel,
                                  &context_),
                "D3D11CreateDevice");
    }

    void createSession() {
        XrGraphicsBindingD3D11KHR graphicsBinding{XR_TYPE_GRAPHICS_BINDING_D3D11_KHR};
        graphicsBinding.device = device_.Get();

        XrSessionCreateInfo sessionCreateInfo{XR_TYPE_SESSION_CREATE_INFO};
        sessionCreateInfo.next = &graphicsBinding;
        sessionCreateInfo.systemId = systemId_;
        checkXr(xrCreateSession(instance_, &sessionCreateInfo, &session_), "xrCreateSession");
        std::cout << "OpenXR D3D11 session created.\n";
    }

    void createAppSpace() {
        XrReferenceSpaceCreateInfo spaceCreateInfo{XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
        spaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
        spaceCreateInfo.poseInReferenceSpace.orientation.w = 1.0f;
        checkXr(xrCreateReferenceSpace(session_, &spaceCreateInfo, &appSpace_), "xrCreateReferenceSpace");
    }

    void createHandTracking() {
        if (!hasExtension(extensions_, XR_EXT_HAND_TRACKING_EXTENSION_NAME) ||
            handTrackingSystemProperties_.supportsHandTracking != XR_TRUE) {
            std::cout << "Hand tracking: not reported as supported over this Link session.\n";
            return;
        }

        xrCreateHandTrackerEXT_ =
            loadXrFunction<PFN_xrCreateHandTrackerEXT>(instance_, "xrCreateHandTrackerEXT", true);
        xrDestroyHandTrackerEXT_ =
            loadXrFunction<PFN_xrDestroyHandTrackerEXT>(instance_, "xrDestroyHandTrackerEXT", true);
        xrLocateHandJointsEXT_ =
            loadXrFunction<PFN_xrLocateHandJointsEXT>(instance_, "xrLocateHandJointsEXT", true);

        createHandTracker(0, XR_HAND_LEFT_EXT, "left");
        createHandTracker(1, XR_HAND_RIGHT_EXT, "right");
        handTrackingReady_ =
            handTrackers_[0] != XR_NULL_HANDLE || handTrackers_[1] != XR_NULL_HANDLE;

        std::cout << "Hand tracking trackers: "
                  << (handTrackers_[0] != XR_NULL_HANDLE ? "left " : "")
                  << (handTrackers_[1] != XR_NULL_HANDLE ? "right" : "")
                  << (handTrackingReady_ ? "\n" : "none\n");
    }

    void createHandTracker(size_t index, XrHandEXT hand, const char* label) {
        XrHandTrackerCreateInfoEXT createInfo{XR_TYPE_HAND_TRACKER_CREATE_INFO_EXT};
        createInfo.hand = hand;
        createInfo.handJointSet = XR_HAND_JOINT_SET_DEFAULT_EXT;

        const XrResult result =
            xrCreateHandTrackerEXT_(session_, &createInfo, &handTrackers_[index]);
        if (XR_FAILED(result)) {
            handTrackers_[index] = XR_NULL_HANDLE;
            std::cout << "Hand tracking " << label << " tracker create: "
                      << resultString(result) << "\n";
        }
    }

    void createRenderResources() {
        uint32_t viewCount = 0;
        checkXr(xrEnumerateViewConfigurationViews(instance_, systemId_, kViewConfig, 0, &viewCount, nullptr),
                "xrEnumerateViewConfigurationViews(count)");

        viewConfigViews_.resize(viewCount, {XR_TYPE_VIEW_CONFIGURATION_VIEW});
        checkXr(xrEnumerateViewConfigurationViews(
                    instance_, systemId_, kViewConfig, viewCount, &viewCount, viewConfigViews_.data()),
                "xrEnumerateViewConfigurationViews(list)");
        views_.resize(viewCount, {XR_TYPE_VIEW});
        projectionViews_.resize(viewCount, {XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW});

        uint32_t formatCount = 0;
        checkXr(xrEnumerateSwapchainFormats(session_, 0, &formatCount, nullptr),
                "xrEnumerateSwapchainFormats(count)");
        std::vector<int64_t> formats(formatCount);
        checkXr(xrEnumerateSwapchainFormats(session_, formatCount, &formatCount, formats.data()),
                "xrEnumerateSwapchainFormats(list)");

        const std::array<DXGI_FORMAT, 4> preferredFormats{
            DXGI_FORMAT_R8G8B8A8_UNORM,
            DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
            DXGI_FORMAT_B8G8R8A8_UNORM,
            DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,
        };
        DXGI_FORMAT selectedFormat = DXGI_FORMAT_UNKNOWN;
        for (DXGI_FORMAT candidate : preferredFormats) {
            if (std::find(formats.begin(), formats.end(), static_cast<int64_t>(candidate)) != formats.end()) {
                selectedFormat = candidate;
                break;
            }
        }
        if (selectedFormat == DXGI_FORMAT_UNKNOWN) {
            selectedFormat = static_cast<DXGI_FORMAT>(formats.front());
        }

        swapchains_.resize(viewCount);
        for (uint32_t eye = 0; eye < viewCount; ++eye) {
            const auto& config = viewConfigViews_[eye];
            SwapchainBundle& swapchain = swapchains_[eye];
            swapchain.width = static_cast<int32_t>(config.recommendedImageRectWidth);
            swapchain.height = static_cast<int32_t>(config.recommendedImageRectHeight);

            XrSwapchainCreateInfo swapchainCreateInfo{XR_TYPE_SWAPCHAIN_CREATE_INFO};
            swapchainCreateInfo.arraySize = 1;
            swapchainCreateInfo.mipCount = 1;
            swapchainCreateInfo.faceCount = 1;
            swapchainCreateInfo.format = selectedFormat;
            swapchainCreateInfo.width = config.recommendedImageRectWidth;
            swapchainCreateInfo.height = config.recommendedImageRectHeight;
            swapchainCreateInfo.sampleCount = config.recommendedSwapchainSampleCount;
            swapchainCreateInfo.usageFlags = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;
            checkXr(xrCreateSwapchain(session_, &swapchainCreateInfo, &swapchain.handle), "xrCreateSwapchain");

            uint32_t imageCount = 0;
            checkXr(xrEnumerateSwapchainImages(swapchain.handle, 0, &imageCount, nullptr),
                    "xrEnumerateSwapchainImages(count)");
            swapchain.images.resize(imageCount, {XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR});
            checkXr(xrEnumerateSwapchainImages(swapchain.handle,
                                               imageCount,
                                               &imageCount,
                                               reinterpret_cast<XrSwapchainImageBaseHeader*>(swapchain.images.data())),
                    "xrEnumerateSwapchainImages(list)");

            swapchain.renderTargets.resize(imageCount);
            for (uint32_t image = 0; image < imageCount; ++image) {
                D3D11_TEXTURE2D_DESC textureDesc{};
                swapchain.images[image].texture->GetDesc(&textureDesc);
                std::cout << "Eye " << eye << " image " << image
                          << " tex=" << textureDesc.Width << "x" << textureDesc.Height
                          << " fmt=" << textureDesc.Format
                          << " samples=" << textureDesc.SampleDesc.Count
                          << " array=" << textureDesc.ArraySize
                          << " bind=0x" << std::hex << textureDesc.BindFlags << std::dec
                          << "\n";

                D3D11_RENDER_TARGET_VIEW_DESC rtvDesc{};
                rtvDesc.Format = selectedFormat;
                if (textureDesc.SampleDesc.Count > 1) {
                    rtvDesc.ViewDimension = textureDesc.ArraySize > 1
                        ? D3D11_RTV_DIMENSION_TEXTURE2DMSARRAY
                        : D3D11_RTV_DIMENSION_TEXTURE2DMS;
                    if (rtvDesc.ViewDimension == D3D11_RTV_DIMENSION_TEXTURE2DMSARRAY) {
                        rtvDesc.Texture2DMSArray.ArraySize = textureDesc.ArraySize;
                        rtvDesc.Texture2DMSArray.FirstArraySlice = 0;
                    }
                } else {
                    rtvDesc.ViewDimension = textureDesc.ArraySize > 1
                        ? D3D11_RTV_DIMENSION_TEXTURE2DARRAY
                        : D3D11_RTV_DIMENSION_TEXTURE2D;
                    if (rtvDesc.ViewDimension == D3D11_RTV_DIMENSION_TEXTURE2DARRAY) {
                        rtvDesc.Texture2DArray.ArraySize = textureDesc.ArraySize;
                        rtvDesc.Texture2DArray.FirstArraySlice = 0;
                        rtvDesc.Texture2DArray.MipSlice = 0;
                    } else {
                        rtvDesc.Texture2D.MipSlice = 0;
                    }
                }

                checkHr(device_->CreateRenderTargetView(
                            swapchain.images[image].texture, &rtvDesc, &swapchain.renderTargets[image]),
                        "CreateRenderTargetView");
            }

            std::cout << "Eye " << eye << " swapchain: " << swapchain.width << "x" << swapchain.height
                      << " images=" << imageCount << " format=" << static_cast<int>(selectedFormat) << "\n";
        }

        createShaders();
    }

    void createShaders() {
        static constexpr const char* shaderSource = R"(
struct VSIn { float2 pos : POSITION; float4 color : COLOR0; };
struct PSIn { float4 pos : SV_POSITION; float4 color : COLOR0; };
PSIn VSMain(VSIn input) {
    PSIn output;
    output.pos = float4(input.pos, 0.0f, 1.0f);
    output.color = input.color;
    return output;
}
float4 PSMain(PSIn input) : SV_Target { return input.color; }
)";

        ComPtr<ID3DBlob> vertexBlob;
        ComPtr<ID3DBlob> pixelBlob;
        ComPtr<ID3DBlob> errors;
        HRESULT hr = D3DCompile(shaderSource, std::strlen(shaderSource), nullptr, nullptr, nullptr,
                                "VSMain", "vs_5_0", 0, 0, &vertexBlob, &errors);
        if (FAILED(hr)) {
            if (errors) {
                std::cerr << static_cast<const char*>(errors->GetBufferPointer()) << "\n";
            }
            checkHr(hr, "D3DCompile(VSMain)");
        }

        errors.Reset();
        hr = D3DCompile(shaderSource, std::strlen(shaderSource), nullptr, nullptr, nullptr,
                        "PSMain", "ps_5_0", 0, 0, &pixelBlob, &errors);
        if (FAILED(hr)) {
            if (errors) {
                std::cerr << static_cast<const char*>(errors->GetBufferPointer()) << "\n";
            }
            checkHr(hr, "D3DCompile(PSMain)");
        }

        checkHr(device_->CreateVertexShader(vertexBlob->GetBufferPointer(), vertexBlob->GetBufferSize(),
                                            nullptr, &vertexShader_),
                "CreateVertexShader");
        checkHr(device_->CreatePixelShader(pixelBlob->GetBufferPointer(), pixelBlob->GetBufferSize(),
                                           nullptr, &pixelShader_),
                "CreatePixelShader");

        constexpr D3D11_INPUT_ELEMENT_DESC inputElements[] = {
            {"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, sizeof(float) * 2, D3D11_INPUT_PER_VERTEX_DATA, 0},
        };
        checkHr(device_->CreateInputLayout(inputElements, static_cast<UINT>(std::size(inputElements)),
                                           vertexBlob->GetBufferPointer(), vertexBlob->GetBufferSize(),
                                           &inputLayout_),
                "CreateInputLayout");

        D3D11_BUFFER_DESC bufferDesc{};
        bufferDesc.ByteWidth = sizeof(Vertex) * 4096;
        bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
        bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        checkHr(device_->CreateBuffer(&bufferDesc, nullptr, &vertexBuffer_), "CreateBuffer(vertex)");

        D3D11_BLEND_DESC blendDesc{};
        blendDesc.RenderTarget[0].BlendEnable = TRUE;
        blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
        blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
        blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
        blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        checkHr(device_->CreateBlendState(&blendDesc, &blendState_), "CreateBlendState");
    }

    void createPassthrough() {
        if (!hasExtension(extensions_, XR_FB_PASSTHROUGH_EXTENSION_NAME) ||
            passthroughSystemProperties_.supportsPassthrough != XR_TRUE) {
            std::cout << "Passthrough compositor layer not available.\n";
            return;
        }

        xrCreatePassthroughFB_ = loadXrFunction<PFN_xrCreatePassthroughFB>(instance_, "xrCreatePassthroughFB", true);
        xrDestroyPassthroughFB_ = loadXrFunction<PFN_xrDestroyPassthroughFB>(instance_, "xrDestroyPassthroughFB", true);
        xrPassthroughStartFB_ = loadXrFunction<PFN_xrPassthroughStartFB>(instance_, "xrPassthroughStartFB", true);
        xrPassthroughPauseFB_ = loadXrFunction<PFN_xrPassthroughPauseFB>(instance_, "xrPassthroughPauseFB", true);
        xrCreatePassthroughLayerFB_ =
            loadXrFunction<PFN_xrCreatePassthroughLayerFB>(instance_, "xrCreatePassthroughLayerFB", true);
        xrDestroyPassthroughLayerFB_ =
            loadXrFunction<PFN_xrDestroyPassthroughLayerFB>(instance_, "xrDestroyPassthroughLayerFB", true);
        xrPassthroughLayerResumeFB_ =
            loadXrFunction<PFN_xrPassthroughLayerResumeFB>(instance_, "xrPassthroughLayerResumeFB", true);
        xrPassthroughLayerPauseFB_ =
            loadXrFunction<PFN_xrPassthroughLayerPauseFB>(instance_, "xrPassthroughLayerPauseFB", true);
        xrPassthroughLayerSetStyleFB_ =
            loadXrFunction<PFN_xrPassthroughLayerSetStyleFB>(instance_, "xrPassthroughLayerSetStyleFB", true);

        XrPassthroughCreateInfoFB passthroughCreateInfo{XR_TYPE_PASSTHROUGH_CREATE_INFO_FB};
        checkXr(xrCreatePassthroughFB_(session_, &passthroughCreateInfo, &passthrough_), "xrCreatePassthroughFB");
        checkXr(xrPassthroughStartFB_(passthrough_), "xrPassthroughStartFB");

        XrPassthroughLayerCreateInfoFB layerCreateInfo{XR_TYPE_PASSTHROUGH_LAYER_CREATE_INFO_FB};
        layerCreateInfo.passthrough = passthrough_;
        layerCreateInfo.purpose = XR_PASSTHROUGH_LAYER_PURPOSE_RECONSTRUCTION_FB;
        checkXr(xrCreatePassthroughLayerFB_(session_, &layerCreateInfo, &passthroughLayer_),
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

    void createEnvironmentDepth() {
        if (!hasExtension(extensions_, XR_META_ENVIRONMENT_DEPTH_EXTENSION_NAME) ||
            environmentDepthSystemProperties_.supportsEnvironmentDepth != XR_TRUE) {
            std::cout << "Environment depth/spatial data: not reported as supported over this Link session.\n";
            return;
        }

        xrCreateEnvironmentDepthProviderMETA_ =
            loadXrFunction<PFN_xrCreateEnvironmentDepthProviderMETA>(
                instance_, "xrCreateEnvironmentDepthProviderMETA", true);
        xrDestroyEnvironmentDepthProviderMETA_ =
            loadXrFunction<PFN_xrDestroyEnvironmentDepthProviderMETA>(
                instance_, "xrDestroyEnvironmentDepthProviderMETA", true);
        xrStartEnvironmentDepthProviderMETA_ =
            loadXrFunction<PFN_xrStartEnvironmentDepthProviderMETA>(
                instance_, "xrStartEnvironmentDepthProviderMETA", true);
        xrStopEnvironmentDepthProviderMETA_ =
            loadXrFunction<PFN_xrStopEnvironmentDepthProviderMETA>(
                instance_, "xrStopEnvironmentDepthProviderMETA", true);
        xrCreateEnvironmentDepthSwapchainMETA_ =
            loadXrFunction<PFN_xrCreateEnvironmentDepthSwapchainMETA>(
                instance_, "xrCreateEnvironmentDepthSwapchainMETA", true);
        xrDestroyEnvironmentDepthSwapchainMETA_ =
            loadXrFunction<PFN_xrDestroyEnvironmentDepthSwapchainMETA>(
                instance_, "xrDestroyEnvironmentDepthSwapchainMETA", true);
        xrEnumerateEnvironmentDepthSwapchainImagesMETA_ =
            loadXrFunction<PFN_xrEnumerateEnvironmentDepthSwapchainImagesMETA>(
                instance_, "xrEnumerateEnvironmentDepthSwapchainImagesMETA", true);
        xrGetEnvironmentDepthSwapchainStateMETA_ =
            loadXrFunction<PFN_xrGetEnvironmentDepthSwapchainStateMETA>(
                instance_, "xrGetEnvironmentDepthSwapchainStateMETA", true);
        xrAcquireEnvironmentDepthImageMETA_ =
            loadXrFunction<PFN_xrAcquireEnvironmentDepthImageMETA>(
                instance_, "xrAcquireEnvironmentDepthImageMETA", true);
        xrSetEnvironmentDepthHandRemovalMETA_ =
            loadXrFunction<PFN_xrSetEnvironmentDepthHandRemovalMETA>(
                instance_, "xrSetEnvironmentDepthHandRemovalMETA", false);

        XrEnvironmentDepthProviderCreateInfoMETA providerCreateInfo{
            XR_TYPE_ENVIRONMENT_DEPTH_PROVIDER_CREATE_INFO_META};
        const XrResult createProviderResult =
            xrCreateEnvironmentDepthProviderMETA_(session_, &providerCreateInfo, &environmentDepthProvider_);
        if (XR_FAILED(createProviderResult)) {
            std::cout << "Environment depth provider create: " << resultString(createProviderResult) << "\n";
            return;
        }

        XrEnvironmentDepthSwapchainCreateInfoMETA swapchainCreateInfo{
            XR_TYPE_ENVIRONMENT_DEPTH_SWAPCHAIN_CREATE_INFO_META};
        const XrResult createSwapchainResult =
            xrCreateEnvironmentDepthSwapchainMETA_(environmentDepthProvider_, &swapchainCreateInfo, &environmentDepthSwapchain_);
        if (XR_FAILED(createSwapchainResult)) {
            std::cout << "Environment depth swapchain create: " << resultString(createSwapchainResult) << "\n";
            return;
        }

        XrEnvironmentDepthSwapchainStateMETA state{XR_TYPE_ENVIRONMENT_DEPTH_SWAPCHAIN_STATE_META};
        const XrResult stateResult =
            xrGetEnvironmentDepthSwapchainStateMETA_(environmentDepthSwapchain_, &state);
        if (XR_SUCCEEDED(stateResult)) {
            environmentDepthWidth_ = state.width;
            environmentDepthHeight_ = state.height;
            std::cout << "Environment depth swapchain: " << state.width << "x" << state.height << "\n";
        } else {
            std::cout << "Environment depth swapchain state: " << resultString(stateResult) << "\n";
        }

        uint32_t depthImageCount = 0;
        const XrResult depthImageCountResult =
            xrEnumerateEnvironmentDepthSwapchainImagesMETA_(
                environmentDepthSwapchain_, 0, &depthImageCount, nullptr);
        if (XR_SUCCEEDED(depthImageCountResult) && depthImageCount > 0) {
            environmentDepthImages_.resize(depthImageCount, {XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR});
            const XrResult depthImagesResult =
                xrEnumerateEnvironmentDepthSwapchainImagesMETA_(
                    environmentDepthSwapchain_,
                    depthImageCount,
                    &depthImageCount,
                    reinterpret_cast<XrSwapchainImageBaseHeader*>(environmentDepthImages_.data()));
            std::cout << "Environment depth images: " << resultString(depthImagesResult)
                      << " count=" << depthImageCount << "\n";
            if (XR_SUCCEEDED(depthImagesResult)) {
                for (uint32_t i = 0; i < depthImageCount; ++i) {
                    D3D11_TEXTURE2D_DESC desc{};
                    environmentDepthImages_[i].texture->GetDesc(&desc);
                    std::cout << "  depth image " << i
                              << " tex=" << desc.Width << "x" << desc.Height
                              << " fmt=" << desc.Format
                              << " samples=" << desc.SampleDesc.Count
                              << " bind=0x" << std::hex << desc.BindFlags << std::dec
                              << "\n";
                }
            }
        } else {
            std::cout << "Environment depth image count: " << resultString(depthImageCountResult)
                      << " count=" << depthImageCount << "\n";
        }

        if (xrSetEnvironmentDepthHandRemovalMETA_ &&
            environmentDepthSystemProperties_.supportsHandRemoval == XR_TRUE) {
            XrEnvironmentDepthHandRemovalSetInfoMETA handRemoval{
                XR_TYPE_ENVIRONMENT_DEPTH_HAND_REMOVAL_SET_INFO_META};
            handRemoval.enabled = XR_TRUE;
            const XrResult handResult =
                xrSetEnvironmentDepthHandRemovalMETA_(environmentDepthProvider_, &handRemoval);
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

    void probePrivateCameraSources() {
        if (!hasExtension(extensions_, "XR_METAX1_passthrough_camera_data")) {
            return;
        }

        auto enumerateCameraSources =
            loadXrFunction<PFN_xrEnumeratePassthroughCameraSourcePropertiesMETAX1>(
                instance_, "xrEnumeratePassthroughCameraSourcePropertiesMETAX1", false);
        if (!enumerateCameraSources) {
            return;
        }

        uint32_t count = 0;
        const XrResult result = enumerateCameraSources(session_, 0, &count, nullptr);
        if (XR_SUCCEEDED(result)) {
            privateCameraSourceCount_ = count;
        }
        std::cout << "Private Link camera-source count probe: " << resultString(result)
                  << " count=" << count << "\n";
    }

    void mainLoop() {
        startTime_ = std::chrono::steady_clock::now();
        std::cout << "\nPut the headset in Quest Link now. Running for " << runSeconds_
                  << "s. You should see live passthrough with a GPU-rendered overlay.\n";

        while (!exitRequested_) {
            pollEvents();

            const auto elapsedSeconds =
                std::chrono::duration_cast<std::chrono::seconds>(
                    std::chrono::steady_clock::now() - startTime_)
                    .count();
            if (elapsedSeconds >= runSeconds_) {
                exitRequested_ = true;
            }

            if (!sessionRunning_) {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                continue;
            }

            XrFrameState frameState{XR_TYPE_FRAME_STATE};
            checkXr(xrWaitFrame(session_, nullptr, &frameState), "xrWaitFrame");
            checkXr(xrBeginFrame(session_, nullptr), "xrBeginFrame");

            std::vector<const XrCompositionLayerBaseHeader*> layers;
            XrCompositionLayerPassthroughFB passthroughLayerInfo{
                XR_TYPE_COMPOSITION_LAYER_PASSTHROUGH_FB};
            XrCompositionLayerProjection projectionLayer{XR_TYPE_COMPOSITION_LAYER_PROJECTION};

            if (frameState.shouldRender == XR_TRUE) {
                XrViewLocateInfo viewLocateInfo{XR_TYPE_VIEW_LOCATE_INFO};
                viewLocateInfo.viewConfigurationType = kViewConfig;
                viewLocateInfo.displayTime = frameState.predictedDisplayTime;
                viewLocateInfo.space = appSpace_;

                XrViewState viewState{XR_TYPE_VIEW_STATE};
                uint32_t viewCount = 0;
                const XrResult locateResult =
                    xrLocateViews(session_,
                                  &viewLocateInfo,
                                  &viewState,
                                  static_cast<uint32_t>(views_.size()),
                                  &viewCount,
                                  views_.data());
                if (XR_SUCCEEDED(locateResult) && viewCount == views_.size()) {
                    acquireEnvironmentDepth(frameState.predictedDisplayTime);
                    locateHands(frameState.predictedDisplayTime);

                    for (uint32_t eye = 0; eye < viewCount; ++eye) {
                        renderEye(eye);

                        projectionViews_[eye].pose = views_[eye].pose;
                        projectionViews_[eye].fov = views_[eye].fov;
                        projectionViews_[eye].subImage.swapchain = swapchains_[eye].handle;
                        projectionViews_[eye].subImage.imageRect.offset = {0, 0};
                        projectionViews_[eye].subImage.imageRect.extent = {
                            swapchains_[eye].width,
                            swapchains_[eye].height,
                        };
                        projectionViews_[eye].subImage.imageArrayIndex = 0;
                    }

                    if (passthroughReady_ && passthroughLayer_ != XR_NULL_HANDLE) {
                        passthroughLayerInfo.flags = XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT;
                        passthroughLayerInfo.space = XR_NULL_HANDLE;
                        passthroughLayerInfo.layerHandle = passthroughLayer_;
                        layers.push_back(
                            reinterpret_cast<const XrCompositionLayerBaseHeader*>(&passthroughLayerInfo));
                    }

                    projectionLayer.layerFlags = XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT;
                    projectionLayer.space = appSpace_;
                    projectionLayer.viewCount = static_cast<uint32_t>(projectionViews_.size());
                    projectionLayer.views = projectionViews_.data();
                    layers.push_back(reinterpret_cast<const XrCompositionLayerBaseHeader*>(&projectionLayer));
                } else {
                    std::cout << "xrLocateViews: " << resultString(locateResult)
                              << " count=" << viewCount << "\n";
                }
            }

            XrFrameEndInfo frameEndInfo{XR_TYPE_FRAME_END_INFO};
            frameEndInfo.displayTime = frameState.predictedDisplayTime;
            frameEndInfo.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
            frameEndInfo.layerCount = static_cast<uint32_t>(layers.size());
            frameEndInfo.layers = layers.empty() ? nullptr : layers.data();
            checkXr(xrEndFrame(session_, &frameEndInfo), "xrEndFrame");

            ++frameIndex_;
            if (frameIndex_ % 120 == 0) {
                std::cout << "frames=" << frameIndex_
                          << " depthFrames=" << environmentDepthFrameCount_
                          << " handFrames=" << handTrackingFrameCount_
                          << " activeHands=" << activeHandCount_
                          << " lastDepth=" << resultString(lastEnvironmentDepthResult_)
                          << "\n";
            }
        }
    }

    void pollEvents() {
        XrEventDataBuffer event{XR_TYPE_EVENT_DATA_BUFFER};
        while (xrPollEvent(instance_, &event) == XR_SUCCESS) {
            switch (event.type) {
                case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED: {
                    const auto& stateChanged =
                        *reinterpret_cast<const XrEventDataSessionStateChanged*>(&event);
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

    void handleSessionStateChanged(XrSessionState state) {
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

    void acquireEnvironmentDepth(XrTime displayTime) {
        if (!environmentDepthReady_ || !xrAcquireEnvironmentDepthImageMETA_) {
            return;
        }

        XrEnvironmentDepthImageTimestampMETA timestamp{
            XR_TYPE_ENVIRONMENT_DEPTH_IMAGE_TIMESTAMP_META};
        XrEnvironmentDepthImageMETA image{XR_TYPE_ENVIRONMENT_DEPTH_IMAGE_META};
        image.next = &timestamp;
        image.views[0].type = XR_TYPE_ENVIRONMENT_DEPTH_IMAGE_VIEW_META;
        image.views[1].type = XR_TYPE_ENVIRONMENT_DEPTH_IMAGE_VIEW_META;

        XrEnvironmentDepthImageAcquireInfoMETA acquireInfo{
            XR_TYPE_ENVIRONMENT_DEPTH_IMAGE_ACQUIRE_INFO_META};
        acquireInfo.space = appSpace_;
        acquireInfo.displayTime = displayTime;

        lastEnvironmentDepthResult_ =
            xrAcquireEnvironmentDepthImageMETA_(environmentDepthProvider_, &acquireInfo, &image);
        if (XR_SUCCEEDED(lastEnvironmentDepthResult_)) {
            ++environmentDepthFrameCount_;
            lastEnvironmentDepthNearZ_ = image.nearZ;
            lastEnvironmentDepthFarZ_ = image.farZ;
        }
    }

    void locateHands(XrTime displayTime) {
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
            locateInfo.baseSpace = appSpace_;
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

    bool projectPointToEye(uint32_t eye, XrVector3f point, Vec2& out) const {
        const XrPosef& viewPose = views_[eye].pose;
        const XrVector3f relative{
            point.x - viewPose.position.x,
            point.y - viewPose.position.y,
            point.z - viewPose.position.z,
        };
        const XrVector3f eyePoint = rotateByInverse(viewPose.orientation, relative);
        const float forward = -eyePoint.z;
        if (forward <= 0.05f) {
            return false;
        }

        const XrFovf& fov = views_[eye].fov;
        const float tanLeft = std::tan(fov.angleLeft);
        const float tanRight = std::tan(fov.angleRight);
        const float tanDown = std::tan(fov.angleDown);
        const float tanUp = std::tan(fov.angleUp);
        const float tanX = eyePoint.x / forward;
        const float tanY = eyePoint.y / forward;

        out.x = 2.0f * ((tanX - tanLeft) / (tanRight - tanLeft)) - 1.0f;
        out.y = 2.0f * ((tanY - tanDown) / (tanUp - tanDown)) - 1.0f;
        return out.x >= -1.15f && out.x <= 1.15f && out.y >= -1.15f && out.y <= 1.15f;
    }

    void appendHandPreview(std::vector<Vertex>& vertices, uint32_t eye, size_t hand, Color color) const {
        if (!handActive_[hand]) {
            return;
        }

        struct Bone {
            XrHandJointEXT a;
            XrHandJointEXT b;
        };
        static constexpr std::array<Bone, 25> bones{{
            {XR_HAND_JOINT_WRIST_EXT, XR_HAND_JOINT_PALM_EXT},
            {XR_HAND_JOINT_PALM_EXT, XR_HAND_JOINT_THUMB_METACARPAL_EXT},
            {XR_HAND_JOINT_THUMB_METACARPAL_EXT, XR_HAND_JOINT_THUMB_PROXIMAL_EXT},
            {XR_HAND_JOINT_THUMB_PROXIMAL_EXT, XR_HAND_JOINT_THUMB_DISTAL_EXT},
            {XR_HAND_JOINT_THUMB_DISTAL_EXT, XR_HAND_JOINT_THUMB_TIP_EXT},
            {XR_HAND_JOINT_PALM_EXT, XR_HAND_JOINT_INDEX_METACARPAL_EXT},
            {XR_HAND_JOINT_INDEX_METACARPAL_EXT, XR_HAND_JOINT_INDEX_PROXIMAL_EXT},
            {XR_HAND_JOINT_INDEX_PROXIMAL_EXT, XR_HAND_JOINT_INDEX_INTERMEDIATE_EXT},
            {XR_HAND_JOINT_INDEX_INTERMEDIATE_EXT, XR_HAND_JOINT_INDEX_DISTAL_EXT},
            {XR_HAND_JOINT_INDEX_DISTAL_EXT, XR_HAND_JOINT_INDEX_TIP_EXT},
            {XR_HAND_JOINT_PALM_EXT, XR_HAND_JOINT_MIDDLE_METACARPAL_EXT},
            {XR_HAND_JOINT_MIDDLE_METACARPAL_EXT, XR_HAND_JOINT_MIDDLE_PROXIMAL_EXT},
            {XR_HAND_JOINT_MIDDLE_PROXIMAL_EXT, XR_HAND_JOINT_MIDDLE_INTERMEDIATE_EXT},
            {XR_HAND_JOINT_MIDDLE_INTERMEDIATE_EXT, XR_HAND_JOINT_MIDDLE_DISTAL_EXT},
            {XR_HAND_JOINT_MIDDLE_DISTAL_EXT, XR_HAND_JOINT_MIDDLE_TIP_EXT},
            {XR_HAND_JOINT_PALM_EXT, XR_HAND_JOINT_RING_METACARPAL_EXT},
            {XR_HAND_JOINT_RING_METACARPAL_EXT, XR_HAND_JOINT_RING_PROXIMAL_EXT},
            {XR_HAND_JOINT_RING_PROXIMAL_EXT, XR_HAND_JOINT_RING_INTERMEDIATE_EXT},
            {XR_HAND_JOINT_RING_INTERMEDIATE_EXT, XR_HAND_JOINT_RING_DISTAL_EXT},
            {XR_HAND_JOINT_RING_DISTAL_EXT, XR_HAND_JOINT_RING_TIP_EXT},
            {XR_HAND_JOINT_PALM_EXT, XR_HAND_JOINT_LITTLE_METACARPAL_EXT},
            {XR_HAND_JOINT_LITTLE_METACARPAL_EXT, XR_HAND_JOINT_LITTLE_PROXIMAL_EXT},
            {XR_HAND_JOINT_LITTLE_PROXIMAL_EXT, XR_HAND_JOINT_LITTLE_INTERMEDIATE_EXT},
            {XR_HAND_JOINT_LITTLE_INTERMEDIATE_EXT, XR_HAND_JOINT_LITTLE_DISTAL_EXT},
            {XR_HAND_JOINT_LITTLE_DISTAL_EXT, XR_HAND_JOINT_LITTLE_TIP_EXT},
        }};

        auto jointIsValid = [this, hand](XrHandJointEXT joint) {
            return (handJointLocations_[hand][joint].locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT) != 0;
        };

        for (const Bone& bone : bones) {
            if (!jointIsValid(bone.a) || !jointIsValid(bone.b)) {
                continue;
            }

            Vec2 a{};
            Vec2 b{};
            if (projectPointToEye(eye, handJointLocations_[hand][bone.a].pose.position, a) &&
                projectPointToEye(eye, handJointLocations_[hand][bone.b].pose.position, b)) {
                appendLine(vertices, a, b, 0.0045f, Color{color.r, color.g, color.b, 0.72f});
            }
        }

        for (uint32_t joint = 0; joint < XR_HAND_JOINT_COUNT_EXT; ++joint) {
            if (!jointIsValid(static_cast<XrHandJointEXT>(joint))) {
                continue;
            }

            Vec2 p{};
            if (!projectPointToEye(eye, handJointLocations_[hand][joint].pose.position, p)) {
                continue;
            }

            const bool tip = joint == XR_HAND_JOINT_THUMB_TIP_EXT ||
                             joint == XR_HAND_JOINT_INDEX_TIP_EXT ||
                             joint == XR_HAND_JOINT_MIDDLE_TIP_EXT ||
                             joint == XR_HAND_JOINT_RING_TIP_EXT ||
                             joint == XR_HAND_JOINT_LITTLE_TIP_EXT;
            const float halfSize = tip ? 0.013f : 0.008f;
            appendRect(vertices,
                       p.x - halfSize,
                       p.y - halfSize,
                       p.x + halfSize,
                       p.y + halfSize,
                       tip ? Color{1.0f, 1.0f, 1.0f, 0.90f} : color);
        }
    }

    void renderEye(uint32_t eye) {
        SwapchainBundle& swapchain = swapchains_[eye];

        uint32_t imageIndex = 0;
        XrSwapchainImageAcquireInfo acquireInfo{XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};
        checkXr(xrAcquireSwapchainImage(swapchain.handle, &acquireInfo, &imageIndex),
                "xrAcquireSwapchainImage");

        XrSwapchainImageWaitInfo waitInfo{XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};
        waitInfo.timeout = XR_INFINITE_DURATION;
        checkXr(xrWaitSwapchainImage(swapchain.handle, &waitInfo), "xrWaitSwapchainImage");

        ID3D11RenderTargetView* rtv = swapchain.renderTargets[imageIndex].Get();
        context_->OMSetRenderTargets(1, &rtv, nullptr);

        D3D11_VIEWPORT viewport{};
        viewport.Width = static_cast<float>(swapchain.width);
        viewport.Height = static_cast<float>(swapchain.height);
        viewport.MinDepth = 0.0f;
        viewport.MaxDepth = 1.0f;
        context_->RSSetViewports(1, &viewport);

        constexpr float transparent[] = {0.0f, 0.0f, 0.0f, 0.0f};
        context_->ClearRenderTargetView(rtv, transparent);

        std::vector<Vertex> vertices;
        vertices.reserve(1024);

        const bool depthLive = environmentDepthFrameCount_ > 0 && XR_SUCCEEDED(lastEnvironmentDepthResult_);
        const bool handsLive = activeHandCount_ > 0 && handTrackingFrameCount_ > 0;
        const Color passthroughColor = passthroughReady_
            ? Color{0.0f, 0.78f, 1.0f, 0.78f}
            : Color{1.0f, 0.12f, 0.08f, 0.80f};
        const Color depthColor = depthLive
            ? Color{0.12f, 1.0f, 0.34f, 0.82f}
            : Color{1.0f, 0.72f, 0.05f, 0.78f};
        const Color cameraColor = privateCameraSourceCount_ > 0
            ? Color{0.95f, 0.18f, 1.0f, 0.75f}
            : Color{0.55f, 0.55f, 0.55f, 0.55f};
        const Color handStatusColor = handsLive
            ? Color{1.0f, 0.86f, 0.18f, 0.88f}
            : (handTrackingReady_
                ? Color{1.0f, 0.48f, 0.08f, 0.78f}
                : Color{0.45f, 0.45f, 0.45f, 0.55f});
        const Color leftHandColor{0.12f, 0.98f, 0.82f, 0.86f};
        const Color rightHandColor{1.0f, 0.72f, 0.12f, 0.86f};

        appendRect(vertices, -0.96f, 0.88f, 0.96f, 0.94f, passthroughColor);
        appendRect(vertices, -0.96f, -0.94f, 0.96f, -0.88f, passthroughColor);
        appendRect(vertices, -0.96f, -0.88f, -0.90f, 0.88f, depthColor);
        appendRect(vertices, 0.90f, -0.88f, 0.96f, 0.88f, cameraColor);
        appendRect(vertices, -0.88f, 0.80f, 0.88f, 0.84f, handStatusColor);

        const auto elapsed =
            std::chrono::duration<float>(std::chrono::steady_clock::now() - startTime_).count();
        appendRotatedQuad(vertices,
                          eye == 0 ? -0.08f : 0.08f,
                          0.0f,
                          0.12f + 0.015f * std::sin(elapsed * 3.0f),
                          elapsed * 1.8f,
                          depthLive ? depthColor : passthroughColor);

        const float pulse = 0.5f + 0.5f * std::sin(elapsed * 4.0f);
        appendRect(vertices, -0.26f, -0.72f, -0.10f, -0.67f, passthroughColor);
        appendRect(vertices, -0.06f, -0.72f, 0.10f, -0.67f, depthColor);
        appendRect(vertices, 0.14f, -0.72f, 0.30f, -0.67f, cameraColor);
        appendRect(vertices, 0.34f, -0.72f, 0.50f, -0.67f, handStatusColor);
        appendRect(vertices, -0.02f - pulse * 0.18f, 0.64f, 0.02f + pulse * 0.18f, 0.69f, depthColor);

        appendHandPreview(vertices, eye, 0, leftHandColor);
        appendHandPreview(vertices, eye, 1, rightHandColor);

        D3D11_MAPPED_SUBRESOURCE mapped{};
        checkHr(context_->Map(vertexBuffer_.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped), "Map(vertexBuffer)");
        std::memcpy(mapped.pData, vertices.data(), vertices.size() * sizeof(Vertex));
        context_->Unmap(vertexBuffer_.Get(), 0);

        constexpr UINT stride = sizeof(Vertex);
        constexpr UINT offset = 0;
        ID3D11Buffer* vertexBuffers[] = {vertexBuffer_.Get()};
        context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        context_->IASetInputLayout(inputLayout_.Get());
        context_->IASetVertexBuffers(0, 1, vertexBuffers, &stride, &offset);
        context_->VSSetShader(vertexShader_.Get(), nullptr, 0);
        context_->PSSetShader(pixelShader_.Get(), nullptr, 0);

        constexpr float blendFactor[] = {0.0f, 0.0f, 0.0f, 0.0f};
        context_->OMSetBlendState(blendState_.Get(), blendFactor, 0xFFFFFFFF);
        context_->Draw(static_cast<UINT>(vertices.size()), 0);

        XrSwapchainImageReleaseInfo releaseInfo{XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
        checkXr(xrReleaseSwapchainImage(swapchain.handle, &releaseInfo), "xrReleaseSwapchainImage");
    }

    void cleanup() {
        if (sessionRunning_ && session_ != XR_NULL_HANDLE) {
            xrEndSession(session_);
            sessionRunning_ = false;
        }

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

        for (auto& swapchain : swapchains_) {
            swapchain.renderTargets.clear();
            swapchain.images.clear();
            if (swapchain.handle != XR_NULL_HANDLE) {
                xrDestroySwapchain(swapchain.handle);
                swapchain.handle = XR_NULL_HANDLE;
            }
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

    std::vector<XrExtensionProperties> extensions_;
    std::vector<const char*> enabledExtensions_;

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

    ComPtr<ID3D11Device> device_;
    ComPtr<ID3D11DeviceContext> context_;
    ComPtr<ID3D11VertexShader> vertexShader_;
    ComPtr<ID3D11PixelShader> pixelShader_;
    ComPtr<ID3D11InputLayout> inputLayout_;
    ComPtr<ID3D11Buffer> vertexBuffer_;
    ComPtr<ID3D11BlendState> blendState_;

    std::vector<XrViewConfigurationView> viewConfigViews_;
    std::vector<XrView> views_;
    std::vector<XrCompositionLayerProjectionView> projectionViews_;
    std::vector<SwapchainBundle> swapchains_;

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
    uint64_t frameIndex_{0};
    int runSeconds_{30};
    std::chrono::steady_clock::time_point startTime_{};
};

int parseRunSeconds(int argc, char** argv) {
    int seconds = 30;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--seconds") == 0 && i + 1 < argc) {
            seconds = std::max(1, std::atoi(argv[++i]));
        }
    }
    return seconds;
}

}  // namespace

int main(int argc, char** argv) {
    LiveLinkApp app(parseRunSeconds(argc, argv));
    return app.run();
}
