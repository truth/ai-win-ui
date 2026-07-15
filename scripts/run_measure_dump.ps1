param(
    [string]$Layout = "layouts/scroll_viewer_cases.xml",
    [ValidateSet("skia", "direct2d")]
    [string]$Renderer = "skia",
    [string]$OutFile = "",
    [int]$QuitAfterMs = 800,
    [switch]$BuildIfMissing
)

$ErrorActionPreference = "Stop"
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$runner = Join-Path $PSScriptRoot "run_layout_demo.ps1"

& $runner -Layout $Layout -Renderer $Renderer -BuildIfMissing:$BuildIfMissing -NoLaunch
if ($LASTEXITCODE -ne 0) {
    throw "prepare failed"
}

$exe = if ($Renderer -eq "skia") {
    Join-Path $repoRoot "build\presets\dev-debug-skia-local-sdk\Debug\ai_win_ui.exe"
} else {
    Join-Path $repoRoot "build\presets\dev-debug\Debug\ai_win_ui.exe"
}

if (-not $OutFile) {
    $OutFile = Join-Path (Split-Path -Parent $exe) "measure_dump.ndjson"
}

$env:AI_WIN_UI_RENDERER = $Renderer
$env:AI_WIN_UI_LAYOUT = if ($Layout -match '^(layouts/|\w)') { $Layout } else { "layouts/$Layout" }
# Resolve via runner aliases already done when path is full alias - re-set from env after runner
# Prefer explicit path under resource when alias was used:
if ($Layout -eq "scroll-viewer" -or $Layout -eq "scroll_viewer") {
    $env:AI_WIN_UI_LAYOUT = "layouts/scroll_viewer_cases.xml"
}
$env:AI_WIN_UI_QUIT_AFTER_MS = "$QuitAfterMs"
$env:AI_WIN_UI_MEASURE_DUMP = $OutFile

Write-Host "Measure dump"
Write-Host "  Layout:   $($env:AI_WIN_UI_LAYOUT)"
Write-Host "  Renderer: $Renderer"
Write-Host "  Out:      $OutFile"

$p = Start-Process -FilePath $exe -WorkingDirectory (Split-Path -Parent $exe) -PassThru -Wait
if ($p.ExitCode -ne 0) {
    throw "process exit $($p.ExitCode)"
}
if (-not (Test-Path $OutFile)) {
    throw "dump file missing: $OutFile"
}

$lines = Get-Content $OutFile
Write-Host "  Lines:    $($lines.Count)"
$lines | Select-Object -First 8 | ForEach-Object { Write-Host "  $_" }
