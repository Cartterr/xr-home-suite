from __future__ import annotations

import json
import shutil
import subprocess
from pathlib import Path

from .paths import ensure_external_layout, external_home


TARGET_UNREAL_VERSION = "5.7.4"
TARGET_META_INTEGRATION_VERSION = "201.0"
META_PLUGIN_NAMES = {"MetaXR", "MetaXRPlatform", "OculusXR"}
SLATE_LAUNCHER_ICON_PLATFORMS = ("Android", "IOS", "Linux", "Mac", "TVOS")

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


def print_engine_markers(path: Path) -> bool:
    markers = [
        ("source tree", path / "Engine" / "Source" / "Runtime"),
        ("generated solution", path / "UE5.sln"),
        (
            "UnrealBuildTool",
            path / "Engine" / "Binaries" / "DotNET" / "UnrealBuildTool" / "UnrealBuildTool.dll",
        ),
        ("UnrealEditor executable", path / "Engine" / "Binaries" / "Win64" / "UnrealEditor.exe"),
        ("ShaderCompileWorker executable", path / "Engine" / "Binaries" / "Win64" / "ShaderCompileWorker.exe"),
    ]
    all_ready = True
    for label, marker in markers:
        ok = marker.exists()
        all_ready = all_ready and ok
        print(f"  {'READY' if ok else 'PENDING'}: {label} - {marker}")
    return all_ready


def print_slate_launcher_icon_markers(path: Path) -> bool:
    launcher_root = path / "Engine" / "Content" / "Editor" / "Slate" / "Launcher"
    missing: list[Path] = []
    for platform in SLATE_LAUNCHER_ICON_PLATFORMS:
        for size in ("24x", "128x"):
            marker = launcher_root / platform / f"Platform_{platform}_{size}.png"
            if not marker.exists():
                missing.append(marker)
    if missing:
        print(f"  PENDING: Slate launcher platform icons - {len(missing)} missing under {launcher_root}")
        return False
    print(f"  READY: Slate launcher platform icons - {launcher_root}")
    return True


def find_meta_plugin_markers(path: Path) -> list[tuple[Path, str]]:
    plugin_roots = [
        path / "Engine" / "Plugins" / "Marketplace",
        path / "Engine" / "Plugins" / "Runtime",
        path / "Engine" / "Plugins",
    ]
    markers: list[tuple[Path, str]] = []
    seen: set[Path] = set()
    for root in plugin_roots:
        if not root.exists():
            continue
        for plugin_file in root.rglob("*.uplugin"):
            resolved = plugin_file.resolve()
            if resolved in seen:
                continue
            seen.add(resolved)
            if not any(name.lower() in plugin_file.stem.lower() for name in META_PLUGIN_NAMES):
                continue
            version = "unknown version"
            try:
                data = json.loads(plugin_file.read_text(encoding="utf-8"))
                version = str(data.get("VersionName") or data.get("Version") or version)
            except (OSError, json.JSONDecodeError):
                pass
            markers.append((plugin_file, version))
    return markers


def has_target_meta_plugin(markers: list[tuple[Path, str]]) -> bool:
    return any(TARGET_META_INTEGRATION_VERSION in version for _, version in markers)


def uplugin_detail(path: Path) -> str:
    if not path.exists():
        return str(path)
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError):
        return str(path)
    version = data.get("VersionName") or data.get("Version")
    return f"{version} - {path}" if version else str(path)


def files_ready(paths: list[Path]) -> bool:
    return all(path.exists() for path in paths)


def first_line(value: str) -> str:
    return value.splitlines()[0] if value else ""


def listed_mcp_servers(output: str) -> set[str]:
    servers: set[str] = set()
    for line in output.splitlines():
        stripped = line.strip()
        if not stripped or stripped.startswith("Name ") or stripped.startswith("-"):
            continue
        servers.add(stripped.split()[0])
    return servers


def print_meta_plugin_markers(path: Path) -> bool:
    markers = find_meta_plugin_markers(path)
    if not markers:
        expected = path / "Engine" / "Plugins" / "Marketplace"
        print(f"  PENDING: Meta XR plugin {TARGET_META_INTEGRATION_VERSION} - expected under {expected}")
        return False
    for plugin_file, version in markers:
        status = "READY" if TARGET_META_INTEGRATION_VERSION in version else "FOUND"
        print(f"  {status}: Meta XR plugin candidate {version} - {plugin_file}")
    return has_target_meta_plugin(markers)


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


