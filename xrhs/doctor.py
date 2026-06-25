from __future__ import annotations

import shutil
import subprocess
from pathlib import Path

from .paths import ensure_external_layout, external_home


TARGET_UNREAL_VERSION = "5.7.4"
TARGET_META_INTEGRATION_VERSION = "201.0"

UNREAL_SOURCE_REMOTES = [
    (
        "EpicGames Unreal source",
        "https://github.com/EpicGames/UnrealEngine.git",
        "https://www.unrealengine.com/ue-on-github",
    ),
    (
        "Meta/Oculus Unreal fork",
        "https://github.com/Oculus-VR/UnrealEngine.git",
        "https://developers.meta.com/horizon/downloads/package/unreal-engine-5-integration/",
    ),
    (
        "NVIDIA NvRTX Unreal branch",
        "https://github.com/NvRTX/UnrealEngine.git",
        "https://developer.nvidia.com/game-engines/unreal-engine/rtx-branch",
    ),
]


def command_exists(name: str) -> bool:
    return shutil.which(name) is not None


def run_checked(args: list[str]) -> tuple[bool, str]:
    try:
        result = subprocess.run(args, text=True, capture_output=True, timeout=20)
    except (OSError, subprocess.SubprocessError) as error:
        return False, str(error)
    output = (result.stdout + result.stderr).strip()
    return result.returncode == 0, output


def print_status(label: str, ok: bool, detail: str = "") -> None:
    prefix = "OK" if ok else "MISSING"
    print(f"{prefix}: {label}{' - ' + detail if detail else ''}")


def check_git_remote(remote: str) -> bool:
    result = subprocess.run(["git", "ls-remote", remote, "HEAD"], text=True, capture_output=True, timeout=30)
    return result.returncode == 0


def print_engine_markers(path: Path) -> None:
    markers = [
        ("source tree", path / "Engine" / "Source" / "Runtime"),
        ("generated solution", path / "UE5.sln"),
        (
            "UnrealBuildTool",
            path / "Engine" / "Binaries" / "DotNET" / "UnrealBuildTool" / "UnrealBuildTool.dll",
        ),
        ("UnrealEditor executable", path / "Engine" / "Binaries" / "Win64" / "UnrealEditor.exe"),
    ]
    for label, marker in markers:
        ok = marker.exists()
        print(f"  {'READY' if ok else 'PENDING'}: {label} - {marker}")


def run_doctor() -> int:
    ensure_external_layout()
    checks: list[tuple[str, bool, str]] = []
    for command in ["python", "cmake", "ninja", "git"]:
        ok, detail = run_checked([command, "--version"])
        checks.append((command, ok, detail.splitlines()[0] if detail else ""))

    ok, detail = run_checked(["git", "lfs", "version"])
    checks.append(("git lfs", ok, detail.splitlines()[0] if detail else "Install Git LFS if large assets are introduced."))

    vswhere = Path("C:/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe")
    ok = vswhere.exists()
    detail = str(vswhere) if ok else "Install Visual Studio Build Tools 2022 with MSVC x64 tools."
    checks.append(("Visual Studio vswhere", ok, detail))

    openxr_root = external_home() / "deps" / "openxr"
    checks.append(("XRHS_HOME", external_home().exists(), str(external_home())))
    checks.append(("OpenXR headers", (openxr_root / "include" / "openxr" / "openxr.h").exists(), str(openxr_root)))
    checks.append(("OpenXR loader", (openxr_root / "loader" / "native" / "x64" / "release" / "lib" / "openxr_loader.lib").exists(), str(openxr_root)))

    failed = False
    for label, ok, detail in checks:
        print_status(label, ok, detail)
        failed = failed or not ok
    return 1 if failed else 0


def run_unreal_doctor() -> int:
    ensure_external_layout()
    engine_root = external_home() / "engines"
    print("Target Unreal validation stack:")
    print(f"  UE {TARGET_UNREAL_VERSION}")
    print(f"  Meta XR / Horizon Integration SDK {TARGET_META_INTEGRATION_VERSION}")
    print("  D3D12 + SM6, Forward Renderer + MSAA comfort path")
    print("  DLSS/DLAA/Reflex validation path after base Link scene is stable")
    print("")
    print(f"Engine root: {engine_root}")

    ok, github_user = run_checked(["gh", "api", "user", "--jq", ".login"])
    if ok:
        print(f"GitHub user: {github_user}")
    else:
        print("GitHub user: unavailable through gh. Run: gh auth status")

    candidates = [
        engine_root / f"UE_{TARGET_UNREAL_VERSION}",
        engine_root / f"Oculus-VR-UnrealEngine-{TARGET_UNREAL_VERSION}",
        engine_root / f"NvRTX-UnrealEngine-{TARGET_UNREAL_VERSION}",
        Path("C:/Program Files/Epic Games/UE_5.7"),
        Path(f"C:/Program Files/Epic Games/UE_{TARGET_UNREAL_VERSION}"),
    ]
    found = [path for path in candidates if path.exists()]
    if found:
        for path in found:
            print(f"FOUND: {path}")
            print_engine_markers(path)
    else:
        print(f"No UE {TARGET_UNREAL_VERSION} installation found.")

    blocked: list[tuple[str, str, str]] = []
    for label, remote, guide_url in UNREAL_SOURCE_REMOTES:
        if check_git_remote(remote):
            print(f"ACCESS OK: {label} - {remote}")
        else:
            blocked.append((label, remote, guide_url))
            print(f"ACCESS BLOCKED: {label} - {remote}")

    if blocked:
        print("")
        print("Next required manual access steps:")
        print("  1. Link the Epic/Unreal account to this GitHub account and accept the @EpicGames invite.")
        print("  2. Confirm Epic source access works before attempting Meta or NvRTX engine bootstrap.")
        print("  3. Confirm Meta/Oculus fork access for the Meta engine path.")
        print("  4. Confirm NVIDIA NvRTX access for the experimental RTX branch.")
        print("")
        print("Official access links:")
        for label, _, guide_url in blocked:
            print(f"  {label}: {guide_url}")
        print("")
        print("Validation commands after account linking:")
        for label, remote, _ in UNREAL_SOURCE_REMOTES:
            print(f"  git ls-remote {remote} HEAD  # {label}")
    else:
        print("")
        print("Source access gate: passed for EpicGames, Meta/Oculus, and NVIDIA NvRTX remotes.")

    return 1 if blocked or not found else 0
