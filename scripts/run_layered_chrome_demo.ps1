param(
    [ValidateSet("Release", "Debug")]
    [string]$Configuration = "Release",
    [ValidateSet("skia", "direct2d")]
    [string]$Renderer = "skia",
    [switch]$BuildIfMissing,
    [switch]$NoLaunch
)

$ErrorActionPreference = "Stop"
$RepoRoot = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
$exePath = Join-Path $RepoRoot "build\$Configuration\ai_win_ui.exe"

if (-not (Test-Path -LiteralPath $exePath)) {
    if ($BuildIfMissing) {
        Write-Host "Building $Configuration..."
        & (Join-Path $RepoRoot "build.ps1") -Configuration $Configuration
        if ($LASTEXITCODE -ne 0) {
            throw "Build failed with exit code $LASTEXITCODE"
        }
    } else {
        throw "Executable not found: $exePath (pass -BuildIfMissing to compile)"
    }
}

$env:AI_WIN_UI_CHROME = "layered"
$env:AI_WIN_UI_LAYOUT = "layouts/layered_chrome_demo.xml"
$env:AI_WIN_UI_RENDERER = $Renderer

Write-Host "Chrome   : layered (per-pixel alpha)"
Write-Host "Renderer : $Renderer"
Write-Host "Layout   : layouts/layered_chrome_demo.xml"
Write-Host "Exe      : $exePath"

if (-not $NoLaunch) {
    # Prefer direct invocation so the new process can take foreground focus.
    # Start-Process often leaves layered windows only on the taskbar until clicked.
    Push-Location (Split-Path -Parent $exePath)
    try {
        Start-Process -FilePath $exePath -WorkingDirectory (Get-Location) -WindowStyle Normal
    } finally {
        Pop-Location
    }
}
