param(
    [string]$Version = "",
    [switch]$AutoGit,
    [string]$CommitMessage = ""
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Require-File([string]$Path) {
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        throw "Required file not found: $Path"
    }
}

function Get-FirstLineTrimmed([string]$Path) {
    Require-File $Path
    $line = (Get-Content -LiteralPath $Path -TotalCount 1)
    if ($null -eq $line) { return "" }
    return $line.Trim()
}

$repoRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $repoRoot

$versionFile = Join-Path $repoRoot "version.txt"
$issFile = Join-Path $repoRoot "installer.iss"
$packScript = Join-Path $repoRoot "make_update_zip.ps1"

Require-File $versionFile
Require-File $issFile
Require-File $packScript

if ($Version -ne "") {
    if ($Version -notmatch '^\d+\.\d+\.\d+$') {
        throw "Version must look like x.y.z (example: 1.0.45)"
    }
    Set-Content -LiteralPath $versionFile -Value $Version -NoNewline
    Write-Host "version.txt set to $Version"
}

$currentVersion = Get-FirstLineTrimmed $versionFile
if ($currentVersion -notmatch '^\d+\.\d+\.\d+$') {
    throw "version.txt first line must be semver x.y.z (got '$currentVersion')."
}

$issText = Get-Content -LiteralPath $issFile -Raw
$updatedIssText = [regex]::Replace(
    $issText,
    '#define\s+AppVersion\s+"[^"]+"',
    "#define AppVersion   `"$currentVersion`"")
if ($updatedIssText -ne $issText) {
    Set-Content -LiteralPath $issFile -Value $updatedIssText -NoNewline
    Write-Host "installer.iss AppVersion synced to $currentVersion"
} else {
    Write-Host "installer.iss AppVersion already $currentVersion"
}

Write-Host "Repackaging update zips (no rebuild)..."
& powershell -ExecutionPolicy Bypass -File $packScript -SkipBuild
if ($LASTEXITCODE -ne 0) {
    throw "make_update_zip.ps1 -SkipBuild failed."
}

$zipPath = Join-Path $repoRoot "SpaceRockBreaker.zip"
Require-File $zipPath

Add-Type -AssemblyName System.IO.Compression.FileSystem
$zip = [System.IO.Compression.ZipFile]::OpenRead($zipPath)
try {
    $entry = $zip.Entries | Where-Object { $_.FullName -ieq "version.txt" } | Select-Object -First 1
    if ($null -eq $entry) {
        throw "SpaceRockBreaker.zip does not contain version.txt"
    }
    $reader = New-Object System.IO.StreamReader($entry.Open())
    try {
        $zipVersion = $reader.ReadToEnd().Trim()
    } finally {
        $reader.Dispose()
    }
} finally {
    $zip.Dispose()
}

if ($zipVersion -ne $currentVersion) {
    throw "Zip version mismatch: zip has '$zipVersion', expected '$currentVersion'"
}

Write-Host ""
Write-Host "Sync complete."
Write-Host "Version: $currentVersion"
Write-Host "Verified zip version.txt matches."

if (-not $AutoGit) {
    Write-Host "Now commit/push the changed files."
    return
}

Write-Host ""
Write-Host "AutoGit enabled: staging, committing, pushing..."

$null = (& git rev-parse --is-inside-work-tree 2>$null)
if ($LASTEXITCODE -ne 0) {
    throw "Current directory is not a git repository."
}

& git add -A
if ($LASTEXITCODE -ne 0) {
    throw "git add failed."
}

$pending = & git status --porcelain
if ([string]::IsNullOrWhiteSpace(($pending -join ""))) {
    Write-Host "No git changes to commit."
    return
}

$msg = $CommitMessage
if ([string]::IsNullOrWhiteSpace($msg)) {
    $msg = "versie $currentVersion"
}

& git commit -m $msg
if ($LASTEXITCODE -ne 0) {
    throw "git commit failed."
}

& git push
if ($LASTEXITCODE -ne 0) {
    throw "git push failed."
}

Write-Host "Git push complete."
