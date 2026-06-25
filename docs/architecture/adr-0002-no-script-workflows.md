# ADR 0002: No PowerShell, Batch, Or Cmd Workflows

## Status

Accepted.

## Decision

The repository will not track `.ps1`, `.bat`, or `.cmd` files. Repository automation uses Python standard-library tooling and CMake presets.

## Rationale

The project needs a clean, cross-agent workflow with fewer Windows shell edge cases and no hidden destructive shell behavior. Python gives the repo one portable orchestration entrypoint while CMake owns native build generation.

## Consequences

- Use `python -m xrhs` for repo automation.
- Use CMake presets for build configuration.
- Architecture checks fail if script files are tracked.
