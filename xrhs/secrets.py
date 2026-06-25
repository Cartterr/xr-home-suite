from __future__ import annotations

import re
import subprocess
from pathlib import Path

from .paths import repo_root


SECRET_PATTERNS = [
    re.compile(r"sk-[A-Za-z0-9_-]{20,}"),
    re.compile(r"gh[pousr]_[A-Za-z0-9_]{20,}"),
    re.compile(
        r"(?i)(api[_-]?key|secret[_-]?key|client[_-]?secret|access[_-]?token|refresh[_-]?token|password)"
        r"\s*[:=]\s*['\"]?[A-Za-z0-9_./+=-]{16,}"
    ),
    re.compile(r"-----BEGIN (?:RSA |EC |OPENSSH |PRIVATE )?PRIVATE KEY-----"),
]

ALLOWLISTED_FILES = {
    "docs/xr-home-suite-research-llm-prompt.md",
    "AGENTS.md",
    "SECURITY.md",
}


def tracked_files() -> list[Path]:
    result = subprocess.run(
        ["git", "ls-files", "--cached", "--others", "--exclude-standard"],
        cwd=repo_root(),
        text=True,
        capture_output=True,
        check=True,
    )
    return [repo_root() / line.strip() for line in result.stdout.splitlines() if line.strip()]


def run_secret_scan() -> int:
    findings: list[str] = []
    for path in tracked_files():
        if not path.exists():
            continue
        relative = path.relative_to(repo_root()).as_posix()
        if relative in ALLOWLISTED_FILES:
            continue
        if path.suffix.lower() in {".png", ".jpg", ".jpeg", ".webp", ".ico", ".dll", ".lib", ".pdb"}:
            continue
        text = path.read_text(encoding="utf-8", errors="ignore")
        for index, line in enumerate(text.splitlines(), start=1):
            if any(pattern.search(line) for pattern in SECRET_PATTERNS):
                findings.append(f"{relative}:{index}")

    if findings:
        print("Secret scan failed:")
        for finding in findings:
            print(f"  {finding}")
        return 1

    print("Secret scan passed.")
    return 0
