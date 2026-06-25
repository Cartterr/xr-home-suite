# ADR 0001: Stack Direction

## Status

Accepted.

## Decision

XR Home Suite will use a hybrid repository strategy:

- Native C++ OpenXR/D3D11 diagnostic probe now.
- Unreal Engine 5.7.4 + Meta XR Integration 201 / Meta fork validation for the future product shell.
- D3D12/SM6, Forward Shading, MSAA, passthrough underlay, and comfort-first profiling for the first Unreal build.
- NvRTX, DLSS, Reflex, and ray tracing stay in isolated experiment branches until validated in PCVR/OpenXR.

## Rationale

The current native probe proves Meta Horizon Link passthrough, environment depth, hand tracking, and private camera source enumeration over the local Oculus OpenXR runtime. Unreal provides the maintainable content, UI, profiling, and long-term product tooling that a custom engine would require us to build from scratch.

## Consequences

- The native probe must stay small and deterministic.
- Unreal source/project bootstrap is gated by external engine access.
- Stock UE vs Meta fork must be validated with a small packaged Windows Link scene before product lock-in.
