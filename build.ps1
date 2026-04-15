param(
    [ValidateSet("Debug", "Release", "RelWithDebInfo", "MinSizeRel")]
    [string]$Configuration = "Debug",

    [string]$BuildDir = "build",

    [string]$Generator = "",

    [switch]$Clean,
    [switch]$Run
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$RepoRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$ResolvedBuildDir = Join-Path $RepoRoot $BuildDir
$TempDir = Join-Path $RepoRoot ".tmp-build"

function Invoke-Step {
    param(
        [Parameter(Mandatory = $true)]
        [string[]]$CommandParts
    )

    Write-Host ">" ($CommandParts -join " ")
    & $CommandParts[0] $CommandParts[1..($CommandParts.Length - 1)]
    if ($LASTEXITCODE -ne 0) {
        throw "Command failed with exit code $LASTEXITCODE"
    }
}

function Get-BuiltExecutable {
    param(
        [Parameter(Mandatory = $true)]
        [string]$BuildRoot,
        [Parameter(Mandatory = $true)]
        [string]$Config
    )

    $candidates = @(
        (Join-Path (Join-Path $BuildRoot $Config) "ai_win_ui.exe"),
        (Join-Path $BuildRoot "ai_win_ui.exe")
    )

    foreach ($candidate in $candidates) {
        if (Test-Path -LiteralPath $candidate) {
            return $candidate
        }
    }

    throw "Could not find built executable under '$BuildRoot'."
}

try {
    New-Item -ItemType Directory -Force -Path $TempDir | Out-Null
    $env:TEMP = $TempDir
    $env:TMP = $TempDir

    if ($Clean -and (Test-Path -LiteralPath $ResolvedBuildDir)) {
        Write-Host "Cleaning build directory: $ResolvedBuildDir"
        Remove-Item -Recurse -Force -LiteralPath $ResolvedBuildDir
    }

    New-Item -ItemType Directory -Force -Path $ResolvedBuildDir | Out-Null

    $configureArgs = @("cmake", "-S", $RepoRoot, "-B", $ResolvedBuildDir)
    if ($Generator) {
        $configureArgs += @("-G", $Generator)
    }
    Invoke-Step -CommandParts $configureArgs

    $buildArgs = @("cmake", "--build", $ResolvedBuildDir, "--config", $Configuration)
    Invoke-Step -CommandParts $buildArgs

    $exePath = Get-BuiltExecutable -BuildRoot $ResolvedBuildDir -Config $Configuration
    $resourcePath = Join-Path (Split-Path -Parent $exePath) "resource"

    Write-Host ""
    Write-Host "Build complete."
    Write-Host "Executable: $exePath"
    Write-Host "Resources : $resourcePath"

    if (-not (Test-Path -LiteralPath $resourcePath)) {
        Write-Warning "The runtime resource directory was not found next to the executable."
    }

    if ($Run) {
        Write-Host "Launching application..."
        Start-Process -FilePath $exePath -WorkingDirectory (Split-Path -Parent $exePath) | Out-Null
    }
}
finally {
    if (Test-Path -LiteralPath $TempDir) {
        Remove-Item -Recurse -Force -LiteralPath $TempDir -ErrorAction SilentlyContinue
    }
}
