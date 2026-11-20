param(
    [string]$Entry = "react/demos/react_showcase.tsx",
    [string]$BundleOut = "resource/react/react_demo.bundle.js",
    [ValidateSet("skia", "direct2d")]
    [string]$Renderer = "skia",
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Debug",
    [switch]$Watch,
    [switch]$SkipBuild
)

$ErrorActionPreference = "Stop"
$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
Set-Location $RepoRoot

& (Join-Path $PSScriptRoot "build_react.ps1") -Entry $Entry -OutFile $BundleOut

$exe = Join-Path $RepoRoot "build\$Configuration\react_demo.exe"
if (-not $SkipBuild -or -not (Test-Path -LiteralPath $exe)) {
    & (Join-Path $RepoRoot "build.ps1") -Configuration $Configuration
}

if (-not (Test-Path -LiteralPath $exe)) {
    throw "Missing react_demo.exe at $exe"
}

$env:AI_WIN_UI_RENDERER = $Renderer
$watchProc = $null
try {
    if ($Watch) {
        $watchProc = Start-Process -FilePath "bun" -ArgumentList @("build", $Entry, "--outfile", $BundleOut, "--target", "browser", "--format", "iife", "--watch") -PassThru -NoNewWindow
        & $exe --bundle $BundleOut --renderer $Renderer --watch
    } else {
        Start-Process -FilePath $exe -ArgumentList @("--bundle", $BundleOut, "--renderer", $Renderer)
    }
} finally {
    if ($watchProc -and -not $watchProc.HasExited) {
        Stop-Process -Id $watchProc.Id -Force -ErrorAction SilentlyContinue
    }
}

