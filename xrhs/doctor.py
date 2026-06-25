from __future__ import annotations

import shutil
import subprocess
from pathlib import Path

from .paths import ensure_external_layout, external_home


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
    print(f"Engine root: {engine_root}")
    candidates = [
        engine_root / "UE_5.7.4",
        engine_root / "Oculus-VR-UnrealEngine-5.7.4",
        engine_root / "NvRTX-UnrealEngine-5.7.4",
        Path("C:/Program Files/Epic Games/UE_5.7"),
        Path("C:/Program Files/Epic Games/UE_5.7.4"),
    ]
    found = [path for path in candidates if path.exists()]
    if found:
        for path in found:
            print(f"FOUND: {path}")
    else:
        print("No UE 5.7.x installation found.")

    remotes = [
        "https://github.com/Oculus-VR/UnrealEngine.git",
        "https://github.com/NvRTX/UnrealEngine.git",
    ]
    failed = False
    for remote in remotes:
        result = subprocess.run(["git", "ls-remote", remote, "HEAD"], text=True, capture_output=True, timeout=30)
        if result.returncode == 0:
            print(f"ACCESS OK: {remote}")
        else:
            failed = True
            print(f"ACCESS BLOCKED: {remote}")
            print("  Link the required Epic/Meta/NVIDIA GitHub access before engine bootstrap.")
    return 1 if failed and not found else 0
