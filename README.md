# XR Home Suite

Native OpenXR experiments for PC-hosted Meta Quest Link mixed reality.

The first module is a lightweight Windows-native Quest Link MR live test. It avoids Java, Gradle, Unity, and APK deployment, so it is much easier on the CPU while proving the actual PC/OpenXR path.

## Current test

`src/live_link_app.cpp`:

- enumerates Meta/OpenXR passthrough, spatial, depth, and camera-related extensions,
- creates a D3D11 OpenXR session on the GPU selected by the active OpenXR runtime,
- displays live passthrough through the public `XR_FB_passthrough` compositor layer,
- renders a transparent GPU overlay over passthrough,
- starts `XR_META_environment_depth` and acquires live spatial/depth frames,
- performs a count-only private Link camera-source probe through `XR_METAX1_passthrough_camera_data`.

Raw private camera frame acquisition is intentionally not attempted yet. The count-only probe confirms that Link exposes camera sources, but the private camera-frame ABI still needs to be mapped before reading frames safely.

## Requirements

- Windows 10/11.
- Meta Horizon Link installed.
- Meta Quest 3/3S connected through Link or Air Link.
- Meta Horizon Link set as the active OpenXR runtime.
- Visual Studio Build Tools 2022 with the MSVC x64 toolchain.
- Meta Horizon Link developer toggles enabled:
  - Developer Runtime Features.
  - Passthrough over Meta Horizon Link.
  - Spatial Data over Meta Horizon Link.
  - Passthrough Camera API permissions for later raw camera work.

## Build and run

From the repo root:

```powershell
powershell -ExecutionPolicy Bypass -File .\build-native.ps1 -Run -Seconds 30
```

The build script downloads the current Khronos OpenXR headers plus the OpenXR loader NuGet package into `third_party/`, then compiles a native x64 D3D11 executable into `build/`.

## In-headset signals

- Cyan frame: passthrough compositor layer is running.
- Left green bar: environment-depth frames are being acquired.
- Right magenta bar: private Link camera-source count returned at least one source.

## Observed local baseline

Observed on the initial development machine with Quest Link active:

- Runtime: Oculus OpenXR `1.205.0`.
- GPU selected by runtime: NVIDIA GeForce RTX 4070.
- Color swapchains: `2720x2976`, 3 images per eye.
- Passthrough: `XR_FB_passthrough`, running.
- Spatial/depth: `XR_META_environment_depth`, `320x320`, 3 depth images.
- Depth acquisition: `XR_SUCCESS`.
- Private Link camera-source count: `2`.

## Useful Meta docs

- [Passthrough](https://developers.meta.com/horizon/documentation/native/android/mobile-passthrough/)
- [Depth API](https://developers.meta.com/horizon/documentation/native/android/mobile-depth/)
- [Passthrough over Link](https://developers.meta.com/horizon/documentation/native/android/mobile-passthrough-over-link/)
