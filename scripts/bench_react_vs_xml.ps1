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
    $OutDir = Join-Path $RepoRoot "build\bench\react_vs_xml"
}
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null

$binDir = Join-Path $RepoRoot "build\$Configuration"
$stockXmlExe = Join-Path $binDir "stock_terminal.exe"
$stressXmlExe = Join-Path $binDir "perf_stress_xml.exe"
$reactDemoExe = Join-Path $binDir "react_demo.exe"

foreach ($req in @($stockXmlExe, $stressXmlExe, $reactDemoExe)) {
    if (-not (Test-Path -LiteralPath $req)) {
        throw "Missing executable: $req (run .\build.ps1 -Configuration $Configuration first)"
    }
}

$backends = if ($Renderer -eq "both") { @("skia", "direct2d") } else { @($Renderer) }

Add-Type -Namespace PerfBench -Name Win32 -MemberDefinition @"
[DllImport("user32.dll")] public static extern bool SetProcessDPIAware();
[DllImport("user32.dll")] public static extern bool SetWindowPos(IntPtr h, IntPtr after, int x, int y, int cx, int cy, uint flags);
[DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
[DllImport("user32.dll")] public static extern bool PostMessageW(IntPtr h, uint msg, IntPtr w, IntPtr l);
[DllImport("user32.dll", CharSet = CharSet.Unicode)] public static extern IntPtr FindWindowExW(IntPtr parent, IntPtr childAfter, string className, string windowTitle);
[DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint processId);
public struct RECT { public int Left, Top, Right, Bottom; }
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
[void][PerfBench.Win32]::SetProcessDPIAware()

function Get-StatsSummary($vals) {
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

function Read-CsvSlice($csvPath, [int]$fromRowExclusive, [int]$toRowInclusive) {
    $rows = Import-Csv $csvPath
    if ($rows.Count -eq 0 -or $toRowInclusive -le $fromRowExclusive) {
        return @()
    }
    $startIdx = [Math]::Max(0, $fromRowExclusive)
    $endIdx = [Math]::Min($rows.Count - 1, $toRowInclusive - 1)
    if ($endIdx -lt $startIdx) { return @() }
    return @($rows[$startIdx..$endIdx])
}

function Get-CsvRowCount($csvPath) {
    if (-not (Test-Path -LiteralPath $csvPath)) { return 0 }
    return [Math]::Max(0, (Get-Content -LiteralPath $csvPath).Count - 1)
}

function Wait-HostAndDump($proc, $dumpPath, $requiredMarker) {
    $hwnd = [IntPtr]::Zero
    for ($i = 0; $i -lt 150; $i++) {
        if ($proc.HasExited) { throw "Process exited unexpectedly with code $($proc.ExitCode)" }
        if ($hwnd -eq [IntPtr]::Zero) {
            $hwnd = [PerfBench.Win32]::FindHostWindow([uint32]$proc.Id)
        }
        if ($hwnd -ne [IntPtr]::Zero -and (Test-Path -LiteralPath $dumpPath)) {
            $raw = Get-Content -LiteralPath $dumpPath -Raw -ErrorAction SilentlyContinue
            if ($raw -and $raw.Contains($requiredMarker)) {
                Start-Sleep -Milliseconds 120
                return $hwnd
            }
        }
        Start-Sleep -Milliseconds 100
    }
    throw "Timed out waiting for host window or measure marker '$requiredMarker' in $dumpPath"
}

function Find-ElementFromDump($dumpPath, $leafPattern) {
    $items = Get-Content -LiteralPath $dumpPath | ForEach-Object { $_ | ConvertFrom-Json }
    return ($items | Where-Object { $_.path -match $leafPattern } | Select-Object -First 1)
}

function Send-Click([IntPtr]$hwnd, $elem) {
    $cx = [int][Math]::Round($elem.x + $elem.w * 0.5)
    $cy = [int][Math]::Round($elem.y + $elem.h * 0.5)
    $lp = [IntPtr](($cy -shl 16) -bor ($cx -band 0xFFFF))
    [void][PerfBench.Win32]::PostMessageW($hwnd, 0x0200, [IntPtr]::Zero, $lp) # WM_MOUSEMOVE
    [void][PerfBench.Win32]::PostMessageW($hwnd, 0x0201, [IntPtr]1, $lp)     # WM_LBUTTONDOWN
    Start-Sleep -Milliseconds 45
    [void][PerfBench.Win32]::PostMessageW($hwnd, 0x0202, [IntPtr]::Zero, $lp) # WM_LBUTTONUP
}

function Summarize-Slice($slice, $triggerFilter = $null) {
    $rows = if ($triggerFilter) { @($slice | Where-Object { $_.trigger -in $triggerFilter }) } else { @($slice) }
    $layoutRows = @($rows | Where-Object { [int]$_.layout_count -gt 0 })
    return [ordered]@{
        frames       = $rows.Count
        layoutFrames = $layoutRows.Count
        layoutTotal  = ($rows | ForEach-Object { [int]$_.layout_count } | Measure-Object -Sum).Sum
        resizeMs     = Get-StatsSummary ($rows | Where-Object { [double]$_.resize_ms -gt 0 } | ForEach-Object resize_ms)
        layoutMs     = Get-StatsSummary ($layoutRows | ForEach-Object layout_ms)
        jsMs         = Get-StatsSummary ($rows | ForEach-Object react_js_ms)
        applyMs      = Get-StatsSummary ($rows | ForEach-Object react_apply_ms)
        renderMs     = Get-StatsSummary ($rows | ForEach-Object render_ms)
        presentMs    = Get-StatsSummary ($rows | ForEach-Object present_ms)
        paintMs      = Get-StatsSummary ($rows | ForEach-Object paint_ms)
        latencyMs    = Get-StatsSummary ($rows | ForEach-Object latency_ms)
        reactE2eMs   = Get-StatsSummary ($rows | ForEach-Object react_e2e_ms)
    }
}

function Run-StockScenario([string]$mode, [string]$renderer) {
    $tag = "stock_${mode}_${renderer}"
    $csv = Join-Path $OutDir "${tag}.csv"
    $dump = Join-Path $OutDir "${tag}_dump.ndjson"
    Remove-Item $csv, $dump -ErrorAction SilentlyContinue

    $env:AI_WIN_UI_RENDERER = $renderer
    $env:AI_WIN_UI_FRAME_STATS = $csv
    $env:AI_WIN_UI_MEASURE_DUMP = $dump
    $env:AI_WIN_UI_QUIT_AFTER_MS = $null

    $exe = if ($mode -eq "xml") { $stockXmlExe } else { $reactDemoExe }
    $argList = if ($mode -eq "xml") { @() } else { @("--bundle", "resource/react/stock_terminal.bundle.js", "--size", "1280x820", "--renderer", $renderer) }

    $proc = if ($argList.Count -gt 0) { Start-Process -FilePath $exe -ArgumentList $argList -PassThru } else { Start-Process -FilePath $exe -PassThru }
    try {
        $hwnd = Wait-HostAndDump $proc $dump "btn_day_k"
        Start-Sleep -Milliseconds 1200

        $proc.Refresh()
        $memWsMb = [Math]::Round($proc.WorkingSet64 / 1MB, 2)
        $memPrivMb = [Math]::Round($proc.PrivateMemorySize64 / 1MB, 2)

        $rect = New-Object PerfBench.Win32+RECT
        [void][PerfBench.Win32]::GetWindowRect($hwnd, [ref]$rect)
        $w0 = $rect.Right - $rect.Left
        $h0 = $rect.Bottom - $rect.Top

        $dayBtn = Find-ElementFromDump $dump "btn_day_k$"
        $tsBtn  = Find-ElementFromDump $dump "btn_timeshare$"
        $chart  = Find-ElementFromDump $dump "stock_chart$"
        if (-not $dayBtn -or -not $tsBtn -or -not $chart) {
            throw "Missing stock benchmark element in $dump (day=$dayBtn, ts=$tsBtn, chart=$chart)"
        }

        # 1. Idle 500ms live tick window (8s)
        $r0 = Get-CsvRowCount $csv
        Start-Sleep -Milliseconds 8000
        $r1 = Get-CsvRowCount $csv

        # 2. 60Hz Crosshair mousemove sweep across chart
        $r2 = $r1
        for ($i = 0; $i -lt 60; $i++) {
            $hx = [int]($chart.x + 60 + (($chart.w - 120) * ($i / 59.0)))
            $hy = [int]($chart.y + $chart.h * 0.4)
            [void][PerfBench.Win32]::PostMessageW($hwnd, 0x0200, [IntPtr]::Zero, [IntPtr](($hy -shl 16) -bor ($hx -band 0xFFFF)))
            Start-Sleep -Milliseconds 16
        }
        Start-Sleep -Milliseconds 250
        $r3 = Get-CsvRowCount $csv

        # 3. Period switch clicks (Day K <-> TimeShare, 10 alternating clicks)
        $r4 = $r3
        for ($i = 0; $i -lt 10; $i++) {
            $btn = if ($i % 2 -eq 0) { $dayBtn } else { $tsBtn }
            Send-Click $hwnd $btn
            Start-Sleep -Milliseconds 180
        }
        Start-Sleep -Milliseconds 300
        $r5 = Get-CsvRowCount $csv

        # 4. Theme toggle hotkey 'T' (6 presses, button already holds focus instead of TextInput)
        $r6 = $r5
        for ($i = 0; $i -lt 6; $i++) {
            [void][PerfBench.Win32]::PostMessageW($hwnd, 0x0100, [IntPtr]0x54, [IntPtr]::Zero) # WM_KEYDOWN 'T'
            Start-Sleep -Milliseconds 240
        }
        Start-Sleep -Milliseconds 300
        $r7 = Get-CsvRowCount $csv

        # 5. Window resize sweep (40 steps @ 16ms)
        $r8 = $r7
        $SWP = 0x0002 -bor 0x0004 -bor 0x0010
        for ($i = 0; $i -lt 40; $i++) {
            $dw = [int](100 * [Math]::Sin($i / 5.0))
            $dh = [int](50 * [Math]::Cos($i / 5.0))
            [void][PerfBench.Win32]::SetWindowPos($hwnd, [IntPtr]::Zero, 0, 0, $w0 + $dw, $h0 + $dh, $SWP)
            Start-Sleep -Milliseconds 16
        }
        [void][PerfBench.Win32]::SetWindowPos($hwnd, [IntPtr]::Zero, 0, 0, $w0, $h0, $SWP)
        Start-Sleep -Milliseconds 400
        $r9 = Get-CsvRowCount $csv

        [void][PerfBench.Win32]::PostMessageW($hwnd, 0x0010, [IntPtr]::Zero, [IntPtr]::Zero)
        $proc.WaitForExit(5000) | Out-Null

        $allRows = @(Import-Csv $csv)
        $startupRow = $allRows | Where-Object { [double]$_.startup_ms -ge 0.0 } | Select-Object -First 1
        $tickTrig = if ($mode -eq "xml") { @("invalidate") } else { @("react_timer") }
        $clickTrig = if ($mode -eq "xml") { @("mouseup") } else { @("react_click") }
        $keyTrig = if ($mode -eq "xml") { @("key") } else { @("key", "react_key") }

        return [ordered]@{
            suite        = "stock_terminal"
            mode         = $mode
            renderer     = $renderer
            windowSize   = "${w0}x${h0}"
            nodes        = (Get-Content -LiteralPath $dump).Count
            workingSetMb = $memWsMb
            privateMb    = $memPrivMb
            startupMs    = if ($startupRow) { [Math]::Round([double]$startupRow.startup_ms, 3) } else { $null }
            tick500ms    = Summarize-Slice (Read-CsvSlice $csv $r0 $r1) $tickTrig
            hover60Hz    = Summarize-Slice (Read-CsvSlice $csv $r2 $r3) @("mousemove")
            periodClick  = Summarize-Slice (Read-CsvSlice $csv $r4 $r5) $clickTrig
            hotkeyTheme  = Summarize-Slice (Read-CsvSlice $csv $r6 $r7) $keyTrig
            resizeSweep  = Summarize-Slice (Read-CsvSlice $csv $r8 $r9) @("resize")
        }
    } finally {
        if (-not $proc.HasExited) { $proc.Kill() }
        Remove-Item Env:AI_WIN_UI_FRAME_STATS, Env:AI_WIN_UI_MEASURE_DUMP, Env:AI_WIN_UI_RENDERER -ErrorAction SilentlyContinue
    }
}

function Run-StressScenario([string]$mode, [string]$renderer) {
    $tag = "stress_${mode}_${renderer}"
    $csv = Join-Path $OutDir "${tag}.csv"
    $dump = Join-Path $OutDir "${tag}_dump.ndjson"
    Remove-Item $csv, $dump -ErrorAction SilentlyContinue

    $env:AI_WIN_UI_RENDERER = $renderer
    $env:AI_WIN_UI_FRAME_STATS = $csv
    $env:AI_WIN_UI_MEASURE_DUMP = $dump
    $env:AI_WIN_UI_QUIT_AFTER_MS = $null

    $exe = if ($mode -eq "xml") { $stressXmlExe } else { $reactDemoExe }
    $argList = if ($mode -eq "xml") { @() } else { @("--bundle", "resource/react/bench_stress.bundle.js", "--size", "1000x700", "--renderer", $renderer) }

    $proc = if ($argList.Count -gt 0) { Start-Process -FilePath $exe -ArgumentList $argList -PassThru } else { Start-Process -FilePath $exe -PassThru }
    try {
        $hwnd = Wait-HostAndDump $proc $dump "stress_input"
        Start-Sleep -Milliseconds 800

        $proc.Refresh()
        $memWsMb = [Math]::Round($proc.WorkingSet64 / 1MB, 2)
        $memPrivMb = [Math]::Round($proc.PrivateMemorySize64 / 1MB, 2)

        $inputBox = Find-ElementFromDump $dump "stress_input$"
        if (-not $inputBox) { throw "Missing stress_input in $dump" }

        # 1. Sustained 60Hz 100-cell update window (4s)
        $r0 = Get-CsvRowCount $csv
        Start-Sleep -Milliseconds 4000
        $r1 = Get-CsvRowCount $csv

        # 2. Focus TextInput & rapid type 40 characters @ 25ms
        Send-Click $hwnd $inputBox
        Start-Sleep -Milliseconds 150
        $r2 = Get-CsvRowCount $csv
        for ($i = 0; $i -lt 40; $i++) {
            $ch = 65 + ($i % 26)
            [void][PerfBench.Win32]::PostMessageW($hwnd, 0x0102, [IntPtr]$ch, [IntPtr]::Zero) # WM_CHAR
            Start-Sleep -Milliseconds 25
        }
        Start-Sleep -Milliseconds 250
        $r3 = Get-CsvRowCount $csv

        [void][PerfBench.Win32]::PostMessageW($hwnd, 0x0010, [IntPtr]::Zero, [IntPtr]::Zero)
        $proc.WaitForExit(5000) | Out-Null

        $allRows = @(Import-Csv $csv)
        $startupRow = $allRows | Where-Object { [double]$_.startup_ms -ge 0.0 } | Select-Object -First 1

        $tickTrig = if ($mode -eq "xml") { @("invalidate") } else { @("react_timer") }
        $stressSlice = Read-CsvSlice $csv $r0 $r1
        $tickRows = @($stressSlice | Where-Object { $_.trigger -in $tickTrig })
        $elapsedMs = if ($tickRows.Count -ge 2) { [double]$tickRows[-1].t_ms - [double]$tickRows[0].t_ms } else { 4000.0 }
        $achievedFps = if ($elapsedMs -gt 1.0 -and $tickRows.Count -ge 2) {
            [Math]::Round((($tickRows.Count - 1) * 1000.0) / $elapsedMs, 2)
        } else { 0.0 }

        $stressSummary = Summarize-Slice $stressSlice $tickTrig
        $stressSummary["achievedFps"] = $achievedFps
        $stressSummary["windowMs"] = [Math]::Round($elapsedMs, 1)

        $typingSlice = Read-CsvSlice $csv $r2 $r3
        $typingReactRows = @($typingSlice | Where-Object { [double]$_.react_e2e_ms -ge 0.0 })

        return [ordered]@{
            suite        = "stress_100cells"
            mode         = $mode
            renderer     = $renderer
            nodes        = (Get-Content -LiteralPath $dump).Count
            workingSetMb = $memWsMb
            privateMb    = $memPrivMb
            startupMs    = if ($startupRow) { [Math]::Round([double]$startupRow.startup_ms, 3) } else { $null }
            batch60Hz    = $stressSummary
            typingChar   = Summarize-Slice $typingSlice @("char")
            typingReact  = Summarize-Slice $typingReactRows $null
        }
    } finally {
        if (-not $proc.HasExited) { $proc.Kill() }
        Remove-Item Env:AI_WIN_UI_FRAME_STATS, Env:AI_WIN_UI_MEASURE_DUMP, Env:AI_WIN_UI_RENDERER -ErrorAction SilentlyContinue
    }
}

$results = New-Object System.Collections.Generic.List[object]

foreach ($renderer in $backends) {
    foreach ($mode in @("xml", "react")) {
        Write-Host "== Running Stock Terminal benchmark: mode=$mode renderer=$renderer ($Configuration) ==" -ForegroundColor Cyan
        $results.Add((Run-StockScenario $mode $renderer)) | Out-Null
    }
}

foreach ($renderer in $backends) {
    foreach ($mode in @("xml", "react")) {
        Write-Host "== Running 60Hz 100-Cell Stress benchmark: mode=$mode renderer=$renderer ($Configuration) ==" -ForegroundColor Cyan
        $results.Add((Run-StressScenario $mode $renderer)) | Out-Null
    }
}

$summaryObj = [ordered]@{
    generatedAt   = (Get-Date).ToString("o")
    configuration = $Configuration
    runs          = $results
}

$summaryJsonPath = Join-Path $OutDir "summary.json"
$summaryObj | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $summaryJsonPath -Encoding UTF8

Write-Host ""
Write-Host "================ BENCHMARK SUMMARY ($Configuration) ================" -ForegroundColor Green
Write-Host "Saved JSON: $summaryJsonPath"

foreach ($r in $results) {
    if ($r.suite -eq "stock_terminal") {
        Write-Host ("`n[STOCK] {0} | {1} | startup={2}ms | WS={3}MB | Priv={4}MB" -f $r.mode, $r.renderer, $r.startupMs, $r.workingSetMb, $r.privateMb) -ForegroundColor Yellow
        Write-Host ("  Tick 500ms  (n={0}, layout={1}): js={2}ms apply={3}ms render={4}ms paint={5}ms" -f $r.tick500ms.frames, $r.tick500ms.layoutTotal, $r.tick500ms.jsMs.avg, $r.tick500ms.applyMs.avg, $r.tick500ms.renderMs.avg, $r.tick500ms.paintMs.avg)
        Write-Host ("  Hover 60Hz  (n={0}): render={1}/{2}ms paint={3}/{4}ms lat={5}/{6}ms" -f $r.hover60Hz.frames, $r.hover60Hz.renderMs.avg, $r.hover60Hz.renderMs.p95, $r.hover60Hz.paintMs.avg, $r.hover60Hz.paintMs.p95, $r.hover60Hz.latencyMs.avg, $r.hover60Hz.latencyMs.p95)
        $clickE2e = if ($r.mode -eq "xml") { $r.periodClick.latencyMs } else { $r.periodClick.reactE2eMs }
        $keyE2e   = if ($r.mode -eq "xml") { $r.hotkeyTheme.latencyMs } else { $r.hotkeyTheme.reactE2eMs }
        Write-Host ("  PeriodClick (n={0}): js={1}ms apply={2}ms paint={3}ms e2e(avg/p95/max)={4}/{5}/{6}ms" -f $r.periodClick.frames, $r.periodClick.jsMs.avg, $r.periodClick.applyMs.avg, $r.periodClick.paintMs.avg, $clickE2e.avg, $clickE2e.p95, $clickE2e.max)
        Write-Host ("  Hotkey 'T'  (n={0}): js={1}ms apply={2}ms paint={3}ms e2e(avg/p95/max)={4}/{5}/{6}ms" -f $r.hotkeyTheme.frames, $r.hotkeyTheme.jsMs.avg, $r.hotkeyTheme.applyMs.avg, $r.hotkeyTheme.paintMs.avg, $keyE2e.avg, $keyE2e.p95, $keyE2e.max)
        Write-Host ("  Resize 40x  (n={0}): resize={1}ms layout={2}ms render={3}ms lat={4}/{5}ms" -f $r.resizeSweep.frames, $r.resizeSweep.resizeMs.avg, $r.resizeSweep.layoutMs.avg, $r.resizeSweep.renderMs.avg, $r.resizeSweep.latencyMs.avg, $r.resizeSweep.latencyMs.p95)
    } else {
        Write-Host ("`n[STRESS] {0} | {1} | startup={2}ms | WS={3}MB | Priv={4}MB" -f $r.mode, $r.renderer, $r.startupMs, $r.workingSetMb, $r.privateMb) -ForegroundColor Yellow
        Write-Host ("  60Hz 100-Cell (n={0}, fps={1}, layout={2}): js={3}/{4}ms apply={5}ms render={6}/{7}ms paint={8}/{9}/{10}ms" -f $r.batch60Hz.frames, $r.batch60Hz.achievedFps, $r.batch60Hz.layoutTotal, $r.batch60Hz.jsMs.avg, $r.batch60Hz.jsMs.p95, $r.batch60Hz.applyMs.avg, $r.batch60Hz.renderMs.avg, $r.batch60Hz.renderMs.p95, $r.batch60Hz.paintMs.avg, $r.batch60Hz.paintMs.p95, $r.batch60Hz.paintMs.max)
        Write-Host ("  Typing 'char' (n={0}): uiLatency(avg/p95)={1}/{2}ms | react_input E2E(n={3})={4}/{5}ms" -f $r.typingChar.frames, $r.typingChar.latencyMs.avg, $r.typingChar.latencyMs.p95, $r.typingReact.frames, $r.typingReact.reactE2eMs.avg, $r.typingReact.reactE2eMs.p95)
    }
}

