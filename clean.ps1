<#
.SYNOPSIS
    Remove local build outputs and CMake caches without rebuilding.
.EXAMPLE
    .\clean.ps1 -WhatIf
.EXAMPLE
    .\clean.ps1
.EXAMPLE
    .\clean.ps1 -IncludeDist
#>
[CmdletBinding(SupportsShouldProcess = $true)]
param(
    [switch]$IncludeDist
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$RepoRoot = $PSScriptRoot
$RelativePaths = @("build", "build_test", "test_parser/build", ".tmp-build")
if ($IncludeDist) {
    $RelativePaths += "dist"
}

# Validate every target before deleting anything. Never traverse directory links.
$Targets = @()
foreach ($relativePath in $RelativePaths) {
    $target = Join-Path $RepoRoot $relativePath
    if (-not (Test-Path -LiteralPath $target)) {
        Write-Host "Skipping missing directory: $target"
        continue
    }

    $item = Get-Item -LiteralPath $target -Force
    if (-not $item.PSIsContainer) {
        throw "Refusing to clean a non-directory target: $target"
    }

    $ancestor = $item
    while ($ancestor.FullName -ne $RepoRoot) {
        if ($ancestor.Attributes -band [System.IO.FileAttributes]::ReparsePoint) {
            throw "Refusing to clean through a directory link: $($ancestor.FullName)"
        }
        $ancestor = $ancestor.Parent
    }

    $links = @(Get-ChildItem -LiteralPath $target -Recurse -Force |
        Where-Object { $_.Attributes -band [System.IO.FileAttributes]::ReparsePoint })
    if ($links.Count -gt 0) {
        throw "Refusing to clean a directory containing links: $target"
    }
    $Targets += $target
}

$RemovedCount = 0
foreach ($target in $Targets) {
    if ($PSCmdlet.ShouldProcess($target, "Remove build outputs and cache directory")) {
        Remove-Item -LiteralPath $target -Recurse -Force -ErrorAction Stop
        $RemovedCount++
        Write-Host "Removed: $target"
    }
}

Write-Host "Clean complete. Removed $RemovedCount directory(s)."
