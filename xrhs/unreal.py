from __future__ import annotations

from pathlib import Path

from .paths import repo_root


PROJECT_PATH = repo_root() / "unreal" / "XRHSValidation" / "XRHSValidation.uproject"
EDITOR_USER_SETTINGS_PATH = (
    PROJECT_PATH.parent / "Saved" / "Config" / "WindowsEditor" / "EditorPerProjectUserSettings.ini"
)
EDITOR_SETTINGS_SECTION = "[/Script/UnrealEd.EditorPerProjectUserSettings]"

LINK_PREVIEW_SETTINGS = {
    "PreviewFeatureLevel": "1",
    "PreviewPlatformName": "Android",
    "PreviewShaderFormatName": "SF_VULKAN_ES31_ANDROID",
    "PreviewShaderPlatformName": "VULKAN_ES3_1_ANDROID_Preview",
    "PreviewDeviceProfileName": "Android_OpenXR",
    "bPreviewFeatureLevelActive": "True",
    "bPreviewFeatureLevelWasDefault": "False",
}


def format_launch_command(engine_root: Path) -> str:
    editor = engine_root / "Engine" / "Binaries" / "Win64" / "UnrealEditor.exe"
    return f'& "{editor}" "{PROJECT_PATH}" -log'


def patch_unreal_ini(path: Path, section: str, values: dict[str, str]) -> bool:
    path.parent.mkdir(parents=True, exist_ok=True)
    original = path.read_text(encoding="utf-8", errors="ignore") if path.exists() else ""
    lines = original.splitlines()
    output: list[str] = []
    changed = False
    in_target_section = False
    section_found = False
    inserted = False
    keys = set(values)

    def append_settings() -> None:
        nonlocal inserted, changed
        if inserted:
            return
        for key, value in values.items():
            output.append(f"{key}={value}")
        inserted = True
        changed = True

    for line in lines:
        stripped = line.strip()
        if stripped.startswith("[") and stripped.endswith("]"):
            if in_target_section:
                append_settings()
            in_target_section = stripped == section
            section_found = section_found or in_target_section
            output.append(line)
            continue
        if in_target_section and "=" in line:
            key = line.split("=", 1)[0].strip()
            if key in keys:
                changed = True
                continue
        output.append(line)

    if in_target_section:
        append_settings()
    if not section_found:
        if output and output[-1].strip():
            output.append("")
        output.append(section)
        append_settings()

    updated = "\n".join(output).rstrip() + "\n"
    if updated != original:
        path.write_text(updated, encoding="utf-8")
        return True
    return changed


def run_unreal_link_setup(engine_root: Path) -> int:
    changed = patch_unreal_ini(EDITOR_USER_SETTINGS_PATH, EDITOR_SETTINGS_SECTION, LINK_PREVIEW_SETTINGS)
    print("Unreal Link passthrough setup:")
    print(f"  {'UPDATED' if changed else 'READY'}: {EDITOR_USER_SETTINGS_PATH}")
    print("  Preview platform: Android Vulkan Mobile / Android_OpenXR")
    print("  Passthrough layer: overlay fallback by default; F2 toggles underlay alpha validation")
    print("")
    print("Safe launch flow:")
    print(f"  {format_launch_command(engine_root)}")
    print("  Then use the editor Play menu: VR Preview")
    return 0
