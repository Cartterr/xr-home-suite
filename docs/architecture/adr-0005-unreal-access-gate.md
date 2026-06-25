# ADR 0005: Unreal Access Gate

## Status

Accepted.

## Decision

Do not commit a generated Unreal project until engine access and local engine installation are confirmed.

## Rationale

Current GitHub authentication cannot access `Oculus-VR/UnrealEngine` or `NvRTX/UnrealEngine`. Committing Unreal project scaffolding before the engine can be opened or built creates stale configuration and generated noise.

## Consequences

- `python -m xrhs unreal-doctor` reports engine availability and source access.
- Future engine installs live under `C:\XRHomeSuite\engines`.
- Unreal work begins with a small packaged Windows Link validation scene after access is available.
