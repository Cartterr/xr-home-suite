# XR Home Suite Architecture Overview

XR Home Suite is a PC-hosted XR shell foundation for Meta Horizon Link/OpenXR. The repository stays source-only on `V:\dev\xr-home-suite`; heavy generated state lives under `C:\XRHomeSuite`.

## Runtime Strategy

- Product direction: Unreal Engine 5.7.4 + Meta XR Integration 201 / Meta fork validation, D3D12/SM6, Forward Shading, MSAA.
- Current executable: native OpenXR/D3D11 diagnostic probe.
- The native probe validates Meta Link runtime capabilities and writes machine-readable reports for agents and future MCP workflows.
- The native probe is not the long-term product renderer.

## Module Boundaries

- `apps/openxr_probe`: entrypoint, frame orchestration, report emission.
- `libs/xrh_core`: shared value types, checks, options, shutdown handling, JSON report writing.
- `libs/xrh_openxr`: OpenXR instance/session/system lifecycle and events.
- `libs/xrh_d3d11`: runtime-selected D3D11 adapter, swapchains, transparent overlay rendering.
- `libs/xrh_meta`: passthrough, depth, hand tracking, and private camera count-only probe.

## External Workspace

Default `XRHS_HOME` is `C:\XRHomeSuite`.

- `deps`: OpenXR headers/loader and future SDK drops.
- `build\xr-home-suite`: CMake binary directories.
- `artifacts`: reports, logs, captures, packaged outputs.
- `engines`: future Unreal/Meta/NvRTX engine installs.
- `cache`: CMake/Ninja/Unreal/DDC-style caches.

## Safety Boundaries

- One process owns the live OpenXR session.
- Helpers may communicate through IPC but must not initialize OpenXR.
- Public APIs are the product path.
- Private camera ABI work remains count-only in the main branch.
- Repo checks enforce no scripts, no secrets, and small source files.
