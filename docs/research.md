# Quest Link OpenXR Research Notes

XR Home Suite is currently a PC-hosted OpenXR app that runs through Meta Horizon Link. The near-term goal is to replace the user-facing Link home experience while keeping the Meta/Oculus runtime underneath for compatibility with Quest Link passthrough, spatial data, depth, and private camera-path experiments.

## Current Architecture

- Host platform: native Windows executable.
- Graphics path: D3D11 selected through `XR_KHR_D3D11_enable`.
- Runtime path: Meta/Oculus OpenXR runtime through Quest Link.
- Headset target: Meta Quest 3 / Quest 3S over Link or Air Link.
- Core constraint: avoid Java, Gradle, Unity, Android Studio, and APK build loops for the PC-hosted test path.

This app should not replace or stop Meta Horizon Link services yet. Meta's runtime still owns device transport, compositor behavior, passthrough support, spatial APIs, and private extension availability.

## Verified Public Path

The current native test proves that the public compositor/depth path is viable over Link:

- `XR_FB_passthrough` can create and start a passthrough layer.
- A D3D11-rendered transparent overlay can be submitted over the passthrough layer.
- `XR_META_environment_depth` can create a depth provider and acquire live depth frames.
- `XR_EXT_hand_tracking` can create left/right hand trackers and locate live hand joints.
- The Oculus runtime can request a specific D3D11 adapter, and the app can create its device on that adapter.
- Bounded test runs with `-Seconds` exit cleanly and avoid stale native processes.

Observed local baseline:

- OpenXR runtime: Oculus OpenXR `1.205.0`.
- GPU selected by runtime: NVIDIA GeForce RTX 4070.
- Eye swapchains: `2720x2976`, 3 images per eye.
- Environment depth swapchain: `320x320`, 3 images.
- Depth acquisition result: `XR_SUCCESS`.
- Hand tracking: left/right trackers active with advancing hand-joint frames.

## Runtime Details That Matter

- `XrCompositionLayerPassthroughFB::space` must be `XR_NULL_HANDLE`.
- The passthrough composition layer uses `XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT`.
- The tested environment blend mode is `XR_ENVIRONMENT_BLEND_MODE_OPAQUE`.
- The runtime may return typeless D3D textures, such as format `27`, even when the selected swapchain format is concrete, such as `28`; render-target views should use the selected swapchain format.
- Environment-depth images must be enumerated and initialized as `XR_TYPE_ENVIRONMENT_DEPTH_IMAGE_VIEW_META`.

## Private Camera Extension Status

`XR_METAX1_passthrough_camera_data` is visible on the tested runtime. A count-only probe through `xrEnumeratePassthroughCameraSourcePropertiesMETAX1` returned `XR_SUCCESS` with 2 camera sources.

The app intentionally does not read raw camera frames yet. The extension is private/undocumented, so raw acquisition needs ABI mapping before any pointer-sized structs, buffers, image handles, or frame metadata are trusted. Guessing the layout could crash the runtime or read invalid memory.

Safe investigation boundary:

- OK: enumerate extension presence.
- OK: load private function pointers defensively.
- OK: perform count-only source enumeration.
- Not OK yet: allocate guessed private structs for frame acquisition.
- Not OK yet: dereference private frame pointers or assume image layout.

## Product Direction

The first useful hub should use the proven public path:

- richer rendered scene over passthrough,
- controller or hand ray interaction,
- spatial menu/launcher surface,
- 3D preview panel,
- frame/performance overlay,
- in-world hand previews and hand-driven pointing,
- environment-depth-driven occlusion or spatial effects.

Private camera-frame work should remain a separate research track until the ABI is understood well enough to isolate crashes and validate memory layouts.

## Useful References

- [Meta passthrough](https://developers.meta.com/horizon/documentation/native/android/mobile-passthrough/)
- [Meta Depth API](https://developers.meta.com/horizon/documentation/native/android/mobile-depth/)
- [Meta passthrough over Link](https://developers.meta.com/horizon/documentation/native/android/mobile-passthrough-over-link/)
