<#
.SYNOPSIS
    Package a React JSX ai-win-ui application in external ZIP mode, single-file EXE embedded mode, or both.

.DESCRIPTION
    Supports two standalone distribution modes with zero external resource/ folder:
      - zip : Produces <OutDir>\zip\<ExeName> + Stored assets.zip
      - exe : Embeds Stored assets.zip into <OutDir>\exe\<ExeName> via Win32 RT_RCDATA (AI_WIN_UI_ASSETS_ZIP)
      - both: Produces both zip and exe outputs

.EXAMPLE
    .\scripts\package_react_app.ps1 -App stock -Mode both -Configuration Release
#>
[CmdletBinding()]
param(
    [ValidateSet("showcase", "stock")]
    [string]$App = "stock",

    [ValidateSet("zip", "exe", "both")]
    [string]$Mode = "both",

    [ValidateSet("Release", "Debug")]
    [string]$Configuration = "Release",

    [string]$BuildDir = "build",

    [string]$OutDir = "",

    [switch]$SkipBunBuild
)

$ErrorActionPreference = "Stop"

$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
if ([string]::IsNullOrWhiteSpace($OutDir)) {
    $OutDir = Join-Path $RepoRoot ("dist\react_" + $App)
} elseif (-not [System.IO.Path]::IsPathRooted($OutDir)) {
    $OutDir = Join-Path $RepoRoot $OutDir
}

$ResolvedBuildDir = if ([System.IO.Path]::IsPathRooted($BuildDir)) { $BuildDir } else { Join-Path $RepoRoot $BuildDir }

function Invoke-Step {
    param([string[]]$CommandParts)
    Write-Host ">> $($CommandParts -join ' ')" -ForegroundColor Cyan
    & $CommandParts[0] $CommandParts[1..($CommandParts.Length - 1)]
    if ($LASTEXITCODE -ne 0) {
        throw "Command failed with exit code ${LASTEXITCODE}: $($CommandParts -join ' ')"
    }
}

function Get-Crc32 {
    param([byte[]]$Bytes)
    $crc = [uint32]0xFFFFFFFFL
    $poly = [uint32]0xEDB88320L
    foreach ($b in $Bytes) {
        $crc = $crc -bxor [uint32]$b
        for ($i = 0; $i -lt 8; $i++) {
            if (($crc -band 1) -ne 0) {
                $crc = (($crc -shr 1) -bxor $poly)
            } else {
                $crc = ($crc -shr 1)
            }
        }
    }
    return ($crc -bxor [uint32]0xFFFFFFFFL)
}

