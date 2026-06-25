# Unreal Engine Gate

This repo must not commit a generated Unreal project until the engine can be opened or built locally.

## Target Stack

- Unreal Engine: `5.7.4`
- Meta XR / Horizon Integration SDK: `201.0`
- Install root: `C:\XRHomeSuite\engines`
- Comfort/default path: D3D12, SM6, Forward Renderer, MSAA
- RTX validation path: DLSS/DLAA/Reflex after the base Link scene is stable
- Product boundary: no raw private camera dependency

## Current Local Gate

`python -m xrhs unreal-doctor` currently reports:

- GitHub auth works for user `Cartterr`.
- `EpicGames/UnrealEngine` access is blocked.
- `Oculus-VR/UnrealEngine` access is blocked.
- `NvRTX/UnrealEngine` access is blocked.
- No local UE `5.7.4` install is present.

Private Unreal repositories usually return `404` when the GitHub account has not completed account linking, organization invite acceptance, or vendor-specific source access.

## Required Access Order

1. Link Epic/Unreal account to the GitHub account and accept the `@EpicGames` GitHub organization invite.
2. Verify Epic source access:
   `git ls-remote https://github.com/EpicGames/UnrealEngine.git HEAD`
3. Verify Meta/Oculus source access:
   `git ls-remote https://github.com/Oculus-VR/UnrealEngine.git HEAD`
4. Verify NVIDIA NvRTX source access:
   `git ls-remote https://github.com/NvRTX/UnrealEngine.git HEAD`
5. Re-run:
   `python -m xrhs unreal-doctor`

Do not clone or generate a UE project until these gates pass.

## Official References

- Epic Unreal source access: https://www.unrealengine.com/ue-on-github
- Epic source download docs: https://dev.epicgames.com/documentation/unreal-engine/downloading-source-code-in-unreal-engine
- Meta Unreal Engine 5 Integration 201.0: https://developers.meta.com/horizon/downloads/package/unreal-engine-5-integration/
- Meta Forward Renderer for PC-VR over Link: https://developers.meta.com/horizon/documentation/unreal/unreal-forward-renderer/
- Epic Forward Shading and MSAA notes: https://dev.epicgames.com/documentation/unreal-engine/forward-shading-renderer-in-unreal-engine
- NVIDIA NvRTX access: https://developer.nvidia.com/game-engines/unreal-engine/rtx-branch

## First Project After Gate Passes

Create the smallest possible Windows PC-VR validation scene:

- Meta Link/OpenXR runtime only.
- Passthrough background.
- Hand/controller input.
- A small in-world debug panel.
- Sensor toggles for hands/depth.
- Performance HUD.
- No raw camera path.

Keep `apps/openxr_probe` as the diagnostics executable. It answers what the machine/runtime exposes; it must not become the product renderer.
