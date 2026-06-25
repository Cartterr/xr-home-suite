param()

$ErrorActionPreference = "Stop"
$ProgressPreference = "SilentlyContinue"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$root = Split-Path -Parent $scriptDir
$thirdParty = Join-Path $root "third_party"
$headersDir = Join-Path $thirdParty "openxr_current\include\openxr"
$loaderDir = Join-Path $thirdParty "openxr_loader"
$loaderVersion = "1.0.10.2"
$loaderPackage = Join-Path $thirdParty "openxr.loader.$loaderVersion.nupkg"
$loaderUrl = "https://api.nuget.org/v3-flatcontainer/openxr.loader/$loaderVersion/openxr.loader.$loaderVersion.nupkg"

function Download-File {
    param(
        [Parameter(Mandatory = $true)][string]$Url,
        [Parameter(Mandatory = $true)][string]$Path
    )

    if (Test-Path -LiteralPath $Path) {
        return
    }

    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Path) | Out-Null
    Write-Host "Downloading $Url"

    $curl = Get-Command curl.exe -ErrorAction SilentlyContinue
    if ($curl) {
        & $curl.Source -L --fail --silent --show-error -o $Path $Url
        if ($LASTEXITCODE -ne 0) {
            throw "curl failed for $Url with exit code $LASTEXITCODE"
        }
    } else {
        Invoke-WebRequest -Uri $Url -OutFile $Path -UseBasicParsing
    }
}

New-Item -ItemType Directory -Force -Path $thirdParty, $headersDir | Out-Null

$headerNames = @(
    "openxr.h",
    "openxr_platform.h",
    "openxr_platform_defines.h",
    "openxr_reflection.h"
)

foreach ($headerName in $headerNames) {
    $url = "https://raw.githubusercontent.com/KhronosGroup/OpenXR-SDK/main/include/openxr/$headerName"
    $path = Join-Path $headersDir $headerName
    Download-File -Url $url -Path $path
}

Download-File -Url $loaderUrl -Path $loaderPackage

$loaderLib = Join-Path $loaderDir "native\x64\release\lib\openxr_loader.lib"
$loaderDll = Join-Path $loaderDir "native\x64\release\bin\openxr_loader.dll"
if (-not (Test-Path -LiteralPath $loaderLib) -or -not (Test-Path -LiteralPath $loaderDll)) {
    New-Item -ItemType Directory -Force -Path $loaderDir | Out-Null
    $tempZip = Join-Path $thirdParty "openxr.loader.$loaderVersion.zip"
    Copy-Item -LiteralPath $loaderPackage -Destination $tempZip -Force
    Expand-Archive -LiteralPath $tempZip -DestinationPath $loaderDir -Force
    Remove-Item -LiteralPath $tempZip -Force
}

$requiredFiles = @(
    (Join-Path $headersDir "openxr.h"),
    (Join-Path $headersDir "openxr_platform.h"),
    (Join-Path $headersDir "openxr_platform_defines.h"),
    (Join-Path $headersDir "openxr_reflection.h"),
    $loaderLib,
    $loaderDll
)

foreach ($file in $requiredFiles) {
    if (-not (Test-Path -LiteralPath $file)) {
        throw "Missing required OpenXR dependency: $file"
    }
}

Write-Host "OpenXR dependencies ready."
