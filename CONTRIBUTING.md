# Contributing

XR Home Suite is a Windows PC-hosted XR project targeting Meta Horizon Link/OpenXR.

## Local Layout

- Repository: `V:\dev\xr-home-suite`
- External workspace: `C:\XRHomeSuite`
- Dependencies: `C:\XRHomeSuite\deps`
- Build trees: `C:\XRHomeSuite\build\xr-home-suite`
- Reports and captures: `C:\XRHomeSuite\artifacts`
- Future engine installs: `C:\XRHomeSuite\engines`

Override with `XRHS_HOME` only if you also keep heavy artifacts outside the repository.

## Commands

```text
python -m xrhs doctor
python -m xrhs deps sync
python -m xrhs configure --preset native-vs-debug
python -m xrhs build --preset native-vs-debug
ctest --preset native-vs-debug
python -m xrhs run-probe -- --seconds 5 --no-depth --no-hands --report C:\XRHomeSuite\artifacts\reports\render-only.json
python -m xrhs check
```

## Architecture Rules

- No tracked `.ps1`, `.bat`, or `.cmd` files.
- No source file over 450 non-generated lines.
- No monolithic subsystem files.
- No private camera-frame ABI guesses in product code.
- No large dependencies or generated outputs in the repository.
- Update `docs/architecture` when architecture decisions change.

## Engine Access

Unreal/Meta/NvRTX engine work is gated by external access. Run:

```text
python -m xrhs unreal-doctor
```

Do not commit a generated Unreal project until the engine path is available and the project can be opened or built.
