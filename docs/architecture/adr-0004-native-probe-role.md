# ADR 0004: Native Probe Role

## Status

Accepted.

## Decision

The native OpenXR executable is a diagnostic probe, not the product renderer.

## Rationale

The native path is the most direct way to verify OpenXR extensions, Meta Link behavior, runtime-selected GPU adapter, shutdown behavior, sensor availability, and private camera source count. Turning it into the product shell would require rebuilding engine features that Unreal already provides.

## Consequences

- Keep the probe modular and small.
- Emit machine-readable capability reports.
- Preserve D3D11 as a stable comparison path.
- Add D3D12 probing separately before using D3D12 as a native product fallback.
