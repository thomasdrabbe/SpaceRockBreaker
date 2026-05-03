# Splits één audio in N gelijke stukken → shot_01.wav … shot_N.wav (default N=14)
# Vereist ffmpeg + ffprobe in PATH.
# Gebruik: plaats je fragment als shot_source.wav in deze map, of:
#   .\split_into_10_shots.ps1 -Source "C:\pad\naar\fragment.wav" -Count 14

param(
    [string] $Source = (Join-Path $PSScriptRoot "shot_source.wav"),
    [int]    $Count  = 14
)

$ErrorActionPreference = "Stop"
if ($Count -lt 1) {
    Write-Host "-Count moet minstens 1 zijn." -ForegroundColor Red
    exit 1
}
if (-not (Test-Path -LiteralPath $Source)) {
    Write-Host "Bronbestand ontbreekt: $Source" -ForegroundColor Red
    Write-Host "Zet je geluidsfragment hier als shot_source.wav of geef -Source mee."
    exit 1
}

$ffprobe = Get-Command ffprobe -ErrorAction SilentlyContinue
$ffmpeg  = Get-Command ffmpeg  -ErrorAction SilentlyContinue
if (-not $ffprobe -or -not $ffmpeg) {
    Write-Host "ffmpeg/ffprobe niet gevonden in PATH." -ForegroundColor Red
    exit 1
}

$durJson = ffprobe -v error -show_entries format=duration -of json $Source | Out-String
$m = [regex]::Match($durJson, '"duration"\s*:\s*"([0-9.]+)"')
if (-not $m.Success) {
    Write-Host "Kon duur niet lezen uit ffprobe-output." -ForegroundColor Red
    exit 1
}
$total = [double]$m.Groups[1].Value
$each  = $total / [double]$Count
Write-Host "Totaal ${total}s → $Count × ${each}s"

for ($i = 0; $i -lt $Count; $i++) {
    $start = $i * $each
    $out   = Join-Path $PSScriptRoot ("shot_{0:D2}.wav" -f ($i + 1))
    & ffmpeg -y -hide_banner -loglevel error -ss $start -i $Source -t $each -acodec pcm_s16le -ar 44100 -ac 2 $out
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    Write-Host "  $out"
}
Write-Host 'Klaar. Herstart het spel; het laadt gun sound.mp3 (14 segmenten) of deze WAVs als fallback.'
