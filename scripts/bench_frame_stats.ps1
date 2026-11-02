param(
    [string]$Layout = "layouts/modern_components_v2.xml",
    [ValidateSet("skia", "direct2d")]
    [string]$Renderer = "skia",
    [ValidateSet("Debug", "Release")]
    [string]$Config = "Debug",
    [int]$ResizeSteps = 60,
    [int]$Clicks = 20,
    [string]$OutDir = ""
)

# Frame-time benchmark: launches ai_win_ui with AI_WIN_UI_FRAME_STATS enabled,
# drives idle / resize / hover / click scenarios via Win32 messages, then
# summarizes per-trigger phase timings (layout / render / present / latency).

$ErrorActionPreference = "Stop"
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$preset = if (Test-Path (Join-Path $repoRoot "build\presets\dev-debug-skia-local-sdk\$Config\ai_win_ui.exe")) { "dev-debug-skia-local-sdk" } else { "dev-debug" }
$exe = Join-Path $repoRoot "build\presets\$preset\$Config\ai_win_ui.exe"
if (-not (Test-Path $exe)) { throw "Executable not found: $exe (build with: cmake --build build\presets\$preset --config $Config)" }

if (-not $OutDir) { $OutDir = Join-Path $repoRoot "build\bench" }
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null
$tag = "$Renderer-$Config"
$csv = Join-Path $OutDir "frame_stats_$tag.csv"
$dump = Join-Path $OutDir "measure_dump_$tag.ndjson"
Remove-Item $csv, $dump -ErrorAction SilentlyContinue

# Sync resources next to the exe.
$targetRes = Join-Path (Split-Path -Parent $exe) "resource"
New-Item -ItemType Directory -Force -Path $targetRes | Out-Null
Copy-Item -Path (Join-Path $repoRoot "resource\*") -Destination $targetRes -Recurse -Force

