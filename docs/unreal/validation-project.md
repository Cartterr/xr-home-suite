# First Unreal Validation Project

Create this project only after `python -m xrhs unreal-doctor` passes source access and finds or installs an engine under `C:\XRHomeSuite\engines`.

## Purpose

This is not the full XR Home Suite product. It is the smallest Unreal scene that proves the selected engine path can drive Meta Horizon Link with stable passthrough, input, and debug UI.

## Engine Path

Preferred first path:

- UE `5.7.4`
- Meta XR / Horizon Integration SDK `201.0`
- Windows PC-VR over Meta Horizon Link
- D3D12 + SM6
- Forward Renderer + MSAA

Experimental RTX path after the comfort path is stable:

- NVIDIA DLSS Super Resolution or DLAA
- NVIDIA Reflex
- NvRTX branch only after stock/Meta path is validated

## Project Location

- Engine installs/clones: `C:\XRHomeSuite\engines`
- Unreal project, once generated: `V:\dev\xr-home-suite\unreal\XRHomeSuiteValidation`
- Unreal generated folders remain ignored:
  - `Binaries`
  - `Build`
  - `DerivedDataCache`
  - `Intermediate`
  - `Saved`

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
