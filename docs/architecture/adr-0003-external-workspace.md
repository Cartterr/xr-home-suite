# ADR 0003: External Workspace On C Drive

## Status

Accepted.

## Decision

The repository stays source-only on `V:\dev\xr-home-suite`. Heavy local state goes under `C:\XRHomeSuite` by default.

## Rationale

Both the repo and Codex live on `V:`, and that drive is space constrained. OpenXR dependencies, CMake binary trees, reports, future engine installs, Unreal caches, captures, and generated artifacts are too large and volatile for the source repository.

## Consequences

- `XRHS_HOME` defaults to `C:\XRHomeSuite`.
- CMake presets use `C:\XRHomeSuite\build\xr-home-suite`.
- Dependency sync writes to `C:\XRHomeSuite\deps`.
- Reports and captures write to `C:\XRHomeSuite\artifacts`.
