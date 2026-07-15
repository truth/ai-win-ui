param(
    [ValidateSet("Release", "Debug")]
    [string]$Configuration = "Release",
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

$env:AI_WIN_UI_CHROME = "custom"
$env:AI_WIN_UI_LAYOUT = "layouts/custom_chrome_demo.xml"

Write-Host "Chrome : custom"
Write-Host "Layout : layouts/custom_chrome_demo.xml"
Write-Host "Exe    : $exePath"

if (-not $NoLaunch) {
    Start-Process -FilePath $exePath -WorkingDirectory (Split-Path -Parent $exePath) -WindowStyle Normal
}
