param(
    [ValidateSet("skia", "direct2d", "both")]
    [string]$Renderer = "both",
    [ValidateSet("Release", "Debug")]
    [string]$Configuration = "Release",
    [string]$OutDir = ""
)

$ErrorActionPreference = "Stop"
$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
Set-Location $RepoRoot

if (-not $OutDir) {
    $OutDir = Join-Path $RepoRoot "build\bench\skia_dirty_dayk"
}
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null

$binDir = Join-Path $RepoRoot "build\$Configuration"
$stockXmlExe = Join-Path $binDir "stock_terminal.exe"
$reactDemoExe = Join-Path $binDir "react_demo.exe"

foreach ($req in @($stockXmlExe, $reactDemoExe)) {
    if (-not (Test-Path -LiteralPath $req)) {
        throw "Missing executable: $req (run .\build.ps1 -Configuration $Configuration first)"
    }
}

$backends = if ($Renderer -eq "both") { @("skia", "direct2d") } else { @($Renderer) }

Add-Type -Namespace DirtyBench -Name Win32 -MemberDefinition @"
[DllImport("user32.dll")] public static extern bool SetProcessDPIAware();
[DllImport("user32.dll")] public static extern bool PostMessageW(IntPtr h, uint msg, IntPtr w, IntPtr l);
[DllImport("user32.dll", CharSet = CharSet.Unicode)] public static extern IntPtr FindWindowExW(IntPtr parent, IntPtr childAfter, string className, string windowTitle);
[DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint processId);
public static IntPtr FindHostWindow(uint targetPid) {
    IntPtr cur = IntPtr.Zero;
    while (true) {
        cur = FindWindowExW(IntPtr.Zero, cur, "AiWinUiWindowClass", null);
        if (cur == IntPtr.Zero) return IntPtr.Zero;
        uint pid = 0;
        GetWindowThreadProcessId(cur, out pid);
        if (pid == targetPid) return cur;
    }
}
"@
[void][DirtyBench.Win32]::SetProcessDPIAware()

function Get-MetricSummary($vals) {
    $arr = @($vals | ForEach-Object { [double]$_ } | Where-Object { $_ -ge 0.0 } | Sort-Object)
    if ($arr.Count -eq 0) {
        return [ordered]@{ count = 0; avg = $null; p95 = $null; max = $null; min = $null }
    }
    $avg = ($arr | Measure-Object -Average).Average
    $p95Idx = [Math]::Min($arr.Count - 1, [int][Math]::Floor($arr.Count * 0.95))
    return [ordered]@{
        count = $arr.Count
        avg   = [Math]::Round($avg, 3)
        p95   = [Math]::Round($arr[$p95Idx], 3)
        max   = [Math]::Round($arr[-1], 3)
        min   = [Math]::Round($arr[0], 3)
    }
}

