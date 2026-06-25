param(
    [switch]$Run,
    [int]$Seconds = 30
)

$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$depsScript = Join-Path $root "scripts\ensure-openxr-deps.ps1"
& $depsScript

$buildDir = Join-Path $root "build"
$src = Join-Path $root "src\live_link_app.cpp"
$includeDir = Join-Path $root "third_party\openxr_current\include"
$loaderLibDir = Join-Path $root "third_party\openxr_loader\native\x64\release\lib"
$loaderDll = Join-Path $root "third_party\openxr_loader\native\x64\release\bin\openxr_loader.dll"
$exe = Join-Path $buildDir "xr_home_suite_link_mr.exe"
$obj = Join-Path $buildDir "live_link_app.obj"

New-Item -ItemType Directory -Force -Path $buildDir | Out-Null

$process = Get-Process -Id $PID
$oldPriority = $process.PriorityClass
try {
    $process.PriorityClass = "BelowNormal"

    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (-not (Test-Path -LiteralPath $vswhere)) {
        throw "vswhere.exe was not found. Install Visual Studio Build Tools 2022 with MSVC x64 tools."
    }

    $vsPath = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if (-not $vsPath) {
        throw "Visual Studio Build Tools with MSVC x64 tools were not found."
    }

    $vcvars = Join-Path $vsPath "VC\Auxiliary\Build\vcvars64.bat"
    if (-not (Test-Path -LiteralPath $vcvars)) {
        throw "vcvars64.bat was not found at $vcvars"
    }

    $compile = @(
        "call `"$vcvars`" >nul",
        "cl /nologo /std:c++17 /EHsc /O2 /W4 /I`"$includeDir`" `"$src`" /Fo`"$obj`" /Fe`"$exe`" /link /LIBPATH:`"$loaderLibDir`" openxr_loader.lib d3d11.lib dxgi.lib d3dcompiler.lib"
    ) -join " && "

    & cmd.exe /d /s /c $compile
    if ($LASTEXITCODE -ne 0) {
        throw "cl failed with exit code $LASTEXITCODE"
    }

    Copy-Item -LiteralPath $loaderDll -Destination (Join-Path $buildDir "openxr_loader.dll") -Force

    if ($Run) {
        & $exe --seconds $Seconds
    } else {
        Write-Host "Built $exe"
    }
}
finally {
    $process.PriorityClass = $oldPriority
}
