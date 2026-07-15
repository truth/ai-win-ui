param(
    [string]$Layout = "core-validation",
    [ValidateSet("skia", "direct2d")]
    [string]$Renderer = "skia",
    [string]$OutFile = "",
    [int]$QuitAfterMs = 900,
    [switch]$BuildIfMissing
)

$ErrorActionPreference = "Stop"
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$runner = Join-Path $PSScriptRoot "run_layout_demo.ps1"

& $runner -Layout $Layout -Renderer $Renderer -BuildIfMissing:$BuildIfMissing -NoLaunch
if ($LASTEXITCODE -ne 0) { throw "prepare failed" }

# Prefer newest binary (same policy as run_measure_dump / run_layout_demo).
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
    $OutFile = Join-Path (Split-Path -Parent $exe) "text_dump.ndjson"
}

$layoutRel = switch ($Layout.ToLowerInvariant()) {
    "core-validation" { "layouts/core_validation.xml" }
    "core_validation" { "layouts/core_validation.xml" }
    "cjk-render" { "layouts/cjk_render_test.xml" }
    "cjk_render" { "layouts/cjk_render_test.xml" }
    "text-wrap" { "layouts/text_wrap_cases.xml" }
    "text_wrap" { "layouts/text_wrap_cases.xml" }
    "scroll-viewer" { "layouts/scroll_viewer_cases.xml" }
    default {
        if ($Layout -match '^(layouts/|\w)') { $Layout } else { "layouts/$Layout" }
    }
}

$env:AI_WIN_UI_RENDERER = $Renderer
$env:AI_WIN_UI_LAYOUT = $layoutRel
$env:AI_WIN_UI_QUIT_AFTER_MS = "$QuitAfterMs"
$env:AI_WIN_UI_TEXT_DUMP = $OutFile
$env:AI_WIN_UI_SIZE = "1000x700"
Remove-Item Env:AI_WIN_UI_CHROME -ErrorAction SilentlyContinue

Write-Host "Text dump  Layout=$layoutRel  Renderer=$Renderer"
Write-Host "  Exe: $exe"
Write-Host "  Out: $OutFile"
$p = Start-Process -FilePath $exe -WorkingDirectory (Split-Path -Parent $exe) -PassThru -Wait
if ($p.ExitCode -ne 0) { throw "exit $($p.ExitCode)" }
if (-not (Test-Path $OutFile)) { throw "missing $OutFile" }
$n = (Get-Content $OutFile).Count
Write-Host "  lines=$n"
Get-Content $OutFile | Select-Object -First 6 | ForEach-Object { Write-Host "  $_" }
