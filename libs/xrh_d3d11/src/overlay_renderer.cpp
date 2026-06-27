#include <xrh/d3d11/d3d11_renderer.h>

#include <xrh/core/checks.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <iostream>
#include <limits>

namespace xrh {

namespace {

constexpr float kPi = 3.14159265358979323846f;

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
        corners[i] = Vertex{cx + std::cos(localAngle) * radius,
                            cy + std::sin(localAngle) * radius,
                            color.r,
                            color.g,
                            color.b,
                            color.a};
    }
    vertices.insert(vertices.end(), {corners[0], corners[1], corners[2], corners[0], corners[2], corners[3]});
}

XrVector3f rotateByInverse(const XrQuaternionf& q, XrVector3f v) {
    const XrQuaternionf inverse{-q.x, -q.y, -q.z, q.w};
    const XrVector3f u{inverse.x, inverse.y, inverse.z};
    const float s = inverse.w;
    const float dotUV = u.x * v.x + u.y * v.y + u.z * v.z;
    const float dotUU = u.x * u.x + u.y * u.y + u.z * u.z;
    const XrVector3f cross{u.y * v.z - u.z * v.y, u.z * v.x - u.x * v.z, u.x * v.y - u.y * v.x};
    return XrVector3f{
        2.0f * dotUV * u.x + (s * s - dotUU) * v.x + 2.0f * s * cross.x,
        2.0f * dotUV * u.y + (s * s - dotUU) * v.y + 2.0f * s * cross.y,
        2.0f * dotUV * u.z + (s * s - dotUU) * v.z + 2.0f * s * cross.z,
    };
}

}  // namespace

bool D3D11Renderer::locateViews(OpenXrContext& context, XrTime displayTime) {
    XrViewLocateInfo viewLocateInfo{XR_TYPE_VIEW_LOCATE_INFO};
    viewLocateInfo.viewConfigurationType = kViewConfig;
    viewLocateInfo.displayTime = displayTime;
    viewLocateInfo.space = context.appSpace();

    XrViewState viewState{XR_TYPE_VIEW_STATE};
    uint32_t viewCount = 0;
    const XrResult result = xrLocateViews(context.session(),
                                          &viewLocateInfo,
                                          &viewState,
                                          static_cast<uint32_t>(views_.size()),
                                          &viewCount,
                                          views_.data());
    if (XR_FAILED(result) || viewCount != views_.size()) {
        std::cout << "xrLocateViews: " << resultString(result) << " count=" << viewCount << "\n";
        return false;
    }
    return true;
}

bool D3D11Renderer::projectPointToEye(uint32_t eye, XrVector3f point, Vec2& out) const {
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
    const float tanX = eyePoint.x / forward;
    const float tanY = eyePoint.y / forward;
    out.x = 2.0f * ((tanX - std::tan(fov.angleLeft)) / (std::tan(fov.angleRight) - std::tan(fov.angleLeft))) - 1.0f;
    out.y = 2.0f * ((tanY - std::tan(fov.angleDown)) / (std::tan(fov.angleUp) - std::tan(fov.angleDown))) - 1.0f;
    return out.x >= -1.15f && out.x <= 1.15f && out.y >= -1.15f && out.y <= 1.15f;
}

void D3D11Renderer::appendHandPreview(std::vector<Vertex>& vertices, uint32_t eye, size_t hand, Color color,
                                      const OverlayState& state) const {
    if (!state.handActive || !state.handJointLocations || !(*state.handActive)[hand]) {
        return;
    }

    struct Bone { XrHandJointEXT a; XrHandJointEXT b; };
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

    const auto& joints = (*state.handJointLocations)[hand];
    auto valid = [&joints](XrHandJointEXT joint) {
        return (joints[joint].locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT) != 0;
    };
    for (const Bone& bone : bones) {
        Vec2 a{};
        Vec2 b{};
        if (valid(bone.a) && valid(bone.b) &&
            projectPointToEye(eye, joints[bone.a].pose.position, a) &&
            projectPointToEye(eye, joints[bone.b].pose.position, b)) {
            appendLine(vertices, a, b, 0.0045f, Color{color.r, color.g, color.b, 0.72f});
        }
    }

    for (uint32_t joint = 0; joint < XR_HAND_JOINT_COUNT_EXT; ++joint) {
        Vec2 p{};
        if (!valid(static_cast<XrHandJointEXT>(joint)) || !projectPointToEye(eye, joints[joint].pose.position, p)) {
            continue;
        }
        const bool tip = joint == XR_HAND_JOINT_THUMB_TIP_EXT ||
                         joint == XR_HAND_JOINT_INDEX_TIP_EXT ||
                         joint == XR_HAND_JOINT_MIDDLE_TIP_EXT ||
                         joint == XR_HAND_JOINT_RING_TIP_EXT ||
                         joint == XR_HAND_JOINT_LITTLE_TIP_EXT;
        const float halfSize = tip ? 0.013f : 0.008f;
        appendRect(vertices, p.x - halfSize, p.y - halfSize, p.x + halfSize, p.y + halfSize,
                   tip ? Color{1.0f, 1.0f, 1.0f, 0.90f} : color);
    }
}

void D3D11Renderer::renderEyes(const OverlayState& state, const std::chrono::steady_clock::time_point& startTime,
                               uint64_t frameIndex) {
    for (uint32_t eye = 0; eye < views_.size(); ++eye) {
        renderEye(eye, state, startTime, frameIndex);
        projectionViews_[eye].pose = views_[eye].pose;
        projectionViews_[eye].fov = views_[eye].fov;
        projectionViews_[eye].subImage.swapchain = swapchains_[eye].handle;
        projectionViews_[eye].subImage.imageRect.offset = {0, 0};
        projectionViews_[eye].subImage.imageRect.extent = {swapchains_[eye].width, swapchains_[eye].height};
        projectionViews_[eye].subImage.imageArrayIndex = 0;
    }
}

