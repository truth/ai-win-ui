param(
    [string]$VersionTag = "m124-08a5439a6b",
    [string]$Destination = "third_party/skia-m124",
    [switch]$IncludeDebug = $true,
    [switch]$IncludeRelease = $true
)

$ErrorActionPreference = "Stop"

function Download-And-Extract {
    param(
        [string]$AssetName,
        [string]$TempDir,
        [string]$DestinationDir
    )

    $downloadUrl = "https://github.com/aseprite/skia/releases/download/$VersionTag/$AssetName"
    $archivePath = Join-Path $TempDir $AssetName

    Write-Host "Downloading $AssetName"
    Invoke-WebRequest -Uri $downloadUrl -OutFile $archivePath

    Write-Host "Extracting $AssetName"
    Expand-Archive -Path $archivePath -DestinationPath $DestinationDir -Force
}

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
$destinationDir = Join-Path $repoRoot $Destination
$tempDir = Join-Path $env:TEMP ("ai-win-ui-skia-" + [Guid]::NewGuid().ToString("N"))

New-Item -ItemType Directory -Force -Path $destinationDir | Out-Null
New-Item -ItemType Directory -Force -Path $tempDir | Out-Null

try {
    if ($IncludeRelease) {
        Download-And-Extract -AssetName "Skia-Windows-Release-x64.zip" -TempDir $tempDir -DestinationDir $destinationDir
    }

    if ($IncludeDebug) {
        Download-And-Extract -AssetName "Skia-Windows-Debug-x64.zip" -TempDir $tempDir -DestinationDir $destinationDir
    }

    Write-Host ""
    Write-Host "Skia package prepared at: $destinationDir"
    Write-Host "Expected library roots:"
    Write-Host "  $destinationDir\out\Release-x64\skia.lib"
    Write-Host "  $destinationDir\out\Debug-x64\skia.lib"
}
finally {
    if (Test-Path $tempDir) {
        Remove-Item -LiteralPath $tempDir -Recurse -Force
    }
}
