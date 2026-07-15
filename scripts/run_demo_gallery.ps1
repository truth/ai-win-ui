param(
    [ValidateSet("skia", "direct2d")]
    [string]$Renderer = "skia",
    [switch]$BuildIfMissing,
    [switch]$Rebuild,
    [switch]$NoLaunch
)

$ErrorActionPreference = "Stop"
$runner = Join-Path $PSScriptRoot "run_layout_demo.ps1"

# Clear sticky chrome/layout from prior demos so the gallery hub is predictable.
Remove-Item Env:AI_WIN_UI_CHROME -ErrorAction SilentlyContinue
Remove-Item Env:AI_WIN_UI_SIZE -ErrorAction SilentlyContinue
$env:AI_WIN_UI_SIZE = "1280x860"

& $runner -Layout "demo-gallery" -Renderer $Renderer -BuildIfMissing:$BuildIfMissing -Rebuild:$Rebuild -NoLaunch:$NoLaunch
