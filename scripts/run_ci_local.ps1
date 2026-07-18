<#
.SYNOPSIS
  Wave2 Q8 — local CI gate: gallery coverage + headless smoke + measure goldens.

.DESCRIPTION
  One-command quality gate for ai-win-ui. Failures exit non-zero so GitHub Actions
  / agents can treat this as a required check.

  Steps (all default on):
    1. Optional Debug build (build.ps1)
    2. scripts/check_gallery_coverage.ps1
    3. scripts/run_headless_smoke.ps1  (-Profile smoke|full)
    4. scripts/run_measure_golden.ps1  for each golden layout × backends

.EXAMPLE
  .\scripts\run_ci_local.ps1
  .\scripts\run_ci_local.ps1 -SkipBuild -SmokeProfile full -Renderer both
  .\scripts\run_ci_local.ps1 -SkipSmoke   # golden + gallery only
#>
param(
    [ValidateSet("skia", "direct2d", "both")]
    [string]$Renderer = "both",

    [ValidateSet("smoke", "full")]
    [string]$SmokeProfile = "smoke",

    [switch]$SkipBuild,
    [switch]$SkipGallery,
    [switch]$SkipSmoke,
    [switch]$SkipGolden,

    [float]$GoldenTolerance = 1.5,

    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Debug"
)

$ErrorActionPreference = "Stop"
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
Set-Location $repoRoot

# Stable size for measure dumps (goldens captured at 1000x700).
if (-not $env:AI_WIN_UI_SIZE) {
    $env:AI_WIN_UI_SIZE = "1000x700"
}

$results = New-Object System.Collections.Generic.List[object]
$started = Get-Date

function Add-Result {
    param(
        [string]$Step,
        [bool]$Ok,
        [string]$Detail = ""
    )
    $status = if ($Ok) { "PASS" } else { "FAIL" }
    $results.Add([pscustomobject]@{
        Step   = $Step
        Status = $status
        Detail = $Detail
    }) | Out-Null
    $color = if ($Ok) { "Green" } else { "Red" }
    Write-Host ("[{0}] {1}{2}" -f $status, $Step, $(if ($Detail) { " — $Detail" } else { "" })) -ForegroundColor $color
}

function Invoke-External {
    param(
        [string]$Name,
        [string]$FilePath,
        [string[]]$ArgumentList = @()
    )
    Write-Host ""
    Write-Host "======== $Name ========" -ForegroundColor Cyan
    try {
        $argLine = ($ArgumentList | ForEach-Object {
            if ($_ -match '\s') { '"{0}"' -f $_ } else { $_ }
        }) -join ' '
        # Nested powershell so child exit codes are isolated.
        $psi = @(
            "-NoProfile",
            "-ExecutionPolicy", "Bypass",
            "-File", $FilePath
        ) + $ArgumentList
        & powershell.exe @psi
        $code = $LASTEXITCODE
        if ($null -eq $code) { $code = 0 }
        if ($code -ne 0) {
            Add-Result -Step $Name -Ok:$false -Detail ("exit=$code")
            return $false
        }
        Add-Result -Step $Name -Ok:$true
        return $true
    } catch {
        Add-Result -Step $Name -Ok:$false -Detail $_.Exception.Message
        return $false
    }
}

# Auto-fallback: no local Skia SDK → Direct2D only (matches GitHub Actions).
$skiaLib = Join-Path $repoRoot "third_party\skia-m124\out\Debug-x64\skia.lib"
if ($Renderer -eq "both" -and -not (Test-Path -LiteralPath $skiaLib)) {
    Write-Host "NOTE: Skia SDK not found; forcing -Renderer direct2d (CI path)." -ForegroundColor Yellow
    $Renderer = "direct2d"
}

Write-Host "AI WinUI local CI (Q8)" -ForegroundColor Cyan
Write-Host "  Root:     $repoRoot"
Write-Host "  Renderer: $Renderer"
Write-Host "  Smoke:    $SmokeProfile"
Write-Host "  Config:   $Configuration"
Write-Host "  Size:     $env:AI_WIN_UI_SIZE"
Write-Host ""

$allOk = $true

# --- Build ---
if (-not $SkipBuild) {
    $ok = Invoke-External -Name "build ($Configuration)" `
        -FilePath (Join-Path $repoRoot "build.ps1") `
        -ArgumentList @("-Configuration", $Configuration)
    if (-not $ok) { $allOk = $false }
} else {
    Add-Result -Step "build" -Ok:$true -Detail "skipped"
}

# --- Gallery coverage ---
if (-not $SkipGallery) {
    $ok = Invoke-External -Name "gallery coverage" `
        -FilePath (Join-Path $PSScriptRoot "check_gallery_coverage.ps1")
    if (-not $ok) { $allOk = $false }
} else {
    Add-Result -Step "gallery coverage" -Ok:$true -Detail "skipped"
}

# --- Headless smoke ---
if (-not $SkipSmoke) {
    $backends = if ($Renderer -eq "both") { @("skia", "direct2d") } else { @($Renderer) }
    foreach ($backend in $backends) {
        $ok = Invoke-External -Name "headless smoke ($backend / $SmokeProfile)" `
            -FilePath (Join-Path $PSScriptRoot "run_headless_smoke.ps1") `
            -ArgumentList @("-Renderer", $backend, "-Profile", $SmokeProfile, "-SkipBuild")
        if (-not $ok) { $allOk = $false }
    }
} else {
    Add-Result -Step "headless smoke" -Ok:$true -Detail "skipped"
}

# --- Measure goldens ---
if (-not $SkipGolden) {
    $goldenLayouts = [System.Collections.Generic.List[string]]@(
        "core-validation",
        "yoga-measure",
        "scroll-viewer"
    )
    $goldenDir = Join-Path $repoRoot "tests\golden"
    if (Test-Path $goldenDir) {
        Get-ChildItem -LiteralPath $goldenDir -Filter "*.ndjson" | ForEach-Object {
            $base = $_.BaseName
            if ($base -match '^(?<layout>.+)\.(skia|direct2d)$') {
                $layout = $Matches['layout']
                if (-not $goldenLayouts.Contains($layout)) {
                    $goldenLayouts.Add($layout) | Out-Null
                }
            }
        }
    }

    foreach ($layout in $goldenLayouts) {
        $ok = Invoke-External -Name "measure golden ($layout / $Renderer)" `
            -FilePath (Join-Path $PSScriptRoot "run_measure_golden.ps1") `
            -ArgumentList @(
                "-Layout", $layout,
                "-Renderer", $Renderer,
                "-Tolerance", "$GoldenTolerance"
            )
        if (-not $ok) { $allOk = $false }
    }
} else {
    Add-Result -Step "measure golden" -Ok:$true -Detail "skipped"
}

$elapsed = (Get-Date) - $started
Write-Host ""
Write-Host "======== CI summary ========" -ForegroundColor Cyan
$results | Format-Table -AutoSize | Out-String | Write-Host
$failCount = @($results | Where-Object { $_.Status -eq "FAIL" }).Count
$passCount = @($results | Where-Object { $_.Status -eq "PASS" }).Count
Write-Host ("Passed: {0}  Failed: {1}  Elapsed: {2:N1}s" -f $passCount, $failCount, $elapsed.TotalSeconds)

if (-not $allOk -or $failCount -gt 0) {
    Write-Host "CI FAILED" -ForegroundColor Red
    exit 1
}
Write-Host "CI PASSED" -ForegroundColor Green
exit 0
