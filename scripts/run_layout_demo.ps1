param(
    [string]$Layout = "layouts/ui.xml",
    [ValidateSet("skia", "direct2d")]
    [string]$Renderer = "skia",
    [switch]$BuildIfMissing,
    [switch]$Rebuild,
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

    $layoutAliases = @{
        "core-validation" = "layouts/core_validation.xml"
        "core_validation" = "layouts/core_validation.xml"
        "yoga-measure" = "layouts/yoga_measure_cases.xml"
        "yoga_measure" = "layouts/yoga_measure_cases.xml"
        "yoga_measure_cases" = "layouts/yoga_measure_cases.xml"
        "skia-image" = "layouts/skia_image_cases.xml"
        "skia_image" = "layouts/skia_image_cases.xml"
        "skia_image_cases" = "layouts/skia_image_cases.xml"
        "stats-components" = "layouts/stats_components.xml"
        "stats_components" = "layouts/stats_components.xml"
        "table-components" = "layouts/table_components.xml"
        "table_components" = "layouts/table_components.xml"
        "dashboard-responsive" = "layouts/dashboard_responsive.xml"
        "dashboard_responsive" = "layouts/dashboard_responsive.xml"
        "dashboard" = "layouts/dashboard_responsive.xml"
        "seagull-animation" = "layouts/seagull_animation.xml"
        "seagull_animation" = "layouts/seagull_animation.xml"
        "core-controls-v2" = "layouts/core_controls_v2.xml"
        "core_controls_v2" = "layouts/core_controls_v2.xml"
        "navigation-components" = "layouts/navigation_components.xml"
        "navigation_components" = "layouts/navigation_components.xml"
        "advanced-inputs" = "layouts/advanced_inputs.xml"
        "advanced_inputs" = "layouts/advanced_inputs.xml"
        "custom-chrome" = "layouts/custom_chrome_demo.xml"
        "custom_chrome" = "layouts/custom_chrome_demo.xml"
        "custom_chrome_demo" = "layouts/custom_chrome_demo.xml"
        "layered-chrome" = "layouts/layered_chrome_demo.xml"
        "layered_chrome" = "layouts/layered_chrome_demo.xml"
        "layered_chrome_demo" = "layouts/layered_chrome_demo.xml"
        "style-catalog" = "layouts/style_catalog_demo.xml"
        "style_catalog" = "layouts/style_catalog_demo.xml"
        "style_catalog_demo" = "layouts/style_catalog_demo.xml"
        "shaped-hub" = "layouts/shaped_windows_hub.xml"
        "shaped_hub" = "layouts/shaped_windows_hub.xml"
        "shaped_windows_hub" = "layouts/shaped_windows_hub.xml"
        "shaped-heart" = "layouts/shaped_heart_window.xml"
        "shaped_heart" = "layouts/shaped_heart_window.xml"
        "shaped-petal" = "layouts/shaped_petal_window.xml"
        "shaped_petal" = "layouts/shaped_petal_window.xml"
        "scroll-viewer" = "layouts/scroll_viewer_cases.xml"
        "scroll_viewer" = "layouts/scroll_viewer_cases.xml"
        "scroll_viewer_cases" = "layouts/scroll_viewer_cases.xml"
        "demo-gallery" = "layouts/demo_gallery.xml"
        "demo_gallery" = "layouts/demo_gallery.xml"
        "gallery" = "layouts/demo_gallery.xml"
    }
    $aliasKey = $layoutPath.ToLowerInvariant()
    if ($layoutAliases.ContainsKey($aliasKey)) {
        $layoutPath = $layoutAliases[$aliasKey]
    }

    if ([System.IO.Path]::IsPathRooted($layoutPath)) {
        if (-not (Test-Path $layoutPath)) {
            throw "Layout file not found: $layoutPath"
        }
        return (Resolve-Path $layoutPath).Path
    }

    $resourceRoot = Join-Path $RepoRoot "resource"
    $candidates = @(
        ($layoutPath.Replace("/", "\")),
        ("layouts\" + $layoutPath.Replace("/", "\")),
        ("layouts\" + $layoutPath.Replace("/", "\") + ".xml"),
        ("layouts\" + $layoutPath.Replace("/", "\") + ".json")
    )
    # Bare stem without extension: layouts/<name>.xml then .json
    $leaf = [System.IO.Path]::GetFileName($layoutPath)
    if ($leaf -notmatch '\.(xml|json)$') {
        $candidates += @(
            ("layouts\" + $leaf + ".xml"),
            ("layouts\" + $leaf + ".json")
        )
    }

    foreach ($rel in $candidates) {
        $resourceCandidate = Join-Path $resourceRoot $rel
        if (Test-Path $resourceCandidate) {
            $full = (Resolve-Path $resourceCandidate).Path
            $prefix = (Resolve-Path $resourceRoot).Path.TrimEnd('\') + '\'
            if ($full.StartsWith($prefix, [System.StringComparison]::OrdinalIgnoreCase)) {
                return $full.Substring($prefix.Length).Replace("\", "/")
            }
            return $rel.Replace("\", "/")
        }
    }

    throw "Layout file not found under resource/: $RequestedLayout (tried layouts/ prefix and .xml/.json). Example: -Layout table-components or -Layout layouts/table_components.xml"
}

function Get-LatestSourceWriteTime {
    param([string]$RepoRoot)

    $roots = @(
        (Join-Path $RepoRoot "src"),
        (Join-Path $RepoRoot "CMakeLists.txt"),
        (Join-Path $RepoRoot "CMakePresets.json")
    )

    $latest = [datetime]::MinValue
    foreach ($root in $roots) {
        if (-not (Test-Path $root)) {
            continue
        }
        if (Test-Path $root -PathType Leaf) {
            $time = (Get-Item $root).LastWriteTime
            if ($time -gt $latest) {
                $latest = $time
            }
            continue
        }
        Get-ChildItem -Path $root -Recurse -File -Include *.cpp,*.h,*.hpp,*.c,*.cc,*.cmake |
            ForEach-Object {
                if ($_.LastWriteTime -gt $latest) {
                    $latest = $_.LastWriteTime
                }
            }
    }
    return $latest
}

function Test-ExeNeedsRebuild {
    param(
        [string]$RepoRoot,
        [string]$ExePath
    )

    if (-not (Test-Path $ExePath)) {
        return $true
    }

    $exeTime = (Get-Item $ExePath).LastWriteTime
    $sourceTime = Get-LatestSourceWriteTime -RepoRoot $RepoRoot
    return ($sourceTime -gt $exeTime)
}

function Ensure-Build {
    param(
        [string]$RepoRoot,
        [hashtable]$Config,
        [string]$Reason
    )

    Write-Host "Building ($Reason). Configure preset '$($Config.ConfigurePreset)'..."
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

$exeExists = Test-Path $config.ExePath
$needsRebuild = $Rebuild -or (-not $exeExists) -or (Test-ExeNeedsRebuild -RepoRoot $repoRoot -ExePath $config.ExePath)

if ($needsRebuild) {
    if (-not $Rebuild -and -not $BuildIfMissing -and $exeExists) {
        Write-Host "WARNING: Executable is older than source files:"
        Write-Host "  EXE: $($config.ExePath)"
        Write-Host "  Rebuilding automatically so new controls/behavior are available."
    }
    $reason = if ($Rebuild) {
        "forced -Rebuild"
    } elseif (-not $exeExists) {
        "executable missing"
    } else {
        "source newer than executable"
    }
    Ensure-Build -RepoRoot $repoRoot -Config $config -Reason $reason
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
Write-Host "  EXE time: $((Get-Item $config.ExePath).LastWriteTime)"
Write-Host "  Resource: synced to $(Join-Path (Split-Path -Parent $config.ExePath) 'resource')"

if ($NoLaunch) {
    Write-Host "  Launch:   skipped (-NoLaunch)"
    exit 0
}

Start-Process -FilePath $config.ExePath
