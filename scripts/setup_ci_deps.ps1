<#
.SYNOPSIS
  Prepare third_party deps for CI / clean machines (Yoga unpack + build).

.DESCRIPTION
  Skia prebuilts are gitignored (too large for GitHub). CI builds Direct2D-only.
  Yoga sources ship as third_party/yoga-3.2.1.zip and must be expanded + built.

.EXAMPLE
  .\scripts\setup_ci_deps.ps1
  .\scripts\setup_ci_deps.ps1 -Configuration Release
#>
param(
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Debug",

    [switch]$SkipBuildYoga
)

$ErrorActionPreference = "Stop"
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
Set-Location $repoRoot

$yogaZip = Join-Path $repoRoot "third_party\yoga-3.2.1.zip"
$yogaSrc = Join-Path $repoRoot "third_party\yoga-3.2.1"
$yogaBuild = Join-Path $yogaSrc "build"

if (-not (Test-Path -LiteralPath $yogaZip)) {
    throw "Missing $yogaZip — cannot prepare Yoga for CI."
}

if (-not (Test-Path -LiteralPath (Join-Path $yogaSrc "CMakeLists.txt"))) {
    Write-Host "Expanding Yoga zip..."
    Expand-Archive -LiteralPath $yogaZip -DestinationPath (Join-Path $repoRoot "third_party") -Force
}

if (-not (Test-Path -LiteralPath (Join-Path $yogaSrc "CMakeLists.txt"))) {
    throw "Yoga sources missing after expand: $yogaSrc"
}

if ($SkipBuildYoga) {
    Write-Host "SkipBuildYoga: sources ready at $yogaSrc"
    exit 0
}

Write-Host "Configuring Yoga (shared CRT /MD — D2D CI path)..."
# Use Ninja if available is optional; VS generator matches the app.
$genArgs = @("-S", $yogaSrc, "-B", $yogaBuild)
# Let cmake pick default generator on runner (VS 2022 on windows-latest).
& cmake @genArgs
if ($LASTEXITCODE -ne 0) { throw "Yoga cmake configure failed" }

Write-Host "Building yogacore ($Configuration)..."
& cmake --build $yogaBuild --config $Configuration --target yogacore
if ($LASTEXITCODE -ne 0) { throw "Yoga build failed" }

$lib = Join-Path $yogaBuild "yoga\$Configuration\yogacore.lib"
if (-not (Test-Path -LiteralPath $lib)) {
    # Some generators nest differently
    $alt = Get-ChildItem -Path $yogaBuild -Recurse -Filter yogacore.lib -ErrorAction SilentlyContinue |
        Select-Object -First 1
    if (-not $alt) {
        throw "yogacore.lib not found under $yogaBuild"
    }
    Write-Host "Found $($alt.FullName)"
} else {
    Write-Host "OK $lib"
}

Write-Host "CI deps ready (Yoga). Skia is optional/local-only."
exit 0
