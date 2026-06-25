# XR Home Suite Agent Rules

## Hard Rules

- Keep the repository source-only and lightweight. Heavy state belongs under `C:\XRHomeSuite`, not under `V:\dev\xr-home-suite`.
- Never add tracked `.ps1`, `.bat`, or `.cmd` files. Use `python -m xrhs`, CMake, and native tools directly.
- Never commit secrets, tokens, keys, certs, `.env` files, auth payloads, crash dumps, or runtime captures.
- Never guess private Meta/OpenXR camera ABI layouts. Private camera work stays count-only until documented or isolated in a separate research branch.
- Never create a second live OpenXR owner process. The active XR app owns the OpenXR session; helpers may use IPC only.
- Never turn large files into dumping grounds. Source files should stay below 450 non-generated lines and should target one responsibility.
- Preserve the native probe as a small diagnostic tool. The long-term product shell is Unreal/Meta/OpenXR gated by engine access.

## Required Workflow

- Run `python -m xrhs check` before committing.
- Run `python -m xrhs doctor` before native build work.
- Run `python -m xrhs deps sync` before CMake configure if OpenXR dependencies are missing.
- Use `python -m xrhs configure --preset native-vs-debug` and `python -m xrhs build --preset native-vs-debug`.
- Write optional probe output to `C:\XRHomeSuite\artifacts\reports`, not into the repo.
- Update docs/ADRs when architecture, public commands, or safety boundaries change.

## Source Boundaries

- `apps/openxr_probe`: CLI entrypoint and orchestration only.
- `libs/xrh_core`: options, checks, shutdown, reports, shared value types.
- `libs/xrh_openxr`: instance/session/system lifecycle and OpenXR events.
- `libs/xrh_d3d11`: D3D11 adapter/device/swapchains and diagnostic overlay rendering.
- `libs/xrh_meta`: Meta passthrough, environment depth, hand tracking, and private camera count probe.

## Open Source Safety

- Treat this as a public repository at all times.
- Prefer clear diagnostics over machine-specific hacks.
- Document external prerequisites instead of vendoring large SDKs or engine source.
- Keep generated artifacts reproducible and ignored.
