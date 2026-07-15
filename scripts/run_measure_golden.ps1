param(
    [string]$Layout = "scroll-viewer",
    [ValidateSet("skia", "direct2d", "both")]
    [string]$Renderer = "skia",
    [float]$Tolerance = 1.5,
    [switch]$UpdateGolden,
    [switch]$BuildIfMissing
)

$ErrorActionPreference = "Stop"
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$dumpRunner = Join-Path $PSScriptRoot "run_measure_dump.ps1"

function Resolve-GoldenName([string]$layoutAlias) {
    $safe = $layoutAlias -replace '[\\/:*?"<>|]', '_'
    return $safe
}

function Read-NdjsonMap([string]$path) {
    $map = @{}
    Get-Content -LiteralPath $path | ForEach-Object {
        $line = $_.Trim()
        if (-not $line) { return }
        $obj = $line | ConvertFrom-Json
        $map[$obj.path] = $obj
    }
    return $map
}

function Compare-Maps($actual, $golden, [float]$tol) {
    $failures = @()
    foreach ($key in $golden.Keys) {
        if (-not $actual.ContainsKey($key)) {
            $failures += "missing path: $key"
            continue
        }
        $a = $actual[$key]
        $g = $golden[$key]
        foreach ($field in @("dw", "dh", "w", "h")) {
            $av = [double]$a.$field
            $gv = [double]$g.$field
            if ([math]::Abs($av - $gv) > $tol) {
                $failures += ("{0}.{1}: actual={2:N2} golden={3:N2} (tol={4})" -f $key, $field, $av, $gv, $tol)
            }
        }
    }
    return $failures
}

$renderers = if ($Renderer -eq "both") { @("skia", "direct2d") } else { @($Renderer) }
$layoutRel = switch ($Layout.ToLowerInvariant()) {
    "scroll-viewer" { "layouts/scroll_viewer_cases.xml" }
    "scroll_viewer" { "layouts/scroll_viewer_cases.xml" }
    "core-validation" { "layouts/core_validation.xml" }
    "core_validation" { "layouts/core_validation.xml" }
    "yoga-measure" { "layouts/yoga_measure_cases.xml" }
    "yoga_measure" { "layouts/yoga_measure_cases.xml" }
    "yoga_measure_cases" { "layouts/yoga_measure_cases.xml" }
    "demo-gallery" { "layouts/demo_gallery.xml" }
    "demo_gallery" { "layouts/demo_gallery.xml" }
    "text-wrap" { "layouts/text_wrap_cases.xml" }
    "text_wrap" { "layouts/text_wrap_cases.xml" }
    default { $Layout }
}

# Fixed client size so goldens are stable across machines.
if (-not $env:AI_WIN_UI_SIZE) {
    $env:AI_WIN_UI_SIZE = "1000x700"
}

$allOk = $true
foreach ($backend in $renderers) {
    $goldenName = "$(Resolve-GoldenName $Layout).$backend.ndjson"
    $goldenPath = Join-Path $repoRoot "tests\golden\$goldenName"
    $tmpOut = Join-Path $env:TEMP ("ai_win_ui_measure_{0}_{1}.ndjson" -f $backend, [guid]::NewGuid().ToString("N"))

    Write-Host "=== $backend / $Layout ==="
    & $dumpRunner -Layout $Layout -Renderer $backend -OutFile $tmpOut -BuildIfMissing:$BuildIfMissing
    if ($LASTEXITCODE -ne 0) {
        Write-Host "FAIL dump"
        $allOk = $false
        continue
    }

    if ($UpdateGolden) {
        $dir = Split-Path -Parent $goldenPath
        New-Item -ItemType Directory -Force -Path $dir | Out-Null
        Copy-Item -LiteralPath $tmpOut -Destination $goldenPath -Force
        Write-Host "UPDATED golden: $goldenPath"
        continue
    }

    if (-not (Test-Path $goldenPath)) {
        Write-Host "FAIL missing golden: $goldenPath (run with -UpdateGolden)"
        $allOk = $false
        continue
    }

    $actualMap = Read-NdjsonMap $tmpOut
    $goldenMap = Read-NdjsonMap $goldenPath
    $fails = Compare-Maps $actualMap $goldenMap $Tolerance
    if ($fails.Count -eq 0) {
        Write-Host "OK ($($actualMap.Count) nodes, tol=$Tolerance)"
    } else {
        $allOk = $false
        Write-Host "FAIL $($fails.Count) diffs:"
        $fails | Select-Object -First 20 | ForEach-Object { Write-Host "  $_" }
    }
    Remove-Item -LiteralPath $tmpOut -ErrorAction SilentlyContinue
}

if (-not $allOk) { exit 1 }
exit 0
