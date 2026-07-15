param(
    [ValidateSet("skia", "direct2d", "both")]
    [string]$Renderer = "skia",

    [ValidateSet("smoke", "full")]
    [string]$Profile = "smoke",

    [int]$QuitAfterMs = 1500,

    [switch]$SkipBuild
)

$ErrorActionPreference = "Stop"

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$layoutRunner = Join-Path $PSScriptRoot "run_layout_demo.ps1"
if (-not (Test-Path $layoutRunner)) {
    throw "Missing layout runner: $layoutRunner"
}

$layoutsSmoke = @(
    "core-validation",
    "yoga-measure",
    "core-controls-v2",
    "navigation-components",
    "table-components"
)
$layoutsFull = $layoutsSmoke + @(
    "skia-image",
    "stats-components",
    "seagull-animation",
    "advanced-inputs"
)
$layouts = if ($Profile -eq "full") { $layoutsFull } else { $layoutsSmoke }

$renderers = if ($Renderer -eq "both") { @("skia", "direct2d") } else { @($Renderer) }

function Resolve-LayoutRelPath([string]$alias) {
    switch ($alias) {
        "core-validation" { return "layouts/core_validation.xml" }
        "yoga-measure" { return "layouts/yoga_measure_cases.xml" }
        "core-controls-v2" { return "layouts/core_controls_v2.xml" }
        "navigation-components" { return "layouts/navigation_components.xml" }
        "table-components" { return "layouts/table_components.xml" }
        "skia-image" { return "layouts/skia_image_cases.xml" }
        "stats-components" { return "layouts/stats_components.xml" }
        "seagull-animation" { return "layouts/seagull_animation.xml" }
        "advanced-inputs" { return "layouts/advanced_inputs.xml" }
        default { return "layouts/$alias.xml" }
    }
}

$results = @()
foreach ($backend in $renderers) {
    foreach ($layout in $layouts) {
        Write-Host "=== $backend / $layout ==="

        if ($SkipBuild) {
            & $layoutRunner -Layout $layout -Renderer $backend -NoLaunch
        } else {
            & $layoutRunner -Layout $layout -Renderer $backend -NoLaunch
        }
        if ($LASTEXITCODE -ne 0) {
            $results += [pscustomobject]@{ Renderer = $backend; Layout = $layout; Ok = $false; Detail = "prepare failed" }
            Write-Host "FAIL prepare"
            continue
        }

        $exe = if ($backend -eq "skia") {
            Join-Path $repoRoot "build\presets\dev-debug-skia-local-sdk\Debug\ai_win_ui.exe"
        } else {
            Join-Path $repoRoot "build\presets\dev-debug\Debug\ai_win_ui.exe"
        }
        if (-not (Test-Path $exe)) {
            $results += [pscustomobject]@{ Renderer = $backend; Layout = $layout; Ok = $false; Detail = "exe missing" }
            continue
        }

        $env:AI_WIN_UI_RENDERER = $backend
        $env:AI_WIN_UI_LAYOUT = Resolve-LayoutRelPath $layout
        $env:AI_WIN_UI_QUIT_AFTER_MS = "$QuitAfterMs"

        $proc = Start-Process -FilePath $exe -WorkingDirectory (Split-Path $exe -Parent) -PassThru
        $waitMs = [Math]::Max(5000, $QuitAfterMs + 4000)
        $exited = $proc.WaitForExit($waitMs)
        if (-not $exited) {
            Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue
            $results += [pscustomobject]@{ Renderer = $backend; Layout = $layout; Ok = $false; Detail = "timeout" }
            Write-Host "FAIL timeout"
            continue
        }
        $code = $proc.ExitCode
        $ok = ($null -eq $code -or $code -eq 0)
        $results += [pscustomobject]@{ Renderer = $backend; Layout = $layout; Ok = $ok; Detail = "exit=$code" }
        if ($ok) { Write-Host "OK exit=$code" } else { Write-Host "FAIL exit=$code" }
    }
}

Remove-Item Env:AI_WIN_UI_QUIT_AFTER_MS -ErrorAction SilentlyContinue

Write-Host ""
Write-Host "Headless smoke summary"
$results | Format-Table -AutoSize
$failed = @($results | Where-Object { -not $_.Ok })
if ($failed.Count -gt 0) {
    throw "Headless smoke failed: $($failed.Count) case(s)"
}
Write-Host "All $($results.Count) cases passed."