function Run-DayKDirtyScenario([string]$mode, [string]$backend) {
    $tag = "dayk_${mode}_${backend}"
    $csv = Join-Path $OutDir "${tag}.csv"
    $dump = Join-Path $OutDir "${tag}_dump.ndjson"
    Remove-Item $csv, $dump -ErrorAction SilentlyContinue

    $env:AI_WIN_UI_RENDERER = $backend
    $env:AI_WIN_UI_FRAME_STATS = $csv
    $env:AI_WIN_UI_MEASURE_DUMP = $dump
    $env:AI_WIN_UI_QUIT_AFTER_MS = $null

    $exe = if ($mode -eq "xml") { $stockXmlExe } else { $reactDemoExe }
    $argList = if ($mode -eq "xml") { @() } else { @("--bundle", "resource/react/stock_terminal.bundle.js", "--size", "1280x820", "--renderer", $backend) }

    $proc = if ($argList.Count -gt 0) { Start-Process -FilePath $exe -ArgumentList $argList -PassThru } else { Start-Process -FilePath $exe -PassThru }
    try {
        $hwnd = [IntPtr]::Zero
        for ($i = 0; $i -lt 150; $i++) {
            if ($proc.HasExited) { throw "Process exited unexpectedly with code $($proc.ExitCode)" }
            if ($hwnd -eq [IntPtr]::Zero) {
                $hwnd = [DirtyBench.Win32]::FindHostWindow([uint32]$proc.Id)
            }
            if ($hwnd -ne [IntPtr]::Zero -and (Test-Path -LiteralPath $dump)) {
                $raw = Get-Content -LiteralPath $dump -Raw -ErrorAction SilentlyContinue
                if ($raw -and $raw.Contains("btn_day_k")) {
                    Start-Sleep -Milliseconds 200
                    break
                }
            }
            Start-Sleep -Milliseconds 100
        }
        if ($hwnd -eq [IntPtr]::Zero) { throw "Timed out waiting for host window" }

        $items = Get-Content -LiteralPath $dump | ForEach-Object { $_ | ConvertFrom-Json }
        $dayBtn = $items | Where-Object { $_.path -match "btn_day_k$" } | Select-Object -First 1
        if (-not $dayBtn) { throw "Missing btn_day_k in $dump" }

        # Click Day-K button to switch stock_chart to static candlestick view
        $cx = [int][Math]::Round($dayBtn.x + $dayBtn.w * 0.5)
        $cy = [int][Math]::Round($dayBtn.y + $dayBtn.h * 0.5)
        $lp = [IntPtr](($cy -shl 16) -bor ($cx -band 0xFFFF))
        [void][DirtyBench.Win32]::PostMessageW($hwnd, 0x0200, [IntPtr]::Zero, $lp)
        [void][DirtyBench.Win32]::PostMessageW($hwnd, 0x0201, [IntPtr]1, $lp)
        Start-Sleep -Milliseconds 50
        [void][DirtyBench.Win32]::PostMessageW($hwnd, 0x0202, [IntPtr]::Zero, $lp)
        Start-Sleep -Milliseconds 800

        $r0 = [Math]::Max(0, (Get-Content -LiteralPath $csv).Count - 1)
        Start-Sleep -Milliseconds 6000
        $r1 = [Math]::Max(0, (Get-Content -LiteralPath $csv).Count - 1)

        [void][DirtyBench.Win32]::PostMessageW($hwnd, 0x0010, [IntPtr]::Zero, [IntPtr]::Zero)
        $proc.WaitForExit(5000) | Out-Null

        $allRows = @(Import-Csv $csv)
        $slice = if ($r1 -gt $r0 -and $allRows.Count -ge $r1) { @($allRows[$r0..($r1 - 1)]) } else { @() }
        $tickTrig = if ($mode -eq "xml") { @("invalidate") } else { @("react_timer") }
        $tickRows = @($slice | Where-Object { $_.trigger -in $tickTrig })
        if ($tickRows.Count -eq 0) { $tickRows = $slice }

        return [ordered]@{
            mode         = $mode
            renderer     = $backend
            frames       = $tickRows.Count
            dirtyRects   = Get-MetricSummary ($tickRows | ForEach-Object dirty_rects)
            dirtyAreaPct = Get-MetricSummary ($tickRows | ForEach-Object dirty_area_pct)
            renderMs     = Get-MetricSummary ($tickRows | ForEach-Object render_ms)
            presentMs    = Get-MetricSummary ($tickRows | ForEach-Object present_ms)
            paintMs      = Get-MetricSummary ($tickRows | ForEach-Object paint_ms)
        }
    } finally {
        if (-not $proc.HasExited) { $proc.Kill() }
        Remove-Item Env:AI_WIN_UI_FRAME_STATS, Env:AI_WIN_UI_MEASURE_DUMP, Env:AI_WIN_UI_RENDERER -ErrorAction SilentlyContinue
    }
}

$runs = New-Object System.Collections.Generic.List[object]
foreach ($b in $backends) {
    foreach ($m in @("xml", "react")) {
        Write-Host "== Day-K 500ms Disjoint-Corner Test: mode=$m renderer=$b ($Configuration) ==" -ForegroundColor Cyan
        $runs.Add((Run-DayKDirtyScenario $m $b)) | Out-Null
    }
}

$outObj = [ordered]@{
    generatedAt   = (Get-Date).ToString("o")
    configuration = $Configuration
    runs          = $runs
}
$jsonPath = Join-Path $OutDir "dayk_summary.json"
$outObj | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $jsonPath -Encoding UTF8

Write-Host "`n================ DAY-K DISJOINT-CORNER BENCHMARK ($Configuration) ================" -ForegroundColor Green
Write-Host "Saved JSON: $jsonPath"
foreach ($r in $runs) {
    Write-Host ("[{0} | {1}] frames={2} dirtyRects={3} dirtyArea={4}% | render={5}/{6}ms present={7}ms paint={8}/{9}ms" -f `
        $r.mode, $r.renderer, $r.frames, $r.dirtyRects.avg, $r.dirtyAreaPct.avg, `
        $r.renderMs.avg, $r.renderMs.p95, $r.presentMs.avg, $r.paintMs.avg, $r.paintMs.p95) -ForegroundColor Yellow
}

