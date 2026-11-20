param(
    [string]$Entry = "react/demos/react_showcase.tsx",
    [string]$OutFile = "resource/react/react_demo.bundle.js",
    [switch]$Minify,
    [switch]$Watch
)

$ErrorActionPreference = "Stop"
$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
Set-Location $RepoRoot

$outDir = Split-Path -Parent $OutFile
if ($outDir) {
    New-Item -ItemType Directory -Force -Path $outDir | Out-Null
}

$argsList = @(
    "build",
    $Entry,
    "--outfile", $OutFile,
    "--target", "browser",
    "--format", "iife"
)
if ($Minify) {
    $argsList += "--minify"
}
if ($Watch) {
    $argsList += "--watch"
}

Write-Host "> bun $($argsList -join ' ')"
& bun @argsList
if ($LASTEXITCODE -ne 0) {
    throw "bun build failed with exit code $LASTEXITCODE"
}

if (-not $Watch -and (Test-Path -LiteralPath $OutFile)) {
    $size = (Get-Item -LiteralPath $OutFile).Length
    $kb = [Math]::Round($size / 1024.0, 2)
    Write-Host "Built React bundle: $OutFile ($size bytes / $kb KB)"
}

