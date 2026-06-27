# Unreal Engine Gate

This repo must not commit a generated Unreal project until the selected engine can be opened or built locally.

## Target Stack

- Unreal Engine: `5.7.4`
- Meta XR / Horizon Integration SDK: `201.0`
- Install root: `C:\XRHomeSuite\engines`
- Comfort/default path: D3D12, SM6, Forward Renderer, MSAA
- RTX validation path: DLSS/DLAA/Reflex after the base Link scene is stable
- Product boundary: no raw private camera dependency

## Current Local Gate

Source access is unlocked on this machine:

- GitHub auth works for user `Cartterr`.
- `EpicGames/UnrealEngine` access is available.
- `Oculus-VR/UnrealEngine` access is available.
- `NvRTX/UnrealEngine` access is available.

Local stock Unreal source install:

- Path: `C:\XRHomeSuite\engines\UE_5.7.4`
- Source tag: `5.7.4-release`
- Source commit: `260bb2e1c5610b31c63a36206eedd289409c5f11`
- Dependencies: synced with `GitDependencies.exe` using Win64-focused excludes and cache at `C:\XRHomeSuite\cache\ue-gitdeps`
- UnrealBuildTool: built under `Engine\Binaries\DotNET\UnrealBuildTool`
- Solution: `C:\XRHomeSuite\engines\UE_5.7.4\UE5.sln`
- Editor binary: built successfully at `C:\XRHomeSuite\engines\UE_5.7.4\Engine\Binaries\Win64\UnrealEditor.exe`
- Editor build log: `C:\XRHomeSuite\artifacts\logs\ue-5.7.4-build-unrealeditor-resume.log`
- Meta XR plugin: installed at `C:\XRHomeSuite\engines\UE_5.7.4\Engine\Plugins\Marketplace\MetaXR`
- Meta XR plugin descriptor: `MetaXR\OculusXR.uplugin`, version `1.201.0`

Known compatible source paths:

- Stock baseline: Epic `5.7.4-release`, matching Meta XR / Horizon Integration SDK `201.0` requirements.
- Meta validation path: `Oculus-VR/UnrealEngine` branch `oculus-5.7`; visible 201.0 tag is `oculus-5.7.3-release-1.201.0-v201.0`.
- RTX experiment path: `NvRTX/UnrealEngine` tag `nvrtx-5.7.4.0`.

`python -m xrhs unreal-doctor` reports source access, local engine markers, the full editor binary, and the Meta plugin staging state.

## Access Verification Commands

1. Verify Epic source access:
   `git ls-remote https://github.com/EpicGames/UnrealEngine.git HEAD`
2. Verify Meta/Oculus source access:
   `git ls-remote https://github.com/Oculus-VR/UnrealEngine.git HEAD`
3. Verify NVIDIA NvRTX source access:
   `git ls-remote https://github.com/NvRTX/UnrealEngine.git HEAD`
4. Re-run:
   `python -m xrhs unreal-doctor`

Private Unreal repositories usually return `404` when the GitHub account has not completed account linking, organization invite acceptance, or vendor-specific source access.

## Build Notes

- Keep all engine source, dependencies, intermediates, and caches on `C:\XRHomeSuite`, never under the repo.
- Use direct executables from the source tree rather than adding tracked `.bat`, `.cmd`, or `.ps1` wrappers.
- If the editor build fails on `atls.lib`, `atlbase.h`, or `DumpSyms`, install the Visual Studio Build Tools 2022 C++ ATL/MFC component and rerun the same UBT command.
- Meta XR `201.0` needed a local MSVC 14.44 compatibility patch on this machine: replace `INFINITY` with `std::numeric_limits<float>::infinity()` in `OculusXRHMD_Layer.cpp` and `OpenXR\OculusXRSpaceWarp.cpp`.
- The first validation project has been created and its `XRHomeSuiteValidationEditor` target builds successfully.
- Keep at least tens of GB free on `C:` before cloning Meta/NvRTX engine paths or installing large Unreal plugins. The stock source build can leave `C:\XRHomeSuite\engines` above 220 GB.

## Official References

- Epic Unreal source access: https://www.unrealengine.com/ue-on-github
- Epic source download docs: https://dev.epicgames.com/documentation/unreal-engine/downloading-source-code-in-unreal-engine
- Meta Unreal Engine 5 Integration 201.0: https://developers.meta.com/horizon/downloads/package/unreal-engine-5-integration/
- Meta XR plugin install docs: https://developers.meta.com/horizon/documentation/unreal/unreal-quick-start-install-metaxr-plugin/
- Meta Forward Renderer for PC-VR over Link: https://developers.meta.com/horizon/documentation/unreal/unreal-forward-renderer/
- Epic Forward Shading and MSAA notes: https://dev.epicgames.com/documentation/unreal-engine/forward-shading-renderer-in-unreal-engine
- NVIDIA NvRTX access: https://developer.nvidia.com/game-engines/unreal-engine/rtx-branch

## First Project After Gate Passes

After the Meta plugin is ready, create the smallest possible Windows PC-VR validation scene:

- Meta Link/OpenXR runtime only.
- Passthrough background.
- Hand/controller input.
- A small in-world debug panel.
- Sensor toggles for hands/depth.
- Performance HUD.
- No raw camera path.

Keep `apps/openxr_probe` as the diagnostics executable. It answers what the machine/runtime exposes; it must not become the product renderer.
