#pragma once

#include <windows.h>

#include <openxr/openxr.h>

#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace xrh {

const char* resultName(XrResult result);
std::string resultString(XrResult result);
void checkXr(XrResult result, const char* label);
void checkHr(HRESULT hr, const char* label);
bool hasExtension(const std::vector<XrExtensionProperties>& extensions, const char* name);
bool containsAnyInterestingToken(const std::string& name);
std::string formatLuid(const LUID& luid);
bool sameLuid(const LUID& left, const LUID& right);

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

}  // namespace xrh
