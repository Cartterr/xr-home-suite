from __future__ import annotations

import subprocess
from pathlib import Path

from .paths import repo_root


FORBIDDEN_SUFFIXES = {".ps1", ".bat", ".cmd"}
SOURCE_SUFFIXES = {".cpp", ".cxx", ".cc", ".h", ".hpp", ".py", ".cmake"}
MAX_SOURCE_LINES = 450
IGNORED_PARTS = {".git", "__pycache__", ".pytest_cache"}


def tracked_files() -> list[Path]:
    result = subprocess.run(
        ["git", "ls-files", "--cached", "--others", "--exclude-standard"],
        cwd=repo_root(),
        text=True,
        capture_output=True,
        check=True,
    )
    return [repo_root() / line.strip() for line in result.stdout.splitlines() if line.strip()]


def count_lines(path: Path) -> int:
    with path.open("r", encoding="utf-8", errors="ignore") as handle:
        return sum(1 for _ in handle)


def run_architecture_check() -> int:
    errors: list[str] = []
    for path in tracked_files():
        if not path.exists():
            continue
        relative = path.relative_to(repo_root())
        if any(part in IGNORED_PARTS for part in relative.parts):
            continue
        if path.suffix.lower() in FORBIDDEN_SUFFIXES:
            errors.append(f"Forbidden tracked script file: {relative}")
        if path.suffix.lower() in SOURCE_SUFFIXES:
            lines = count_lines(path)
            if lines > MAX_SOURCE_LINES:
                errors.append(f"Source file too long ({lines} > {MAX_SOURCE_LINES}): {relative}")

    if errors:
        print("Architecture check failed:")
        for error in errors:
            print(f"  {error}")
        return 1

    print("Architecture check passed.")
    return 0
