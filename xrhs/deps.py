from __future__ import annotations

import shutil
import urllib.request
import zipfile
from pathlib import Path

from .paths import ensure_external_layout, external_home


OPENXR_HEADER_NAMES = [
    "openxr.h",
    "openxr_platform.h",
    "openxr_platform_defines.h",
    "openxr_reflection.h",
]

OPENXR_LOADER_VERSION = "1.0.10.2"
OPENXR_LOADER_URL = (
    "https://api.nuget.org/v3-flatcontainer/openxr.loader/"
    f"{OPENXR_LOADER_VERSION}/openxr.loader.{OPENXR_LOADER_VERSION}.nupkg"
)


def download_file(url: str, destination: Path) -> None:
    if destination.exists():
        return
    destination.parent.mkdir(parents=True, exist_ok=True)
    print(f"Downloading {url}")
    with urllib.request.urlopen(url, timeout=60) as response:
        destination.write_bytes(response.read())


def sync_openxr_deps() -> int:
    ensure_external_layout()
    deps_root = external_home() / "deps" / "openxr"
    headers_dir = deps_root / "include" / "openxr"
    package_path = deps_root / f"openxr.loader.{OPENXR_LOADER_VERSION}.nupkg"
    loader_dir = deps_root / "loader"

    for header_name in OPENXR_HEADER_NAMES:
        url = f"https://raw.githubusercontent.com/KhronosGroup/OpenXR-SDK/main/include/openxr/{header_name}"
        download_file(url, headers_dir / header_name)

    download_file(OPENXR_LOADER_URL, package_path)

    loader_lib = loader_dir / "native" / "x64" / "release" / "lib" / "openxr_loader.lib"
    loader_dll = loader_dir / "native" / "x64" / "release" / "bin" / "openxr_loader.dll"
    if not loader_lib.exists() or not loader_dll.exists():
        temp_zip = deps_root / f"openxr.loader.{OPENXR_LOADER_VERSION}.zip"
        shutil.copyfile(package_path, temp_zip)
        with zipfile.ZipFile(temp_zip) as archive:
            archive.extractall(loader_dir)
        temp_zip.unlink(missing_ok=True)

    required = [headers_dir / name for name in OPENXR_HEADER_NAMES]
    required.extend([loader_lib, loader_dll])
    missing = [path for path in required if not path.exists()]
    if missing:
        for path in missing:
            print(f"Missing required OpenXR dependency: {path}")
        return 1

    print(f"OpenXR dependencies ready under {deps_root}")
    return 0