void D3D11Renderer::renderEye(uint32_t eye, const OverlayState& state,
                              const std::chrono::steady_clock::time_point& startTime,
                              uint64_t frameIndex) {
    SwapchainBundle& swapchain = swapchains_[eye];
    uint32_t imageIndex = 0;
    XrSwapchainImageAcquireInfo acquireInfo{XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};
    checkXr(xrAcquireSwapchainImage(swapchain.handle, &acquireInfo, &imageIndex), "xrAcquireSwapchainImage");
    XrSwapchainImageWaitInfo waitInfo{XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};
    waitInfo.timeout = XR_INFINITE_DURATION;
    checkXr(xrWaitSwapchainImage(swapchain.handle, &waitInfo), "xrWaitSwapchainImage");

    ID3D11RenderTargetView* rtv = swapchain.renderTargets[imageIndex].Get();
    context_->OMSetRenderTargets(1, &rtv, nullptr);
    D3D11_VIEWPORT viewport{};
    viewport.Width = static_cast<float>(swapchain.width);
    viewport.Height = static_cast<float>(swapchain.height);
    viewport.MaxDepth = 1.0f;
    context_->RSSetViewports(1, &viewport);
    constexpr float transparent[] = {0.0f, 0.0f, 0.0f, 0.0f};
    context_->ClearRenderTargetView(rtv, transparent);

    std::vector<Vertex> vertices;
    vertices.reserve(1024);
    const Color passthrough = state.passthroughReady ? Color{0.0f, 0.78f, 1.0f, 0.78f} : Color{1.0f, 0.12f, 0.08f, 0.80f};
    const Color depth = state.depthLive ? Color{0.12f, 1.0f, 0.34f, 0.82f} : Color{1.0f, 0.72f, 0.05f, 0.78f};
    const Color camera = state.privateCameraSourceCount > 0 ? Color{0.95f, 0.18f, 1.0f, 0.75f} : Color{0.55f, 0.55f, 0.55f, 0.55f};
    const Color hands = state.handsLive ? Color{1.0f, 0.86f, 0.18f, 0.88f} :
        (state.handTrackingReady ? Color{1.0f, 0.48f, 0.08f, 0.78f} : Color{0.45f, 0.45f, 0.45f, 0.55f});

    appendRect(vertices, -0.96f, 0.88f, 0.96f, 0.94f, passthrough);
    appendRect(vertices, -0.96f, -0.94f, 0.96f, -0.88f, passthrough);
    appendRect(vertices, -0.96f, -0.88f, -0.90f, 0.88f, depth);
    appendRect(vertices, 0.90f, -0.88f, 0.96f, 0.88f, camera);
    appendRect(vertices, -0.88f, 0.80f, 0.88f, 0.84f, hands);

    const auto elapsed = std::chrono::duration<float>(std::chrono::steady_clock::now() - startTime).count();
    appendRotatedQuad(vertices, eye == 0 ? -0.08f : 0.08f, 0.0f, 0.12f + 0.015f * std::sin(elapsed * 3.0f),
                      elapsed * 1.8f, state.depthLive ? depth : passthrough);
    const float pulse = 0.5f + 0.5f * std::sin(elapsed * 4.0f);
    appendRect(vertices, -0.26f, -0.72f, -0.10f, -0.67f, passthrough);
    appendRect(vertices, -0.06f, -0.72f, 0.10f, -0.67f, depth);
    appendRect(vertices, 0.14f, -0.72f, 0.30f, -0.67f, camera);
    appendRect(vertices, 0.34f, -0.72f, 0.50f, -0.67f, hands);
    appendRect(vertices, -0.02f - pulse * 0.18f, 0.64f, 0.02f + pulse * 0.18f, 0.69f, depth);
    appendHandPreview(vertices, eye, 0, Color{0.12f, 0.98f, 0.82f, 0.86f}, state);
    appendHandPreview(vertices, eye, 1, Color{1.0f, 0.72f, 0.12f, 0.86f}, state);

    D3D11_MAPPED_SUBRESOURCE mapped{};
    checkHr(context_->Map(vertexBuffer_.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped), "Map(vertexBuffer)");
    std::memcpy(mapped.pData, vertices.data(), vertices.size() * sizeof(Vertex));
    context_->Unmap(vertexBuffer_.Get(), 0);

    constexpr UINT stride = sizeof(Vertex);
    constexpr UINT offset = 0;
    ID3D11Buffer* buffers[] = {vertexBuffer_.Get()};
    context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context_->IASetInputLayout(inputLayout_.Get());
    context_->IASetVertexBuffers(0, 1, buffers, &stride, &offset);
    context_->RSSetState(rasterizerState_.Get());
    context_->VSSetShader(vertexShader_.Get(), nullptr, 0);
    context_->PSSetShader(pixelShader_.Get(), nullptr, 0);
    constexpr float blendFactor[] = {0.0f, 0.0f, 0.0f, 0.0f};
    context_->OMSetBlendState(nullptr, blendFactor, 0xFFFFFFFF);
    context_->Draw(static_cast<UINT>(vertices.size()), 0);

    const double elapsedSeconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - startTime).count();
    captureEyeIfNeeded(eye, frameIndex, elapsedSeconds, swapchain.images[imageIndex].texture);

    XrSwapchainImageReleaseInfo releaseInfo{XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
    checkXr(xrReleaseSwapchainImage(swapchain.handle, &releaseInfo), "xrReleaseSwapchainImage");
}

}  // namespace xrh
