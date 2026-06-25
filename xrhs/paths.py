from __future__ import annotations

import os
from pathlib import Path


DEFAULT_XRHS_HOME = Path("C:/XRHomeSuite")


def repo_root() -> Path:
    return Path(__file__).resolve().parents[1]


def external_home() -> Path:
    return Path(os.environ.get("XRHS_HOME", str(DEFAULT_XRHS_HOME))).resolve()


def external_dirs() -> list[Path]:
    home = external_home()
    return [
        home / "deps",
        home / "build" / "xr-home-suite",
        home / "artifacts",
        home / "artifacts" / "reports",
        home / "artifacts" / "logs",
        home / "artifacts" / "captures",
        home / "engines",
        home / "cache",
    ]


def ensure_external_layout() -> None:
    for path in external_dirs():
        path.mkdir(parents=True, exist_ok=True)
