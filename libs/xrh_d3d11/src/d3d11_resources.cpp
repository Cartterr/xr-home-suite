#include <xrh/d3d11/d3d11_renderer.h>

#include <xrh/core/checks.h>

#include <array>
#include <cstring>
#include <d3dcompiler.h>
#include <iostream>
#include <iterator>

namespace xrh {

void D3D11Renderer::createRenderResources(OpenXrContext& context) {
    uint32_t viewCount = 0;
    checkXr(xrEnumerateViewConfigurationViews(context.instance(), context.systemId(), kViewConfig, 0, &viewCount, nullptr),
            "xrEnumerateViewConfigurationViews(count)");

    viewConfigViews_.resize(viewCount, {XR_TYPE_VIEW_CONFIGURATION_VIEW});
    checkXr(xrEnumerateViewConfigurationViews(
                context.instance(), context.systemId(), kViewConfig, viewCount, &viewCount, viewConfigViews_.data()),
            "xrEnumerateViewConfigurationViews(list)");
    views_.resize(viewCount, {XR_TYPE_VIEW});
    projectionViews_.resize(viewCount, {XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW});

    uint32_t formatCount = 0;
    checkXr(xrEnumerateSwapchainFormats(context.session(), 0, &formatCount, nullptr),
            "xrEnumerateSwapchainFormats(count)");
    std::vector<int64_t> formats(formatCount);
    checkXr(xrEnumerateSwapchainFormats(context.session(), formatCount, &formatCount, formats.data()),
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

        XrSwapchainCreateInfo createInfo{XR_TYPE_SWAPCHAIN_CREATE_INFO};
        createInfo.arraySize = 1;
        createInfo.mipCount = 1;
        createInfo.faceCount = 1;
        createInfo.format = selectedFormat;
        createInfo.width = config.recommendedImageRectWidth;
        createInfo.height = config.recommendedImageRectHeight;
        createInfo.sampleCount = config.recommendedSwapchainSampleCount;
        createInfo.usageFlags = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;
        checkXr(xrCreateSwapchain(context.session(), &createInfo, &swapchain.handle), "xrCreateSwapchain");

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
            D3D11_RENDER_TARGET_VIEW_DESC rtvDesc{};
            rtvDesc.Format = selectedFormat;
            rtvDesc.ViewDimension = textureDesc.SampleDesc.Count > 1
                ? D3D11_RTV_DIMENSION_TEXTURE2DMS
                : D3D11_RTV_DIMENSION_TEXTURE2D;
            checkHr(device_->CreateRenderTargetView(
                        swapchain.images[image].texture, &rtvDesc, &swapchain.renderTargets[image]),
                    "CreateRenderTargetView");
        }

        std::cout << "Eye " << eye << " swapchain: " << swapchain.width << "x" << swapchain.height
                  << " images=" << imageCount << " format=" << static_cast<int>(selectedFormat) << "\n";
    }

    createShaders();
}

void D3D11Renderer::createShaders() {
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

    Microsoft::WRL::ComPtr<ID3DBlob> vertexBlob;
    Microsoft::WRL::ComPtr<ID3DBlob> pixelBlob;
    Microsoft::WRL::ComPtr<ID3DBlob> errors;
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
                                       vertexBlob->GetBufferPointer(), vertexBlob->GetBufferSize(), &inputLayout_),
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

void D3D11Renderer::cleanup() {
    for (auto& swapchain : swapchains_) {
        swapchain.renderTargets.clear();
        swapchain.images.clear();
        if (swapchain.handle != XR_NULL_HANDLE) {
            xrDestroySwapchain(swapchain.handle);
            swapchain.handle = XR_NULL_HANDLE;
        }
    }
}

}  // namespace xrh
