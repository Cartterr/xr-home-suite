# First Unreal Validation Project

Create this project only after `python -m xrhs unreal-doctor` passes source access, finds the engine under `C:\XRHomeSuite\engines`, the editor binary exists, and Meta XR / Horizon Integration SDK `201.0` is installed or staged.

## Purpose

This is not the full XR Home Suite product. It is the smallest Unreal scene that proves the selected engine path can drive Meta Horizon Link with stable passthrough, input, and debug UI.

## Engine Path

Preferred first path:

- UE `5.7.4`
- Stock Epic source install first: `C:\XRHomeSuite\engines\UE_5.7.4`
- Meta XR / Horizon Integration SDK `201.0`
- Windows PC-VR over Meta Horizon Link
- D3D12 + SM6
- Forward Renderer + MSAA

Passthrough-over-Link validation is a separate editor preview path. Meta's current Unreal guidance uses VR Preview with Android Vulkan Mobile Preview for passthrough-over-Link. Do not use the forced `-game -vr -d3d12` launch path for passthrough validation; it can initialize the passthrough layer while the normal opaque PC projection layer hides it.

Experimental RTX path after the comfort path is stable:

- NVIDIA DLSS Super Resolution or DLAA
- NVIDIA Reflex
- NvRTX branch only after stock/Meta path is validated

Source validation order:

- Stock Epic `5.7.4-release` first because it matches Meta XR / Horizon Integration SDK `201.0`.
- Meta `oculus-5.7` fork second to compare Meta-specific fixes/features against the stock plugin path.
- NvRTX `nvrtx-5.7.4.0` third as an RTX experiment, not as the default comfort path.

## Project Location

- Engine installs/clones: `C:\XRHomeSuite\engines`
- Unreal project, once generated: `V:\dev\xr-home-suite\unreal\XRHSValidation`
- Unreal generated folders remain ignored:
  - `Binaries`
  - `Build`
  - `DerivedDataCache`
  - `Intermediate`
  - `Saved`
- Local generated folders are junctioned to `C:\XRHomeSuite\build\xr-home-suite\unreal\XRHSValidation` after build validation.

## Current Bootstrap State

- Source access is available for EpicGames, Oculus-VR, and NvRTX remotes.
- Stock UE `5.7.4-release` source is cloned under `C:\XRHomeSuite\engines\UE_5.7.4`.
- Dependencies are synced outside the repo with cache under `C:\XRHomeSuite\cache`.
- `UE5.sln` and UnrealBuildTool are generated.
- `Engine\Binaries\Win64\UnrealEditor.exe` is built.
- `Engine\Binaries\Win64\ShaderCompileWorker.exe` is built.
- Meta XR / Horizon Integration SDK `201.0` is installed under `Engine\Plugins\Marketplace\MetaXR`.
- Meta Interaction SDK for Unreal `201.0` is installed under `Engine\Plugins\Marketplace\MetaXRInteraction`.
- Meta Movement SDK modules are present through `MetaXR\Source\OculusXRMovement`.
- NVIDIA DLSS/Streamline UE `5.7` plugins are installed under `Engine\Plugins\Marketplace` for later opt-in RTX validation.
- `XRHSValidationEditor` builds successfully through UnrealBuildTool.
- The project shell and C++ modules use `XRHSValidation` because Unreal warns when a project filename exceeds 20 characters and several editor code-generation paths assume the project name is also the primary game module.

## External Tooling

Run this after machine setup changes:

`python -m xrhs tools-doctor`

Expected local tools are external to the repo:

- Meta Quest Developer Hub: `C:\Program Files\Meta Quest Developer Hub`
- Meta XR Simulator payload: `C:\XRHomeSuite\tools\MetaXRSimulator\v201.0`
- RenderDoc: `C:\Program Files\RenderDoc`
- Microsoft PIX: `C:\Program Files\Microsoft PIX\2603.25`
- OpenXR Explorer: `C:\XRHomeSuite\tools\OpenXR-Explorer-v1.7`
- Khronos OpenXR SDK API layers: `C:\XRHomeSuite\tools\OpenXR-SDK-Source`
- NVIDIA Nsight Graphics portable host: `C:\XRHomeSuite\tools\NVIDIA_Nsight_Graphics_2026.2_Portable`

Do not activate Meta XR Simulator as the system OpenXR runtime while Meta Horizon Link validation is active; its activation script writes `HKLM\SOFTWARE\Khronos\OpenXR\1\ActiveRuntime`.

## Commands

Build the validation editor target without using script wrappers:

`dotnet C:\XRHomeSuite\engines\UE_5.7.4\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.dll XRHSValidationEditor Win64 Development "-Project=V:\dev\xr-home-suite\unreal\XRHSValidation\XRHSValidation.uproject" -WaitMutex -NoHotReload`

Build the shader worker if the engine tree is rebuilt or launch reports it missing:

