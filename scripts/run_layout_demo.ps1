param(
    [string]$Layout = "layouts/ui.xml",
    [ValidateSet("skia", "direct2d")]
    [string]$Renderer = "skia",
    [switch]$BuildIfMissing,
    [switch]$NoLaunch
)

$ErrorActionPreference = "Stop"

function Resolve-LauncherConfig {
    param(
        [string]$RepoRoot,
        [string]$Backend
    )

    switch ($Backend) {
        "skia" {
            return @{
                ExePath = Join-Path $RepoRoot "build\presets\dev-debug-skia-local-sdk\Debug\ai_win_ui.exe"
                ConfigurePreset = "dev-debug-skia-local-sdk"
                BuildPreset = "build-dev-debug-skia-local-sdk"
            }
        }
        "direct2d" {
            return @{
                ExePath = Join-Path $RepoRoot "build\presets\dev-debug\Debug\ai_win_ui.exe"
                ConfigurePreset = "dev-debug"
                BuildPreset = "build-dev-debug"
            }
        }
        default {
            throw "Unsupported renderer: $Backend"
        }
    }
}

function Resolve-LayoutPath {
    param(
        [string]$RepoRoot,
        [string]$RequestedLayout
    )

    $layoutPath = $RequestedLayout.Trim()
    if ([string]::IsNullOrWhiteSpace($layoutPath)) {
        throw "Layout path cannot be empty."
    }

    if ([System.IO.Path]::IsPathRooted($layoutPath)) {
        if (-not (Test-Path $layoutPath)) {
            throw "Layout file not found: $layoutPath"
        }
        return (Resolve-Path $layoutPath).Path
    }

    $resourceCandidate = Join-Path $RepoRoot ("resource\" + $layoutPath.Replace("/", "\"))
    if (-not (Test-Path $resourceCandidate)) {
        throw "Layout file not found under resource/: $layoutPath"
    }

    return $layoutPath.Replace("\", "/")
}

function Ensure-Build {
    param(
        [string]$RepoRoot,
        [hashtable]$Config
    )

    Write-Host "Build output missing. Configuring preset '$($Config.ConfigurePreset)'..."
    & cmake --preset $Config.ConfigurePreset
    if ($LASTEXITCODE -ne 0) {
        throw "Configure failed for preset '$($Config.ConfigurePreset)'."
    }

    Write-Host "Building preset '$($Config.BuildPreset)'..."
    & cmake --build --preset $Config.BuildPreset
    if ($LASTEXITCODE -ne 0) {
        throw "Build failed for preset '$($Config.BuildPreset)'."
    }
}

function Sync-Resources {
    param(
        [string]$RepoRoot,
        [string]$ExePath
    )

    $sourceResourceDir = Join-Path $RepoRoot "resource"
    $targetDir = Split-Path -Parent $ExePath
    $targetResourceDir = Join-Path $targetDir "resource"

    if (-not (Test-Path $sourceResourceDir)) {
        throw "Source resource directory not found: $sourceResourceDir"
    }

    New-Item -ItemType Directory -Force -Path $targetResourceDir | Out-Null
    Copy-Item -Path (Join-Path $sourceResourceDir "*") -Destination $targetResourceDir -Recurse -Force
}

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$config = Resolve-LauncherConfig -RepoRoot $repoRoot -Backend $Renderer
$resolvedLayout = Resolve-LayoutPath -RepoRoot $repoRoot -RequestedLayout $Layout

if (-not (Test-Path $config.ExePath)) {
    if (-not $BuildIfMissing) {
        throw "Executable not found: $($config.ExePath). Re-run with -BuildIfMissing to configure and build automatically."
    }
    Ensure-Build -RepoRoot $repoRoot -Config $config
}

if (-not (Test-Path $config.ExePath)) {
    throw "Executable still not found after build: $($config.ExePath)"
}

Sync-Resources -RepoRoot $repoRoot -ExePath $config.ExePath

$env:AI_WIN_UI_RENDERER = $Renderer
$env:AI_WIN_UI_LAYOUT = $resolvedLayout

Write-Host "Prepared ai_win_ui launch"
Write-Host "  Renderer: $Renderer"
Write-Host "  Layout:   $resolvedLayout"
Write-Host "  EXE:      $($config.ExePath)"
Write-Host "  Resource: synced to $(Join-Path (Split-Path -Parent $config.ExePath) 'resource')"

if ($NoLaunch) {
    Write-Host "  Launch:   skipped (-NoLaunch)"
    exit 0
}

Start-Process -FilePath $config.ExePath
