#include <xrh/core/checks.h>

#include <algorithm>
#include <cctype>
#include <cstring>
#include <iomanip>

namespace xrh {

namespace {

std::string toLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

}  // namespace

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

}  // namespace xrh