function New-StoredZip {
    param(
        [string]$SourceDir,
        [string]$DestinationZip
    )
    $rootFull = (Resolve-Path $SourceDir).Path.TrimEnd('\', '/')
    $files = Get-ChildItem -LiteralPath $rootFull -Recurse -File | Sort-Object FullName

    $out = [System.IO.File]::Open($DestinationZip, [System.IO.FileMode]::Create)
    $writer = New-Object System.IO.BinaryWriter($out)
    $central = New-Object System.Collections.Generic.List[byte[]]

    foreach ($file in $files) {
        $rel = $file.FullName.Substring($rootFull.Length + 1).Replace('\', '/')
        $nameBytes = [System.Text.Encoding]::UTF8.GetBytes($rel)
        $data = [System.IO.File]::ReadAllBytes($file.FullName)
        $crc = Get-Crc32 -Bytes $data
        $size = [uint32]$data.Length
        $localOffset = [uint32]$out.Position

        # Local file header (Stored, method 0)
        $writer.Write([uint32]0x04034b50)
        $writer.Write([uint16]20)
        $writer.Write([uint16]0)
        $writer.Write([uint16]0)
        $writer.Write([uint16]0)
        $writer.Write([uint16]0)
        $writer.Write([uint32]$crc)
        $writer.Write([uint32]$size)
        $writer.Write([uint32]$size)
        $writer.Write([uint16]$nameBytes.Length)
        $writer.Write([uint16]0)
        $writer.Write($nameBytes)
        $writer.Write($data)

        $cd = New-Object System.IO.MemoryStream
        $cw = New-Object System.IO.BinaryWriter($cd)
        $cw.Write([uint32]0x02014b50)
        $cw.Write([uint16]20)
        $cw.Write([uint16]20)
        $cw.Write([uint16]0)
        $cw.Write([uint16]0)
        $cw.Write([uint16]0)
        $cw.Write([uint16]0)
        $cw.Write([uint32]$crc)
        $cw.Write([uint32]$size)
        $cw.Write([uint32]$size)
        $cw.Write([uint16]$nameBytes.Length)
        $cw.Write([uint16]0)
        $cw.Write([uint16]0)
        $cw.Write([uint16]0)
        $cw.Write([uint16]0)
        $cw.Write([uint32]0)
        $cw.Write([uint32]$localOffset)
        $cw.Write($nameBytes)
        $cw.Flush()
        $central.Add($cd.ToArray())
        $cw.Dispose()
        $cd.Dispose()
    }

    $centralOffset = [uint32]$out.Position
    foreach ($record in $central) {
        $writer.Write($record)
    }
    $centralSize = [uint32]($out.Position - $centralOffset)

    $writer.Write([uint32]0x06054b50)
    $writer.Write([uint16]0)
    $writer.Write([uint16]0)
    $writer.Write([uint16]$files.Count)
    $writer.Write([uint16]$files.Count)
    $writer.Write([uint32]$centralSize)
    $writer.Write([uint32]$centralOffset)
    $writer.Write([uint16]0)

    $writer.Flush()
    $writer.Dispose()
    $out.Dispose()
}

if (-not $SkipBunBuild) {
    $buildReactScript = Join-Path $PSScriptRoot "build_react.ps1"
    if (Test-Path $buildReactScript) {
        & $buildReactScript
        if ($LASTEXITCODE -ne 0) {
            throw "build_react.ps1 failed with exit code $LASTEXITCODE"
        }
    }
}

$bundleSource = if ($App -eq "stock") {
    Join-Path $RepoRoot "resource\react\stock_terminal.bundle.js"
} else {
    Join-Path $RepoRoot "resource\react\react_demo.bundle.js"
}
if (-not (Test-Path -LiteralPath $bundleSource)) {
    throw "Missing React bundle: $bundleSource"
}

$exeFileName = if ($App -eq "stock") { "stock_terminal_react.exe" } else { "react_demo.exe" }

New-Item -ItemType Directory -Force -Path $OutDir | Out-Null

# Stage Stored assets.zip (maps chosen app bundle to react/react_demo.bundle.js so zero-arg launch works)
$stageDir = Join-Path $OutDir "_stage"
if (Test-Path -LiteralPath $stageDir) {
    Remove-Item -Recurse -Force -LiteralPath $stageDir
}
New-Item -ItemType Directory -Force -Path (Join-Path $stageDir "react") | Out-Null
Copy-Item -LiteralPath $bundleSource -Destination (Join-Path $stageDir "react\react_demo.bundle.js")

foreach ($sub in @("icons", "styles", "themes", "images")) {
    $srcSub = Join-Path $RepoRoot ("resource\" + $sub)
    if (Test-Path -LiteralPath $srcSub) {
        Copy-Item -Path $srcSub -Destination $stageDir -Recurse
    }
}

$masterZip = Join-Path $OutDir "assets.zip"
if (Test-Path -LiteralPath $masterZip) {
    Remove-Item -Force -LiteralPath $masterZip
}
New-StoredZip -SourceDir $stageDir -DestinationZip $masterZip
Remove-Item -Recurse -Force -LiteralPath $stageDir

if ($Mode -eq "zip" -or $Mode -eq "both") {
    if (-not (Test-Path (Join-Path $ResolvedBuildDir "CMakeCache.txt"))) {
        Invoke-Step -CommandParts @("cmake", "-S", $RepoRoot, "-B", $ResolvedBuildDir)
    }
    Invoke-Step -CommandParts @(
        "cmake", "--build", $ResolvedBuildDir,
        "--config", $Configuration,
        "--target", "react_demo"
    )
    $zipBuildExe = Join-Path (Join-Path $ResolvedBuildDir $Configuration) "react_demo.exe"
    if (-not (Test-Path -LiteralPath $zipBuildExe)) {
        throw "Built react_demo.exe not found at $zipBuildExe"
    }
    $zipOutDir = Join-Path $OutDir "zip"
    New-Item -ItemType Directory -Force -Path $zipOutDir | Out-Null
    Copy-Item -LiteralPath $zipBuildExe -Destination (Join-Path $zipOutDir $exeFileName) -Force
    Copy-Item -LiteralPath $masterZip -Destination (Join-Path $zipOutDir "assets.zip") -Force
    Write-Host "[OK] ZIP package -> $zipOutDir" -ForegroundColor Green
}

if ($Mode -eq "exe" -or $Mode -eq "both") {
    $embedBuildDir = Join-Path $ResolvedBuildDir ("embed_react_" + $App)
    Invoke-Step -CommandParts @(
        "cmake", "-S", $RepoRoot, "-B", $embedBuildDir,
        "-DAI_WIN_UI_EMBED_ASSETS_ZIP=$masterZip"
    )
    Invoke-Step -CommandParts @(
        "cmake", "--build", $embedBuildDir,
        "--config", $Configuration,
        "--target", "react_demo"
    )
    $embedExe = Join-Path (Join-Path $embedBuildDir $Configuration) "react_demo.exe"
    if (-not (Test-Path -LiteralPath $embedExe)) {
        throw "Embedded react_demo.exe not found at $embedExe"
    }
    $exeOutDir = Join-Path $OutDir "exe"
    New-Item -ItemType Directory -Force -Path $exeOutDir | Out-Null
    Copy-Item -LiteralPath $embedExe -Destination (Join-Path $exeOutDir $exeFileName) -Force
    Write-Host "[OK] Single-file EXE package -> $exeOutDir" -ForegroundColor Green
}

Remove-Item -Force -LiteralPath $masterZip -ErrorAction SilentlyContinue
Write-Host "Packaging complete under: $OutDir" -ForegroundColor Green

