# Verify demo_gallery.xml openDemo targets exist under resource/.
# Exit 1 if any missing or if major layout files are not referenced.
param(
    [switch]$Strict  # also fail when resource layouts are missing from gallery
)

$ErrorActionPreference = "Stop"
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$gallery = Join-Path $repoRoot "resource\layouts\demo_gallery.xml"
$resourceLayouts = Join-Path $repoRoot "resource\layouts"

if (-not (Test-Path $gallery)) {
    throw "Missing gallery: $gallery"
}

$galleryText = Get-Content -LiteralPath $gallery -Raw
# Only real button handlers: onClick="openDemo:..."
$refs = [regex]::Matches($galleryText, 'onClick\s*=\s*"openDemo:([^"|]+)') |
    ForEach-Object { $_.Groups[1].Value.Trim() } |
    Sort-Object -Unique

Write-Host "Gallery openDemo targets: $($refs.Count)"
$missing = @()
foreach ($rel in $refs) {
    $path = Join-Path $repoRoot ("resource\" + ($rel -replace '/', '\'))
    if (-not (Test-Path -LiteralPath $path)) {
        $missing += $rel
        Write-Host "  MISSING $rel"
    } else {
        Write-Host "  OK      $rel"
    }
}

# Layouts that are intentionally not gallery cards (child shells / duplicates).
$optionalSkip = @(
    'shaped_heart_window.xml',
    'shaped_petal_window.xml',
    'shaped_oval_window.xml',
    'shaped_star_window.xml',
    'demo_gallery.xml',
    'ui.json',
    'advanced_inputs.json',
    'cjk_render_test.json',
    'core_controls_v2.json',
    'navigation_components.json',
    'seagull_animation.json',
    'skia_image_cases.json',
    'stats_components.json',
    'style_catalog_demo.json',
    'table_components.json',
    'yoga_measure_cases.json'
)

$unlisted = @()
if ($Strict) {
    Get-ChildItem -LiteralPath $resourceLayouts -File | ForEach-Object {
        $name = $_.Name
        if ($optionalSkip -contains $name) { return }
        $relXml = "layouts/$name"
        $referenced = $false
        foreach ($r in $refs) {
            if ($r -eq $relXml -or $r.EndsWith("/$name")) { $referenced = $true; break }
        }
        if (-not $referenced) {
            $unlisted += $name
        }
    }
    if ($unlisted.Count -gt 0) {
        Write-Host "Unlisted layout files (Strict):"
        $unlisted | ForEach-Object { Write-Host "  $_" }
    }
}

if ($missing.Count -gt 0) {
    Write-Host "FAIL: $($missing.Count) missing openDemo targets"
    exit 1
}
if ($Strict -and $unlisted.Count -gt 0) {
    Write-Host "FAIL: $($unlisted.Count) layouts not in gallery"
    exit 1
}
Write-Host "PASS gallery coverage"
exit 0
