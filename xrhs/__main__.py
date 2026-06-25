from __future__ import annotations

import argparse
import datetime as dt
import os
from pathlib import Path
import subprocess
import sys

from .architecture import run_architecture_check
from .deps import sync_openxr_deps
from .doctor import run_doctor, run_unreal_doctor
from .paths import ensure_external_layout, external_home
from .secrets import run_secret_scan


RUNTIME_NOISE_TOKENS = (
    "[RUNTIMEIPC]",
    "RuntimeIPC",
    "RipcClient",
    "RipcServer",
    "RipcLocal",
    "RipcRemote",
    "RipcFramework",
    "RipcTransport",
    "IPCSharedMemory",
    "IPCDataChannel",
    "WindowsPackage:WindowsProcess",
    "[PCASTREAMING:",
)

VISIBLE_PROBE_PREFIXES = (
    "Quest Link",
    "Runtime:",
    "Passthrough supported:",
    "Environment depth supported:",
    "Hand tracking supported:",
    "D3D11 adapter:",
    "OpenXR D3D11 session created.",
    "Eye ",
    "Hand tracking:",
    "Passthrough compositor layer:",
    "Environment depth",
    "Private Link camera-source count probe:",
    "Put the headset",
    "Sensor polling caps:",
    "Session running.",
    "frames=",
    "Ctrl+C received.",
    "Warning:",
    "Fatal:",
    "OpenXR shutdown complete.",
    "Probe report:",
)


def run_command(args: list[str], *, cwd: str | None = None) -> int:
    env = os.environ.copy()
    env.setdefault("XRHS_HOME", str(external_home()))
    completed = subprocess.run(args, cwd=cwd, env=env)
    return completed.returncode


def is_runtime_noise(line: str) -> bool:
    return any(token in line for token in RUNTIME_NOISE_TOKENS)


def is_visible_probe_line(line: str) -> bool:
    stripped = line.lstrip()
    return not stripped or any(stripped.startswith(prefix) for prefix in VISIBLE_PROBE_PREFIXES)


def default_probe_log_path() -> Path:
    timestamp = dt.datetime.now(dt.UTC).strftime("%Y%m%d-%H%M%S")
    return external_home() / "artifacts" / "logs" / f"openxr_probe-{timestamp}.log"


def run_quiet_probe(command: list[str], log_path: Path) -> int:
    env = os.environ.copy()
    env.setdefault("XRHS_HOME", str(external_home()))
    log_path.parent.mkdir(parents=True, exist_ok=True)
    print(f"Full probe log: {log_path}")
    with log_path.open("w", encoding="utf-8", errors="replace") as log:
        process = subprocess.Popen(
            command,
            env=env,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            encoding="utf-8",
            errors="replace",
            bufsize=1,
        )
        try:
            assert process.stdout is not None
            skip_block = False
            for line in process.stdout:
                log.write(line)
                if line.startswith("Relevant runtime extensions:"):
                    print("Relevant runtime extensions: full list written to raw log.")
                    skip_block = True
                    continue
                if line.startswith("Enabled OpenXR extensions:"):
                    print("Enabled OpenXR extensions: full list written to raw log.")
                    skip_block = True
                    continue
                if skip_block:
                    if not line.strip():
                        skip_block = False
                    continue
                if line.startswith("Session state:"):
                    continue
                if not is_runtime_noise(line) and is_visible_probe_line(line):
                    print(line, end="")
            return process.wait()
        except KeyboardInterrupt:
            print("\nCtrl+C received. Waiting for openxr_probe to clean up...")
            try:
                return process.wait(timeout=15)
            except subprocess.TimeoutExpired:
                process.terminate()
                return 130


def cmake_configure(args: argparse.Namespace) -> int:
    ensure_external_layout()
    return run_command(["cmake", "--preset", args.preset])


def cmake_build(args: argparse.Namespace) -> int:
    ensure_external_layout()
    return run_command(["cmake", "--build", "--preset", args.preset])


def run_probe(args: argparse.Namespace) -> int:
    ensure_external_layout()
    exe = external_home() / "build" / "xr-home-suite" / args.preset / "apps" / "openxr_probe"
    exe = exe / args.config / "openxr_probe.exe"
    if not exe.exists():
        print(f"openxr_probe.exe not found at {exe}")
        print(f"Build it first: python -m xrhs build --preset {args.preset}")
        return 1
    probe_args = args.probe_args
    if probe_args and probe_args[0] == "--":
        probe_args = probe_args[1:]
    command = [str(exe), *probe_args]
    if args.raw_output:
        return run_command(command)
    return run_quiet_probe(command, args.log or default_probe_log_path())


def run_all_checks(_: argparse.Namespace) -> int:
    status = 0
    status |= run_architecture_check()
    status |= run_secret_scan()
    return 0 if status == 0 else 1


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(prog="python -m xrhs")
    subparsers = parser.add_subparsers(dest="command", required=True)

    doctor = subparsers.add_parser("doctor", help="Check local machine prerequisites.")
    doctor.set_defaults(func=lambda args: run_doctor())

    unreal_doctor = subparsers.add_parser("unreal-doctor", help="Check Unreal/Meta/NVIDIA engine access.")
    unreal_doctor.set_defaults(func=lambda args: run_unreal_doctor())

    deps = subparsers.add_parser("deps", help="Manage external dependencies.")
    deps_subparsers = deps.add_subparsers(dest="deps_command", required=True)
    deps_sync = deps_subparsers.add_parser("sync", help="Sync OpenXR dependencies into XRHS_HOME.")
    deps_sync.set_defaults(func=lambda args: sync_openxr_deps())

    configure = subparsers.add_parser("configure", help="Configure CMake.")
    configure.add_argument("--preset", default="native-vs-debug")
    configure.set_defaults(func=cmake_configure)

    build = subparsers.add_parser("build", help="Build a CMake preset.")
    build.add_argument("--preset", default="native-vs-debug")
    build.set_defaults(func=cmake_build)

    probe = subparsers.add_parser("run-probe", help="Run the native OpenXR probe.")
    probe.add_argument("--preset", default="native-vs-debug")
    probe.add_argument("--config", default="Debug")
    probe.add_argument("--raw-output", action="store_true", help="Print unfiltered Meta runtime output.")
    probe.add_argument("--log", type=Path, help="Path for the full unfiltered probe log.")
    probe.add_argument("probe_args", nargs=argparse.REMAINDER)
    probe.set_defaults(func=run_probe)

    check = subparsers.add_parser("check", help="Run all repo checks.")
    check.set_defaults(func=run_all_checks)

    arch = subparsers.add_parser("architecture-check", help="Check source architecture rules.")
    arch.set_defaults(func=lambda args: run_architecture_check())

    secret = subparsers.add_parser("secret-scan", help="Scan tracked files for likely secrets.")
    secret.set_defaults(func=lambda args: run_secret_scan())

    args = parser.parse_args(argv)
    return args.func(args)


if __name__ == "__main__":
    raise SystemExit(main())