def run_tools_doctor() -> int:
    ensure_external_layout()
    home = external_home()
    engine = home / "engines" / f"UE_{TARGET_UNREAL_VERSION}"
    marketplace = engine / "Engine" / "Plugins" / "Marketplace"
    tools = home / "tools"

    meta_xr = marketplace / "MetaXR" / "OculusXR.uplugin"
    interaction = marketplace / "MetaXRInteraction" / "OculusInteraction.uplugin"
    movement = marketplace / "MetaXR" / "Source" / "OculusXRMovement" / "OculusXRMovement.Build.cs"
    simulator = [
        tools / "MetaXRSimulator" / "v201.0" / "MetaXRSimulator.exe",
        tools / "MetaXRSimulator" / "v201.0" / "meta_openxr_simulator.json",
    ]
    renderdoc = [
        Path("C:/Program Files/RenderDoc/qrenderdoc.exe"),
        Path("C:/Program Files/RenderDoc/renderdoccmd.exe"),
    ]
    pix = [
        Path("C:/Program Files/Microsoft PIX/2603.25/WinPix.exe"),
        Path("C:/Program Files/Microsoft PIX/2603.25/pixtool.exe"),
    ]
    openxr_explorer = [
        tools / "OpenXR-Explorer-v1.7" / "openxr-explorer.exe",
        tools / "OpenXR-Explorer-v1.7" / "xrsetruntime.exe",
    ]
    openxr_layers = [
        tools / "OpenXR-SDK-Source" / "bin" / "api_layers" / "XrApiLayer_api_dump.dll",
        tools / "OpenXR-SDK-Source" / "bin" / "api_layers" / "XrApiLayer_core_validation.dll",
        tools / "OpenXR-SDK-Source" / "bin" / "api_layers" / "XrApiLayer_best_practices_validation.dll",
    ]
    nsight = [
        tools / "NVIDIA_Nsight_Graphics_2026.2_Portable" / "host" / "windows-desktop-nomad-x64" / "ngfx.exe",
        tools / "NVIDIA_Nsight_Graphics_2026.2_Portable" / "host" / "windows-desktop-nomad-x64" / "ngfx-ui.exe",
    ]
    dlss_plugins = [
        marketplace / "DLSS" / "DLSS.uplugin",
        marketplace / "StreamlineCore" / "StreamlineCore.uplugin",
        marketplace / "StreamlineReflex" / "StreamlineReflex.uplugin",
        marketplace / "StreamlineDLSSG" / "StreamlineDLSSG.uplugin",
        marketplace / "StreamlineNGXCommon" / "StreamlineNGXCommon.uplugin",
    ]
    insights = engine / "Engine" / "Binaries" / "Win64" / "UnrealInsights.exe"

    checks = [
        ("Meta Quest Developer Hub", Path("C:/Program Files/Meta Quest Developer Hub/Meta Quest Developer Hub.exe").exists(), "6.4.1 expected"),
        ("Meta XR plugin", meta_xr.exists(), uplugin_detail(meta_xr)),
        ("Meta Interaction SDK", interaction.exists(), uplugin_detail(interaction)),
        ("Meta Movement SDK module", movement.exists(), str(movement)),
        ("Meta XR Simulator payload", files_ready(simulator), str(simulator[0])),
        ("RenderDoc", files_ready(renderdoc), str(renderdoc[0])),
        ("Microsoft PIX", files_ready(pix), str(pix[0])),
        ("OpenXR Explorer", files_ready(openxr_explorer), str(openxr_explorer[0])),
        ("Khronos OpenXR SDK API layers", files_ready(openxr_layers), str(openxr_layers[0].parent)),
        ("NVIDIA Nsight Graphics portable host", files_ready(nsight), str(nsight[0])),
        ("NVIDIA DLSS/Streamline UE plugins", files_ready(dlss_plugins), str(marketplace)),
        ("Unreal Insights", insights.exists(), str(insights)),
    ]

    codex_ok, codex_detail = run_checked(["codex", "mcp", "list"])
    checks.append(("Codex MCP config", codex_ok, "codex mcp list succeeded" if codex_ok else first_line(codex_detail)))
    if codex_ok:
        servers = listed_mcp_servers(codex_detail)
        for server in ("github", "tubemcp", "blender", "playwright", "context7", "ue-mcp"):
            checks.append((f"Codex MCP {server}", server in servers, "enabled in Codex MCP list"))

    failed = False
    for label, ok, detail in checks:
        print_status(label, ok, detail)
        failed = failed or not ok
    if files_ready(nsight):
        print("NOTE: Nsight Graphics is portable-extracted because its MSI requires elevation for registration.")
    if files_ready(simulator):
        print("NOTE: Meta XR Simulator is staged but not activated as the system OpenXR runtime.")
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
    launch_ready = False
    if found:
        for path in found:
            print(f"FOUND: {path}")
            engine_ready = print_engine_markers(path)
            slate_icons_ready = print_slate_launcher_icon_markers(path)
            meta_plugin_ready = print_meta_plugin_markers(path)
            launch_ready = launch_ready or (engine_ready and slate_icons_ready and meta_plugin_ready)
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

    return 1 if blocked or not launch_ready else 0
