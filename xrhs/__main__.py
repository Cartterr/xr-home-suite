from __future__ import annotations

import argparse
import os
import subprocess
import sys

from .architecture import run_architecture_check
from .deps import sync_openxr_deps
from .doctor import run_doctor, run_unreal_doctor
from .paths import ensure_external_layout, external_home
from .secrets import run_secret_scan


def run_command(args: list[str], *, cwd: str | None = None) -> int:
    env = os.environ.copy()
    env.setdefault("XRHS_HOME", str(external_home()))
    completed = subprocess.run(args, cwd=cwd, env=env)
    return completed.returncode


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
    return run_command([str(exe), *probe_args])


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
