param(
    [ValidateSet("skia", "direct2d")]
    [string]$Renderer = "skia",
    [switch]$BuildIfMissing,
    [switch]$NoLaunch
)

$ErrorActionPreference = "Stop"

$runner = Join-Path $PSScriptRoot "run_layout_demo.ps1"

if (-not (Test-Path $runner)) {
    throw "Missing runner script: $runner"
}

& $runner `
    -Renderer $Renderer `
    -Layout "layouts/dashboard_reference.xml" `
    -BuildIfMissing:$BuildIfMissing `
    -NoLaunch:$NoLaunch
