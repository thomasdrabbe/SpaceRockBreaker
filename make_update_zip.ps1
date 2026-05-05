param(
    [string]$Version = "",
    [switch]$SkipBuild
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Require-File([string]$Path) {
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        throw "Required file not found: $Path"
    }
}

$repoRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $repoRoot

$versionFile = Join-Path $repoRoot "version.txt"
Require-File $versionFile

if ($Version -ne "") {
    if ($Version -notmatch '^\d+\.\d+\.\d+$') {
        throw "Version must look like x.y.z (example: 1.0.2)"
    }
    Set-Content -LiteralPath $versionFile -Value $Version -NoNewline
    Write-Host "Set version.txt to $Version"
}

if (-not $SkipBuild) {
    Write-Host "Building release..."
    & cmake --build --preset release
    if ($LASTEXITCODE -ne 0) {
        throw "Release build failed."
    }
    Write-Host "Note: SRB_AUTO_BUMP_VERSION_ON_BUILD verhoogt patch + sync't installer.iss tijdens build."
}

$releaseDir  = Join-Path $repoRoot "build\Release"
$launcherDir = Join-Path $repoRoot "build\launcher\Release"
$assetsDir   = Join-Path $releaseDir "assets"

$gameExe     = Join-Path $releaseDir "SpaceRockBreaker.exe"
$launcherExe = Join-Path $launcherDir "SpaceRockLauncher.exe"

Require-File $gameExe
Require-File $launcherExe
if (-not (Test-Path -LiteralPath $assetsDir -PathType Container)) {
    throw "Assets folder not found: $assetsDir"
}

$stageDir = Join-Path $repoRoot ".update_package"
$zipRoot  = Join-Path $repoRoot "SpaceRockBreaker.zip"
$zipOutDir = Join-Path $repoRoot "installer_output"
$zipOut   = Join-Path $zipOutDir "SpaceRockBreaker.zip"

if (Test-Path -LiteralPath $stageDir) {
    Remove-Item -LiteralPath $stageDir -Recurse -Force
}
New-Item -ItemType Directory -Path $stageDir | Out-Null

Copy-Item -LiteralPath $gameExe -Destination $stageDir -Force
Copy-Item -LiteralPath $launcherExe -Destination $stageDir -Force
Copy-Item -LiteralPath $versionFile -Destination (Join-Path $stageDir "version.txt") -Force

Get-ChildItem -Path (Join-Path $releaseDir "*.dll") -File -ErrorAction SilentlyContinue |
    ForEach-Object { Copy-Item -LiteralPath $_.FullName -Destination $stageDir -Force }

Copy-Item -LiteralPath $assetsDir -Destination (Join-Path $stageDir "assets") -Recurse -Force

if (Test-Path -LiteralPath $zipRoot) {
    Remove-Item -LiteralPath $zipRoot -Force
}
Compress-Archive -Path (Join-Path $stageDir "*") -DestinationPath $zipRoot -Force

if (-not (Test-Path -LiteralPath $zipOutDir -PathType Container)) {
    New-Item -ItemType Directory -Path $zipOutDir | Out-Null
}
Copy-Item -LiteralPath $zipRoot -Destination $zipOut -Force

$currentVersion = (Get-Content -LiteralPath $versionFile -Raw).Trim()
$zipRootVersioned = Join-Path $repoRoot ("SpaceRockBreaker_{0}.zip" -f $currentVersion)
$zipOutVersioned  = Join-Path $zipOutDir ("SpaceRockBreaker_{0}.zip" -f $currentVersion)
Copy-Item -LiteralPath $zipRoot -Destination $zipRootVersioned -Force
Copy-Item -LiteralPath $zipRoot -Destination $zipOutVersioned -Force

$zipA = Get-Item -LiteralPath $zipRoot
$zipB = Get-Item -LiteralPath $zipOut

Write-Host ""
Write-Host "Done."
Write-Host "Version: $currentVersion"
Write-Host "Zip 1: $($zipA.FullName) ($($zipA.Length) bytes)"
Write-Host "Zip 2: $($zipB.FullName) ($($zipB.Length) bytes)"
Write-Host ""
Write-Host "Next: git add version.txt SpaceRockBreaker.zip installer_output/SpaceRockBreaker.zip"
