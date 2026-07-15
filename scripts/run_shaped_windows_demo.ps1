param(
    [ValidateSet("skia", "direct2d")]
    [string]$Renderer = "skia",
    [switch]$BuildIfMissing,
    [switch]$Rebuild,
    [switch]$NoLaunch
)

$ErrorActionPreference = "Stop"
$runner = Join-Path $PSScriptRoot "run_layout_demo.ps1"

& $runner -Layout "shaped-hub" -Renderer $Renderer -BuildIfMissing:$BuildIfMissing -Rebuild:$Rebuild -NoLaunch:$NoLaunch
