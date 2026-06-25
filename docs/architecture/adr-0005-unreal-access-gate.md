# ADR 0005: Unreal Access Gate

## Status

Accepted.

## Decision

Do not commit a generated Unreal project until engine access, local engine installation, editor build, and Meta plugin staging are confirmed.

## Rationale

The source access gate is now available for `EpicGames/UnrealEngine`, `Oculus-VR/UnrealEngine`, and `NvRTX/UnrealEngine`. The stock UE `5.7.4` editor binary is built locally. The next risk is generated Unreal project noise before the selected engine path can load the Meta XR plugin. Keeping the project gated avoids stale settings, large generated folders, and half-valid editor configuration.

## Consequences

- `python -m xrhs unreal-doctor` reports engine availability, source access, and core local build markers.
- Future engine installs live under `C:\XRHomeSuite\engines`.
- Unreal work begins with a small packaged Windows Link validation scene after the Meta plugin path is available.
- The current gate and official access links live in `docs/unreal/engine-gate.md`.
