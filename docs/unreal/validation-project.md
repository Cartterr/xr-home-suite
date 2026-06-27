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
- Unreal project, once generated: `V:\dev\xr-home-suite\unreal\XRHomeSuiteValidation`
- Unreal generated folders remain ignored:
  - `Binaries`
  - `Build`
  - `DerivedDataCache`
  - `Intermediate`
  - `Saved`
- Local generated folders are junctioned to `C:\XRHomeSuite\build\xr-home-suite\unreal\XRHomeSuiteValidation` after build validation.

## Current Bootstrap State

- Source access is available for EpicGames, Oculus-VR, and NvRTX remotes.
- Stock UE `5.7.4-release` source is cloned under `C:\XRHomeSuite\engines\UE_5.7.4`.
- Dependencies are synced outside the repo with cache under `C:\XRHomeSuite\cache`.
- `UE5.sln` and UnrealBuildTool are generated.
- `Engine\Binaries\Win64\UnrealEditor.exe` is built.
- `Engine\Binaries\Win64\ShaderCompileWorker.exe` is built.
- Meta XR / Horizon Integration SDK `201.0` is installed under `Engine\Plugins\Marketplace\MetaXR`.
- `XRHomeSuiteValidationEditor` builds successfully through UnrealBuildTool.

## Commands

Build the validation editor target without using script wrappers:

`dotnet C:\XRHomeSuite\engines\UE_5.7.4\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.dll XRHomeSuiteValidationEditor Win64 Development "-Project=V:\dev\xr-home-suite\unreal\XRHomeSuiteValidation\XRHomeSuiteValidation.uproject" -WaitMutex -NoHotReload`

Build the shader worker if the engine tree is rebuilt or launch reports it missing:

`dotnet C:\XRHomeSuite\engines\UE_5.7.4\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.dll ShaderCompileWorker Win64 Development -WaitMutex`

Launch the validation scene over Meta Horizon Link:

`C:\XRHomeSuite\engines\UE_5.7.4\Engine\Binaries\Win64\UnrealEditor.exe V:\dev\xr-home-suite\unreal\XRHomeSuiteValidation\XRHomeSuiteValidation.uproject -game -vr -d3d12 -log`

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
- The project enables `OculusXR`, `OpenXR`, `OpenXRHandTracking`, `XRBase`, and `EnhancedInput`.
- The project disables `XGEController` so machines without Incredibuild do not log XGE shader compile warnings.
- The renderer defaults to D3D12, SM6, Forward Renderer, instanced stereo, and MSAA.
- The validation pawn requests a persistent Meta passthrough underlay at startup.
- The debug panel shows XR runtime, GPU adapter, frame time, passthrough state, hand/controller tracking state, and depth mode.
- Keyboard toggles exist for early testing: `F1` debug panel, `H` hand/controller visibility request, `D` depth mode label.

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
