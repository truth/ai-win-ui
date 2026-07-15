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

# Mirror run_layout_demo Resolve-LauncherConfig: newest existing binary.
$presetExe = if ($Renderer -eq "skia") {
    Join-Path $repoRoot "build\presets\dev-debug-skia-local-sdk\Debug\ai_win_ui.exe"
} else {
    Join-Path $repoRoot "build\presets\dev-debug\Debug\ai_win_ui.exe"
}
$candidates = @(
    $presetExe,
    (Join-Path $repoRoot "build\Release\ai_win_ui.exe"),
    (Join-Path $repoRoot "build\Debug\ai_win_ui.exe")
)
$existing = @($candidates | Where-Object { Test-Path -LiteralPath $_ })
if ($existing.Count -eq 0) {
    throw "ai_win_ui.exe not found (tried preset + build/Release + build/Debug)"
}
$exe = ($existing | Sort-Object { (Get-Item -LiteralPath $_).LastWriteTime } -Descending | Select-Object -First 1)

if (-not $OutFile) {
    $OutFile = Join-Path (Split-Path -Parent $exe) "measure_dump.ndjson"
}

function Resolve-LayoutEnvPath([string]$layoutArg) {
    $key = $layoutArg.ToLowerInvariant()
    $aliases = @{
        "scroll-viewer" = "layouts/scroll_viewer_cases.xml"
        "scroll_viewer" = "layouts/scroll_viewer_cases.xml"
        "scroll_viewer_cases" = "layouts/scroll_viewer_cases.xml"
        "core-validation" = "layouts/core_validation.xml"
        "core_validation" = "layouts/core_validation.xml"
        "yoga-measure" = "layouts/yoga_measure_cases.xml"
        "yoga_measure" = "layouts/yoga_measure_cases.xml"
        "yoga_measure_cases" = "layouts/yoga_measure_cases.xml"
        "demo-gallery" = "layouts/demo_gallery.xml"
        "demo_gallery" = "layouts/demo_gallery.xml"
        "text-wrap" = "layouts/text_wrap_cases.xml"
        "text_wrap" = "layouts/text_wrap_cases.xml"
        "text_wrap_cases" = "layouts/text_wrap_cases.xml"
    }
    if ($aliases.ContainsKey($key)) {
        return $aliases[$key]
    }
    if ($layoutArg -match '^(layouts/|\w)') {
        return $layoutArg
    }
    return "layouts/$layoutArg"
}

$env:AI_WIN_UI_RENDERER = $Renderer
$env:AI_WIN_UI_LAYOUT = Resolve-LayoutEnvPath $Layout
$env:AI_WIN_UI_QUIT_AFTER_MS = "$QuitAfterMs"
$env:AI_WIN_UI_MEASURE_DUMP = $OutFile
if (-not $env:AI_WIN_UI_SIZE) {
    $env:AI_WIN_UI_SIZE = "1000x700"
}

Write-Host "Measure dump"
Write-Host "  Layout:   $($env:AI_WIN_UI_LAYOUT)"
Write-Host "  Renderer: $Renderer"
Write-Host "  Exe:      $exe"
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
