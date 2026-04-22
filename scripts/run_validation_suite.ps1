param(
    [ValidateSet("both", "skia", "direct2d")]
    [string]$Renderer = "both",

    [ValidateSet("smoke", "full")]
    [string]$Profile = "full",

    [switch]$SkipBuild,
    [switch]$Launch,
    [switch]$ContinueOnError
)

$ErrorActionPreference = "Stop"

function Get-RendererList {
    param([string]$RendererMode)
    switch ($RendererMode) {
        "skia" { return @("skia") }
        "direct2d" { return @("direct2d") }
        default { return @("direct2d", "skia") }
    }
}

function Get-LayoutList {
    param([string]$ProfileName)

    $smoke = @(
        "core-validation",
        "yoga-measure",
        "core-controls-v2"
    )

    $full = @(
        "core-validation",
        "yoga-measure",
        "skia-image",
        "stats-components",
        "table-components",
        "seagull-animation",
        "core-controls-v2",
        "navigation-components"
    )

    if ($ProfileName -eq "smoke") {
        return $smoke
    }
    return $full
}

function Invoke-BuildPreset {
    param([string]$Preset)
    Write-Host "Building preset: $Preset"
    & cmake --build --preset $Preset
    if ($LASTEXITCODE -ne 0) {
        throw "Build failed for preset '$Preset'."
    }
}

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$layoutRunner = Join-Path $PSScriptRoot "run_layout_demo.ps1"

if (-not (Test-Path $layoutRunner)) {
    throw "Missing layout runner: $layoutRunner"
}

$renderers = Get-RendererList -RendererMode $Renderer
$layouts = Get-LayoutList -ProfileName $Profile
$noLaunch = -not $Launch

if (-not $SkipBuild) {
    if ($renderers -contains "direct2d") {
        Invoke-BuildPreset -Preset "build-dev-debug"
    }
    if ($renderers -contains "skia") {
        Invoke-BuildPreset -Preset "build-dev-debug-skia-local-sdk"
    }
}

$results = @()
$startedAt = Get-Date

foreach ($backend in $renderers) {
    foreach ($layout in $layouts) {
        $stepName = "$backend / $layout"
        Write-Host ""
        Write-Host "Running validation: $stepName"

        try {
            & $layoutRunner `
                -Renderer $backend `
                -Layout $layout `
                -NoLaunch:$noLaunch `
                -BuildIfMissing:$false

            $results += [pscustomobject]@{
                Step = $stepName
                Status = "PASS"
                Message = ""
            }
        } catch {
            $results += [pscustomobject]@{
                Step = $stepName
                Status = "FAIL"
                Message = $_.Exception.Message
            }

            if (-not $ContinueOnError) {
                Write-Host ""
                Write-Host "Validation failed at: $stepName"
                Write-Host "Use -ContinueOnError to keep running remaining items."
                throw
            }
        }
    }
}

$elapsed = (Get-Date) - $startedAt
$failed = @($results | Where-Object { $_.Status -eq "FAIL" })

Write-Host ""
Write-Host "Validation Summary"
$results | Format-Table -AutoSize
Write-Host ("Elapsed: {0:n1}s" -f $elapsed.TotalSeconds)

if ($failed.Count -gt 0) {
    Write-Host ("Failures: {0}" -f $failed.Count)
    exit 1
}

Write-Host "All validation steps passed."
exit 0
