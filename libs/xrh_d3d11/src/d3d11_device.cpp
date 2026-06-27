#include <xrh/d3d11/d3d11_renderer.h>

#include <xrh/core/checks.h>

#include <array>
#include <iostream>
#include <iterator>

namespace xrh {

void D3D11Renderer::initializeDevice(OpenXrContext& context) {
    auto getD3D11GraphicsRequirements =
        loadXrFunction<PFN_xrGetD3D11GraphicsRequirementsKHR>(
            context.instance(), "xrGetD3D11GraphicsRequirementsKHR", true);

    XrGraphicsRequirementsD3D11KHR graphicsRequirements{XR_TYPE_GRAPHICS_REQUIREMENTS_D3D11_KHR};
    checkXr(getD3D11GraphicsRequirements(context.instance(), context.systemId(), &graphicsRequirements),
            "xrGetD3D11GraphicsRequirementsKHR");
    adapterLuid_ = formatLuid(graphicsRequirements.adapterLuid);
    std::cout << "Runtime D3D11 adapter LUID: " << adapterLuid_ << "\n";

    Microsoft::WRL::ComPtr<IDXGIFactory1> factory;
    checkHr(CreateDXGIFactory1(IID_PPV_ARGS(&factory)), "CreateDXGIFactory1");

    Microsoft::WRL::ComPtr<IDXGIAdapter1> selectedAdapter;
    for (UINT adapterIndex = 0;; ++adapterIndex) {
        Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter;
        if (factory->EnumAdapters1(adapterIndex, &adapter) == DXGI_ERROR_NOT_FOUND) {
            break;
        }

        DXGI_ADAPTER_DESC1 desc{};
        if (SUCCEEDED(adapter->GetDesc1(&desc)) && sameLuid(desc.AdapterLuid, graphicsRequirements.adapterLuid)) {
            selectedAdapter = adapter;
            std::wcout << L"D3D11 adapter: " << desc.Description << L"\n";
            char narrowName[128]{};
            WideCharToMultiByte(CP_UTF8, 0, desc.Description, -1, narrowName, sizeof(narrowName), nullptr, nullptr);
            adapterName_ = narrowName;
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

void D3D11Renderer::appendReport(ProbeReport& report) const {
    report.gpuAdapterName = adapterName_;
    report.gpuAdapterLuid = adapterLuid_;
    if (!swapchains_.empty()) {
        report.eyeSwapchainWidth = static_cast<uint32_t>(swapchains_[0].width);
        report.eyeSwapchainHeight = static_cast<uint32_t>(swapchains_[0].height);
        report.eyeSwapchainImageCount = firstEyeImageCount_;
        report.eyeSwapchainFormat = static_cast<int64_t>(swapchainFormat_);
    }
    report.screenshotPaths = screenshotPaths_;
}

}  // namespace xrh
