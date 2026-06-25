# XR Link Capability Matrix

This note tracks the Meta Horizon Link developer toggles that are currently enabled on the test machine and the OpenXR/native-PC capabilities we can safely use from `apps/openxr_probe`.

## Why Passthrough Is Split

Meta exposes two different paths:

- `XR_FB_passthrough`: compositor-owned passthrough visualization. It is built for comfort, privacy, and stable compositor timing. Apps create a placeholder passthrough layer; the XR compositor replaces it with the real passthrough rendition. Apps do not receive camera pixels through this path.
- Passthrough Camera API: explicit camera-data access for CV/ML and custom processing. This is separate, permissioned Device User Data. Official public routes are Android Camera2, Unity/MRUK `PassthroughCameraAccess`, Spatial SDK, and newer Unreal support. Our native Windows/OpenXR path only sees the private `XR_METAX1_passthrough_camera_data` extension so far.

So: "Passthrough over Link" lets the host app see/use passthrough visually; "Passthrough Camera API permissions" is the separate path intended to expose camera frames to apps that use the supported camera APIs.

## Current Meta Horizon Link Toggles

- Developer Runtime Features: on.
- Passthrough over Meta Horizon Link: on.
- Passthrough Camera API permissions: on.
- Spatial Data over Meta Horizon Link: on.
- Eye tracking over Link: off, ignored.
- Natural Facial Expressions over Link: off, ignored.

## Current Native App Extension Set

Required:

- `XR_KHR_D3D11_enable`

Optional when available:

- `XR_FB_passthrough`
- `XR_FB_triangle_mesh`
- `XR_META_environment_depth`
- `XR_FB_scene`
- `XR_FB_spatial_entity`
- `XR_META_spatial_entity_mesh`
- `XR_EXT_hand_tracking`
- `XR_METAX1_passthrough_camera_data` private count-only probe

## Observed Local Runtime Baseline

- Runtime: Oculus OpenXR `1.205.0`.
- GPU selected by runtime: NVIDIA GeForce RTX 4070.
- Eye swapchains: `2720x2976`, 3 images per eye.
- Eye texture format returned by runtime: typeless `27`; render-target view uses selected format `28`.
- Environment depth swapchain: `320x320`, 3 images.
- Passthrough capabilities observed: `0x3`, meaning base passthrough + color passthrough. `XR_PASSTHROUGH_CAPABILITY_LAYER_DEPTH_BIT_FB` was not present in that value.
- Private passthrough camera source count: `2`.
- Hand tracking: left and right trackers created; live joint frames observed.

## Capability Details

### PC Rendering: `XR_KHR_D3D11_enable`

- Purpose: bind the OpenXR session to a native D3D11 device selected by the runtime.
- Current quality ceiling: runtime-recommended eye swapchains, observed at `2720x2976` per eye.
- App control: render resolution follows `xrEnumerateViewConfigurationViews`; do not hardcode it. The app can choose supported swapchain formats but must render on the adapter LUID requested by the runtime.
- Not controlled here: Link encode bitrate, transport mode, Meta Link global render-resolution sliders, and headset refresh-rate policy. Those are Meta Link/runtime settings, not direct app API knobs in this prototype.

### Visual Passthrough: `XR_FB_passthrough`

- Purpose: compositor-layer passthrough visualization.
- Pixel access: none. Meta documents that apps cannot access images/videos through this path.
- Current app mode: reconstruction layer (`XR_PASSTHROUGH_LAYER_PURPOSE_RECONSTRUCTION_FB`) behind a transparent D3D11 overlay.
- Current runtime capabilities: base + color passthrough.
- Styling knobs available in `XR_FB_passthrough`: opacity, edge color, mono color map, brightness, contrast, saturation.
- Quality/resolution knobs: no app-visible camera resolution control. The compositor owns camera processing and final passthrough composition.
- Depth blending limitation: Meta documents no depth-based passthrough blending control in this API path. We need our own depth-aware virtual content pipeline if we want better occlusion.
- Link constraint: Meta says Color Passthrough over Link should have effective USB bandwidth of at least `2 Gbps`; Link visual/performance characteristics can differ from on-device runs.

### Surface-Projected Passthrough: `XR_FB_triangle_mesh`

- Purpose: define meshes for projected passthrough surfaces instead of pure automatic reconstruction.
- Current app status: extension enabled, no mesh instances created yet.
- Quality knobs: mesh density and update frequency are app choices; no fixed "max quality" value is exposed in the core struct. Runtime can reject allocations if resources are exceeded.
- Useful for: stable passthrough on known surfaces, portal/window effects, passthrough constrained to room geometry.

### Passthrough Camera API: public PCA vs private native extension

Official public PCA constraints from Meta docs:

- Supported devices: Quest 3 / Quest 3S.
- Official camera access basis: Android Camera2, Unity/MRUK `PassthroughCameraAccess`, Spatial SDK; Unreal extension exists in newer releases.
- Camera source: left or right forward-facing RGB camera.
- Camera data includes: texture/data, intrinsics, extrinsics, timestamps.
- Supported Unity/MRUK resolutions include:
  - `320x240`
  - `640x360`
  - `640x480`
  - `720x480`
  - `720x576`
  - `800x600`
  - `1024x576`
  - `1280x720`
  - `1280x960`
  - `1280x1080`
  - `1280x1280`
- Documented max: `1280x1280`.
- Documented stream rate: `60 Hz`.
- Documented format: internal `YUV420`.
- Documented latency: `20-40 ms`.
- Documented overhead: about `1-2%` GPU overhead per streamed camera and about `45 MB` memory overhead.
- Important quality rule: do not blindly pick the largest resolution. Pick a tested concrete resolution or aspect ratio because supported resolutions can change.

