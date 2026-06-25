#pragma once

#include <xrh/core/openxr_platform.h>
#include <xrh/core/overlay_state.h>
#include <xrh/core/probe_report.h>
#include <xrh/openxr/openxr_context.h>

#include <chrono>
#include <string>
#include <vector>

namespace xrh {

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
    std::vector<Microsoft::WRL::ComPtr<ID3D11RenderTargetView>> renderTargets;
};

class D3D11Renderer {
public:
    void initializeDevice(OpenXrContext& context);
    void createRenderResources(OpenXrContext& context);
    bool locateViews(OpenXrContext& context, XrTime displayTime);
    void renderEyes(const OverlayState& state, const std::chrono::steady_clock::time_point& startTime);
    void cleanup();
    void appendReport(ProbeReport& report) const;

    ID3D11Device* device() const { return device_.Get(); }
    const std::vector<XrCompositionLayerProjectionView>& projectionViews() const { return projectionViews_; }

private:
    void createShaders();
    void renderEye(uint32_t eye, const OverlayState& state, const std::chrono::steady_clock::time_point& startTime);
    bool projectPointToEye(uint32_t eye, XrVector3f point, Vec2& out) const;
    void appendHandPreview(std::vector<Vertex>& vertices, uint32_t eye, size_t hand, Color color,
                           const OverlayState& state) const;

    Microsoft::WRL::ComPtr<ID3D11Device> device_;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context_;
    Microsoft::WRL::ComPtr<ID3D11VertexShader> vertexShader_;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> pixelShader_;
    Microsoft::WRL::ComPtr<ID3D11InputLayout> inputLayout_;
    Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer_;
    Microsoft::WRL::ComPtr<ID3D11BlendState> blendState_;

    std::vector<XrViewConfigurationView> viewConfigViews_;
    std::vector<XrView> views_;
    std::vector<XrCompositionLayerProjectionView> projectionViews_;
    std::vector<SwapchainBundle> swapchains_;

    std::string adapterName_;
    std::string adapterLuid_;
};

}  // namespace xrh