`dotnet C:\XRHomeSuite\engines\UE_5.7.4\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.dll ShaderCompileWorker Win64 Development -WaitMutex`

Launch the validation scene over Meta Horizon Link:

0. Apply the local editor preview settings:

   `python -m xrhs unreal-link-setup`

1. Open the project in the editor:

   `C:\XRHomeSuite\engines\UE_5.7.4\Engine\Binaries\Win64\UnrealEditor.exe V:\dev\xr-home-suite\unreal\XRHSValidation\XRHSValidation.uproject -log`

2. Confirm the viewport preview platform is `Android Vulkan Mobile` / `Android_OpenXR`. The source default lives in `Config\DefaultEditorPerProjectUserSettings.ini`:

   - `PreviewPlatformName=Android`
   - `PreviewShaderFormatName=SF_VULKAN_ES31_ANDROID`
   - `PreviewShaderPlatformName=VULKAN_ES3_1_ANDROID_Preview`
   - `PreviewDeviceProfileName=Android_OpenXR`

3. Use the editor's `VR Preview` play mode while Meta Horizon Link is already connected and the headset is awake.

   The validation project also adds editor shortcuts:

   - Bottom status toolbar: `XRHS Start VR` and `XRHS Stop`.
   - Main menu fallback: `Tools > XR Home Suite > Start XRHS VR Preview`.
   - `XRHS Start VR` applies the Android OpenXR Link preview platform and passthrough alpha CVars before requesting Unreal's VR Preview session.
   - Live Coding is disabled by project default so external editor-target builds are not blocked while iterating on this validation shell.

Do not restart Meta Horizon Link from automation while debugging the validation scene. Close the Unreal preview first, then relaunch from the editor if needed.

Official references:

- Meta Link for Unreal: https://developers.meta.com/horizon/documentation/unreal/unreal-link/
- Meta passthrough over Link: https://developers.meta.com/horizon/documentation/unreal/unreal-passthrough-use-over-link/
- Meta passthrough setup: https://developers.meta.com/horizon/documentation/unreal/unreal-passthrough-overview-gs/

## Required Scene

The first scene should include:

- Meta Link/OpenXR startup through the active Meta runtime.
- Passthrough background.
- A simple GPU-rendered room shell or grid.
- Hands and controller input enabled.
- A compact in-world debug panel showing:
  - runtime name
  - selected GPU
  - app frame time
  - hand state
  - depth toggle state
  - passthrough state
- Sensor toggles:
  - hands on/off
  - depth off/low/adaptive
  - performance HUD on/off

Current implementation status:

- The scene is source-only and creates a validation grid at runtime.
- The project enables `OculusXR`, `OpenXR`, `OpenXRHandTracking`, `XRBase`, `EnhancedInput`, and `OculusInteraction`.
- The project disables `XGEController` so machines without Incredibuild do not log XGE shader compile warnings.
- The product renderer defaults to D3D12, SM6, Forward Renderer, instanced stereo, and MSAA.
- The Link passthrough validation path defaults the editor preview platform to Android Vulkan Mobile with the `Android_OpenXR` device profile.
- The validation pawn requests persistent Meta passthrough at startup. Editor VR Preview starts in overlay-fallback mode so the camera feed is visible even if the app framebuffer is still opaque; underlay mode remains available for alpha validation.
- Persistent passthrough parameters must call `ApplyShape()` after setting layer placement/opacity because the Meta editor path reloads shape settings from the serializable temporary fields.
- The debug panel shows XR runtime, GPU adapter, frame time, passthrough state, hand/controller tracking state, and depth mode.
- Keyboard toggles exist for early testing: `F1` debug panel, `F2` passthrough overlay/underlay placement, `R` floor-origin HMD recenter, `H` hand/controller visibility request, `D` depth mode label.

Current clean-launch verification:

- Short hidden launch log: `C:\XRHomeSuite\artifacts\logs\xr-validation-clean-launch.log`
- No matching `LogXGEController`, missing Slate platform icon, or missing `ShaderCompileWorker` warnings after cleanup.

## Defaults

- Do not depend on raw camera frames.
- Do not enable expensive RTX features by default.
- Keep depth low-rate/adaptive until profiling proves combined hands+depth is smooth.
- Keep the native `openxr_probe` as the runtime capability diagnostic.

## Acceptance Criteria

- Opens from the selected engine without generating committed trash.
- Runs over Meta Horizon Link on Quest 3/3S.
- Uses the RTX 4070 through the selected D3D12 path.
- Shows passthrough with stable frame pacing.
- Hands/controllers can interact with the debug panel.
- Depth can be toggled without making the base scene unusable.
- Packaged Windows build is possible before expanding the scene.

## Deferred

- Raw passthrough camera buffers.
- Private OpenXR ABI experiments.
- NvRTX branch migration.
- Ray tracing showcase mode.
- Full launcher/home shell UI.