Add-Type -Namespace Bench -Name Win32 -MemberDefinition @"
[DllImport("user32.dll")] public static extern bool SetProcessDPIAware();
[DllImport("user32.dll")] public static extern bool SetWindowPos(IntPtr h, IntPtr after, int x, int y, int cx, int cy, uint flags);
[DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
[DllImport("user32.dll")] public static extern IntPtr SendMessageW(IntPtr h, uint msg, IntPtr w, IntPtr l);
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

[void][Bench.Win32]::SetProcessDPIAware()
$env:AI_WIN_UI_RENDERER = $Renderer
$env:AI_WIN_UI_LAYOUT = $Layout
$env:AI_WIN_UI_FRAME_STATS = $csv
$env:AI_WIN_UI_MEASURE_DUMP = $dump

$proc = Start-Process -FilePath $exe -PassThru
try {
    $hwnd = [IntPtr]::Zero
    for ($i = 0; $i -lt 100 -and $hwnd -eq [IntPtr]::Zero; $i++) {
        Start-Sleep -Milliseconds 100
        $hwnd = [Bench.Win32]::FindHostWindow([uint32]$proc.Id)
    }
    if ($hwnd -eq [IntPtr]::Zero) { throw "Window did not appear" }
    Start-Sleep -Milliseconds 800

    $rect = New-Object Bench.Win32+RECT
    [void][Bench.Win32]::GetWindowRect($hwnd, [ref]$rect)
    $w0 = $rect.Right - $rect.Left; $h0 = $rect.Bottom - $rect.Top
    $SWP = 0x0002 -bor 0x0004 -bor 0x0010  # NOMOVE | NOZORDER | NOACTIVATE

    # --- Resize: oscillate width/height every ~16ms (simulates edge drag) ---
    for ($i = 0; $i -lt $ResizeSteps; $i++) {
        $dw = [int](120 * [Math]::Sin($i / 6.0))
        $dh = [int](60 * [Math]::Cos($i / 6.0))
        [void][Bench.Win32]::SetWindowPos($hwnd, [IntPtr]::Zero, 0, 0, $w0 + $dw, $h0 + $dh, $SWP)
        Start-Sleep -Milliseconds 16
    }
    [void][Bench.Win32]::SetWindowPos($hwnd, [IntPtr]::Zero, 0, 0, $w0, $h0, $SWP)
    Start-Sleep -Milliseconds 600

    # --- Locate bench_switch from the measure dump (initial size == restored size) ---
    $sw = Get-Content $dump | ForEach-Object { $_ | ConvertFrom-Json } | Where-Object { $_.path -like "*bench_switch" } | Select-Object -First 1
    if (-not $sw) { throw "bench_switch not found in $dump" }
    $cx = [int]($sw.x + 20); $cy = [int]($sw.y + $sw.h / 2)
    $lp = [IntPtr](($cy -shl 16) -bor ($cx -band 0xFFFF))

    # --- Hover: sweep the mouse across the page ---
    for ($i = 0; $i -lt 60; $i++) {
        $hx = 40 + $i * 18; $hy = 120 + ($i % 10) * 40
        [void][Bench.Win32]::PostMessageW($hwnd, 0x0200, [IntPtr]::Zero, [IntPtr](($hy -shl 16) -bor $hx))
        Start-Sleep -Milliseconds 16
    }
    Start-Sleep -Milliseconds 300

    # --- Click the switch ---
    for ($i = 0; $i -lt $Clicks; $i++) {
        [void][Bench.Win32]::PostMessageW($hwnd, 0x0200, [IntPtr]::Zero, $lp)
        [void][Bench.Win32]::PostMessageW($hwnd, 0x0201, [IntPtr]1, $lp)
        Start-Sleep -Milliseconds 60
        [void][Bench.Win32]::PostMessageW($hwnd, 0x0202, [IntPtr]::Zero, $lp)
        Start-Sleep -Milliseconds 140
    }
    Start-Sleep -Milliseconds 300
    [void][Bench.Win32]::PostMessageW($hwnd, 0x0010, [IntPtr]::Zero, [IntPtr]::Zero)  # WM_CLOSE
    $proc.WaitForExit(5000) | Out-Null
} finally {
    if (-not $proc.HasExited) { $proc.Kill() }
    Remove-Item Env:AI_WIN_UI_FRAME_STATS, Env:AI_WIN_UI_MEASURE_DUMP -ErrorAction SilentlyContinue
}

# --- Summarize ---
$rows = Import-Csv $csv
function Stat($vals) {
    $v = @($vals | ForEach-Object { [double]$_ } | Where-Object { $_ -ge 0 } | Sort-Object)
    if ($v.Count -eq 0) { return "      -" }
    $avg = ($v | Measure-Object -Average).Average
    $p95 = $v[[Math]::Min($v.Count - 1, [int]($v.Count * 0.95))]
    return ("{0,6:N2}/{1,6:N2}/{2,6:N2}" -f $avg, $p95, $v[-1])
}
Write-Host ""
Write-Host "== Frame stats: $tag | window ${w0}x${h0} | frames $($rows.Count) | CSV: $csv"
Write-Host "   (avg/p95/max ms)"
"{0,-11}{1,6} {2,22} {3,22} {4,22} {5,22} {6,22} {7,22}" -f "trigger", "n", "resize", "layout", "render", "present", "paint", "latency" | Write-Host
foreach ($g in ($rows | Group-Object trigger)) {
    $r = $g.Group
    "{0,-11}{1,6} {2,22} {3,22} {4,22} {5,22} {6,22} {7,22}" -f $g.Name, $r.Count,
        (Stat ($r | Where-Object { [double]$_.resize_ms -gt 0 } | ForEach-Object resize_ms)),
        (Stat ($r | Where-Object { [int]$_.layout_count -gt 0 } | ForEach-Object layout_ms)),
        (Stat ($r.render_ms)), (Stat ($r.present_ms)), (Stat ($r.paint_ms)), (Stat ($r.latency_ms)) | Write-Host
}