Native Windows/OpenXR status in this repo:

- `XR_METAX1_passthrough_camera_data` is private/undocumented.
- Current safe probe only loads `xrEnumeratePassthroughCameraSourcePropertiesMETAX1` and asks for the source count.
- Observed source count: `2`.
- Raw frames are not implemented because struct layouts, image handles, buffers, timestamps, permissions, and lifecycle are not public for this PC-native path.
- Treat any raw-frame work as a separate ABI-research branch, not part of the normal hub prototype.

### Environment Depth: `XR_META_environment_depth`

- Purpose: live depth maps for dynamic real-world occlusion/raycast-style spatial effects.
- Current observed size: `320x320`, 3 images.
- Device/API notes: Meta documents Depth API for Quest 3-class MR; Unity page says the feature has runtime overhead even if textures are not consumed while enabled.
- Effective range: Meta documents unreliable estimates closer than about `0.2 m`.
- Current app controls:
  - `--no-depth`
  - `--depth-hz N`
  - default poll cap: `30 Hz`
- Current runtime support:
  - `supportsEnvironmentDepth = true`
  - `supportsHandRemoval = false` on this Link session
- Quality knobs: query `xrGetEnvironmentDepthSwapchainStateMETA` each run for width/height. Do not assume a fixed size. Use `nearZ`, `farZ`, `minDepth`, and `maxDepth` metadata for correct interpretation.
- Use cases: occlusion of virtual content by moving real objects, approximate physical-surface placement, hand/body/person/pet dynamic occlusion.

### Spatial Data / Scene Model: `XR_FB_scene`, `XR_FB_spatial_entity`, `XR_META_spatial_entity_mesh`

- Purpose: access OS-managed room/scene data, anchors, semantic labels, 2D boundaries, 3D bounding boxes, room layout, and meshes.
- Current app status: extensions enabled, no scene query implementation yet.
- User/privacy model: scene data is user-controlled spatial data and requires permission.
- Link constraint: Meta documents that Space Setup cannot be performed over Link; scan/setup must happen on-device before the app loads scene data over Link.
- Multi-room note: Meta docs say the OS maintains up to `15` rooms.
- Scene API data shape:
  - scene anchors/spatial entities
  - persistent UUIDs
  - component support/status
  - semantic labels
  - 2D bounds
  - 3D bounds
  - room layout floor/ceiling/wall UUID references
  - optional mesh data through `XR_META_spatial_entity_mesh`
- Quality knobs: mostly OS-captured, not app-captured over Link. The app can choose how densely to render or simplify returned meshes, but does not control the scan quality from this native Link session.

### Hand Tracking: `XR_EXT_hand_tracking`

- Purpose: hand pose/joint input without raw camera frames.
- Current app status: left/right trackers created; projected skeleton preview implemented.
- Data size: `XR_HAND_JOINT_COUNT_EXT = 26` joints per hand.
- Data fields: per-joint pose, radius, and location validity flags.
- Current app controls:
  - `--no-hands`
  - `--hand-hz N`
  - default poll cap: `30 Hz`
- Runtime max: not exposed as a single quality number in `XR_EXT_hand_tracking`. Meta native docs mention a high-frequency hand tracking manifest hint for Android apps, but that does not directly apply to this PC-native executable.
- Known limitations from Meta docs: occlusion, noise, lighting sensitivity, and controller/hand concurrency are not guaranteed. Our runtime also exposes `XR_META_simultaneous_hands_and_controllers`, but this app does not enable/use it yet.

## Practical Defaults For This Repo

- Default test: keep depth/hands capped at `30 Hz`.
- Lag isolation:
  - render + passthrough only: `python -m xrhs run-probe -- --seconds 60 --no-depth --no-hands`
  - hands only: `python -m xrhs run-probe -- --seconds 60 --no-depth --hand-hz 15`
  - depth + hands conservative: `python -m xrhs run-probe -- --seconds 60 --depth-hz 15 --hand-hz 15`
- Do not assume raw camera access is available in native Windows OpenXR until `XR_METAX1_passthrough_camera_data` is mapped safely.
- Do not treat passthrough compositor quality as app-controlled camera quality.
- For actual CV/ML camera input today, use public PCA paths first unless native ABI research is the explicit goal.

## Sources

- Meta native passthrough: https://developers.meta.com/horizon/documentation/native/android/mobile-passthrough/
- Meta passthrough over Link: https://developers.meta.com/horizon/documentation/native/android/mobile-passthrough-over-link/
- Meta PCA Unity docs: https://developers.meta.com/horizon/documentation/unity/unity-pca-documentation/
- Meta PCA overview: https://developers.meta.com/horizon/documentation/spatial-sdk/spatial-sdk-pca-overview/
- Meta native depth: https://developers.meta.com/horizon/documentation/native/android/mobile-depth/
- Meta Unity depth runtime notes: https://developers.meta.com/horizon/documentation/unity/unity-depthapi-xr-oculus/
- Meta native hand tracking: https://developers.meta.com/horizon/documentation/native/android/mobile-hand-tracking/
- Meta native scene overview: https://developers.meta.com/horizon/documentation/native/android/openxr-scene-overview/
- Meta scene API reference: https://developers.meta.com/horizon/documentation/native/android/mobile-scene-api-ref/
- Khronos OpenXR headers used locally after dependency sync: `C:\XRHomeSuite\deps\openxr\include\openxr\openxr.h`
