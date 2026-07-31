<#
.SYNOPSIS
  Build a self-contained release package for the sticky_notes sample.

.DESCRIPTION
  Builds the sticky_notes target in the requested configuration, then produces
  a minimal distributable folder containing only:
    - sticky_notes.exe
    - assets.zip   (icons/ + styles/ from resource/, zipped together)

  assets.zip is picked up automatically by the ai_win_ui host at startup
  (it checks "<exe dir>/assets.zip" before falling back to "<exe dir>/resource/"),
  so the shipped folder does not need the full loose resource/ tree.

.EXAMPLE
  .\scripts\package_sticky_notes.ps1
  .\scripts\package_sticky_notes.ps1 -Configuration Release -OutDir dist\sticky_notes
  .\scripts\package_sticky_notes.ps1 -SkipBuild   # re-zip assets only
#>
param(
    [ValidateSet("Debug", "Release", "RelWithDebInfo", "MinSizeRel")]
    [string]$Configuration = "Release",

    [string]$BuildDir = "build",

    [string]$OutDir = "dist\sticky_notes",

    [switch]$SkipBuild
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$ResolvedBuildDir = Join-Path $RepoRoot $BuildDir
$ResolvedOutDir = Join-Path $RepoRoot $OutDir

function Invoke-Step {
    param([Parameter(Mandatory = $true)][string[]]$CommandParts)
    Write-Host ">" ($CommandParts -join " ")
    & $CommandParts[0] $CommandParts[1..($CommandParts.Length - 1)]
    if ($LASTEXITCODE -ne 0) {
        throw "Command failed with exit code $LASTEXITCODE"
    }
}

# ai_win_ui's built-in ZipResourceProvider (src/zip_resource_provider.cpp) is a
# minimal reader that only understands the ZIP "Stored" (uncompressed) method.
# Neither Compress-Archive nor System.IO.Compression.ZipArchive on Windows
# PowerShell's .NET Framework actually emit Stored entries for
# -CompressionLevel NoCompression (they still write Deflate, method 8), so we
# hand-write a tiny Stored-only ZIP here to guarantee compatibility.
function Get-Crc32Table {
    $table = New-Object 'uint32[]' 256
    # NOTE: Windows PowerShell 5.1 parses hex literals above [int32]::MaxValue by
    # reinterpreting the bit pattern as a *negative* Int32 (0xEDB88320 -> a
    # negative number) instead of widening to Int64/UInt32 the way decimal
    # literals do. Use the decimal form so these stay non-negative Int64s.
    $poly = 3988292384 # 0xEDB88320
    for ($i = 0; $i -lt 256; $i++) {
        [uint32]$c = $i
        for ($k = 0; $k -lt 8; $k++) {
            if (($c -band 1) -ne 0) {
                $c = ($poly -bxor ($c -shr 1))
            } else {
                $c = $c -shr 1
            }
        }
        $table[$i] = $c
    }
    return $table
}

function Get-Crc32 {
    param([byte[]]$Bytes, [uint32[]]$Table)
    $allOnes = 4294967295 # 0xFFFFFFFF, see Get-Crc32Table note
    [uint32]$crc = $allOnes
    foreach ($b in $Bytes) {
        $idx = ($crc -bxor $b) -band 0xFF
        $crc = ($Table[$idx] -bxor ($crc -shr 8))
    }
    return ($crc -bxor $allOnes)
}

function New-StoredZip {
    param(
        [Parameter(Mandatory = $true)][string]$SourceDir,
        [Parameter(Mandatory = $true)][string]$DestinationZip
    )

    $crcTable = Get-Crc32Table
    $files = Get-ChildItem -LiteralPath $SourceDir -Recurse -File
    $out = [System.IO.File]::Open($DestinationZip, [System.IO.FileMode]::Create)
    $writer = New-Object System.IO.BinaryWriter($out)
    $central = New-Object System.Collections.Generic.List[byte[]]

    foreach ($file in $files) {
        $relPath = $file.FullName.Substring($SourceDir.Length + 1) -replace '\\', '/'
        $nameBytes = [System.Text.Encoding]::UTF8.GetBytes($relPath)
        $data = [System.IO.File]::ReadAllBytes($file.FullName)
        $crc = Get-Crc32 -Bytes $data -Table $crcTable
        $localOffset = [uint32]$out.Position

        # Local file header (Stored, no compression).
        $writer.Write([uint32]0x04034b50)
        $writer.Write([uint16]20)      # version needed
        $writer.Write([uint16]0)       # flags
        $writer.Write([uint16]0)       # method = Stored
        $writer.Write([uint16]0)       # mod time
        $writer.Write([uint16]0x21)    # mod date (nonzero to satisfy strict readers)
        $writer.Write([uint32]$crc)
        $writer.Write([uint32]$data.Length)  # compressed size
        $writer.Write([uint32]$data.Length)  # uncompressed size
        $writer.Write([uint16]$nameBytes.Length)
        $writer.Write([uint16]0)       # extra field length
        $writer.Write($nameBytes)
        $writer.Write($data)

        # Buffer the matching central directory record.
        $cd = New-Object System.IO.MemoryStream
        $cw = New-Object System.IO.BinaryWriter($cd)
        $cw.Write([uint32]0x02014b50)
        $cw.Write([uint16]20)          # version made by
        $cw.Write([uint16]20)          # version needed
        $cw.Write([uint16]0)           # flags
        $cw.Write([uint16]0)           # method
        $cw.Write([uint16]0)           # mod time
        $cw.Write([uint16]0x21)        # mod date
        $cw.Write([uint32]$crc)
        $cw.Write([uint32]$data.Length)
        $cw.Write([uint32]$data.Length)
        $cw.Write([uint16]$nameBytes.Length)
        $cw.Write([uint16]0)           # extra field length
        $cw.Write([uint16]0)           # comment length
        $cw.Write([uint16]0)           # disk number start
        $cw.Write([uint16]0)           # internal attrs
        $cw.Write([uint32]0)           # external attrs
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

    # End of central directory record.
    $writer.Write([uint32]0x06054b50)
    $writer.Write([uint16]0)           # disk number
    $writer.Write([uint16]0)           # disk with central dir
    $writer.Write([uint16]$files.Count)
    $writer.Write([uint16]$files.Count)
    $writer.Write([uint32]$centralSize)
    $writer.Write([uint32]$centralOffset)
    $writer.Write([uint16]0)           # comment length

    $writer.Flush()
    $writer.Dispose()
    $out.Dispose()
}

if (-not $SkipBuild) {
    Invoke-Step -CommandParts @(
        "cmake", "--build", $ResolvedBuildDir,
        "--config", $Configuration,
        "--target", "sticky_notes"
    )
}

$exeCandidates = @(
    (Join-Path (Join-Path $ResolvedBuildDir $Configuration) "sticky_notes.exe"),
    (Join-Path $ResolvedBuildDir "sticky_notes.exe")
)
$exePath = $exeCandidates | Where-Object { Test-Path -LiteralPath $_ } | Select-Object -First 1
if (-not $exePath) {
    throw "Could not find sticky_notes.exe under '$ResolvedBuildDir'. Build it first (drop -SkipBuild)."
}

if (Test-Path -LiteralPath $ResolvedOutDir) {
    Remove-Item -Recurse -Force -LiteralPath $ResolvedOutDir
}
New-Item -ItemType Directory -Force -Path $ResolvedOutDir | Out-Null

Copy-Item -LiteralPath $exePath -Destination $ResolvedOutDir

# Stage only what sticky_notes actually loads at runtime: icons referenced by
# the generated layout XML, plus the default style catalog (styles/default.json
# imports buttons.json + cards.json). themes/ and layouts/ belong to the main
# demo app and are intentionally left out of this package.
$stageDir = Join-Path $ResolvedOutDir "_stage"
New-Item -ItemType Directory -Force -Path (Join-Path $stageDir "icons") | Out-Null
New-Item -ItemType Directory -Force -Path (Join-Path $stageDir "styles") | Out-Null
Copy-Item -Path (Join-Path $RepoRoot "resource\icons\*") -Destination (Join-Path $stageDir "icons") -Recurse
Copy-Item -Path (Join-Path $RepoRoot "resource\styles\*") -Destination (Join-Path $stageDir "styles") -Recurse

$zipPath = Join-Path $ResolvedOutDir "assets.zip"
if (Test-Path -LiteralPath $zipPath) {
    Remove-Item -Force -LiteralPath $zipPath
}
New-StoredZip -SourceDir $stageDir -DestinationZip $zipPath
Remove-Item -Recurse -Force -LiteralPath $stageDir

Write-Host ""
Write-Host "Package complete: $ResolvedOutDir"
Get-ChildItem -LiteralPath $ResolvedOutDir | Format-Table Name, Length -AutoSize
Write-Host "Test it standalone: run the .exe from inside $OutDir with no resource\ folder present."
