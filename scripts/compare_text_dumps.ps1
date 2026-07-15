# Compare Label text metrics between Skia and Direct2D (Wave1 R1).
param(
    [string]$Layout = "core-validation",
    [float]$TolerancePx = 2.0,
    [switch]$BuildIfMissing,
    [switch]$HeightsOnly  # R2: only compare wrapH/nowrapH
)

$ErrorActionPreference = "Stop"
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$dump = Join-Path $PSScriptRoot "run_text_dump.ps1"
$skiaOut = Join-Path $env:TEMP ("text_skia_{0}.ndjson" -f [guid]::NewGuid().ToString("N"))
$d2dOut = Join-Path $env:TEMP ("text_d2d_{0}.ndjson" -f [guid]::NewGuid().ToString("N"))

& $dump -Layout $Layout -Renderer skia -OutFile $skiaOut -BuildIfMissing:$BuildIfMissing
if ($LASTEXITCODE -ne 0) { throw "skia dump failed" }
& $dump -Layout $Layout -Renderer direct2d -OutFile $d2dOut -BuildIfMissing:$BuildIfMissing
if ($LASTEXITCODE -ne 0) { throw "d2d dump failed" }

function Read-Map([string]$path) {
    $map = @{}
    Get-Content -LiteralPath $path | ForEach-Object {
        $o = $_ | ConvertFrom-Json
        $map[$o.path] = $o
    }
    return $map
}

$skia = Read-Map $skiaOut
$d2d = Read-Map $d2dOut
$fails = @()
$checked = 0
foreach ($key in ($skia.Keys | Sort-Object)) {
    if (-not $d2d.ContainsKey($key)) {
        $fails += "missing in d2d: $key"
        continue
    }
    $a = $skia[$key]
    $b = $d2d[$key]
    $fields = if ($HeightsOnly) { @("wrapH", "nowrapH") } else { @("wrapH", "nowrapH", "wrapW", "nowrapW") }
    foreach ($f in $fields) {
        $pa = $a.PSObject.Properties[$f]
        $pb = $b.PSObject.Properties[$f]
        if (-not $pa -or -not $pb) {
            $fails += "missing field $f on $key"
            continue
        }
        # Use $a.$f (not redirect `>`): PowerShell `>` is file redirection, which
        # previously created junk files like "2" / "1.5" and always "passed".
        $da = [double]($a.$f)
        $db = [double]($b.$f)
        $diff = [math]::Abs($da - $db)
        $checked++
        if ($diff -gt $TolerancePx) {
            $fails += ("{0}.{1}: skia={2:N2} d2d={3:N2} diff={4:N2}" -f $key, $f, $da, $db, $diff)
        }
    }
}

Write-Host "Compared $($skia.Count) labels, $checked metrics, tol=${TolerancePx}px"
if ($fails.Count -eq 0) {
    Write-Host "PASS: Skia vs Direct2D text metrics within tolerance"
    Remove-Item $skiaOut, $d2dOut -ErrorAction SilentlyContinue
    exit 0
}

Write-Host "FAIL: $($fails.Count) diffs > ${TolerancePx}px"
$fails | Select-Object -First 40 | ForEach-Object { Write-Host "  $_" }
# Keep dumps for investigation
Write-Host "Skia dump: $skiaOut"
Write-Host "D2D dump:  $d2dOut"
exit 1
