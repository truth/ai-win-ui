# Clear process-level AI_WIN_UI_* variables left by demos/tests.
# Usage: . .\scripts\clear_ai_win_env.ps1
#    or:  powershell -File .\scripts\clear_ai_win_env.ps1

$names = @(
    'AI_WIN_UI_LAYOUT',
    'AI_WIN_UI_CHROME',
    'AI_WIN_UI_RENDERER',
    'AI_WIN_UI_THEME',
    'AI_WIN_UI_STYLES',
    'AI_WIN_UI_SIZE',
    'AI_WIN_UI_QUIT_AFTER_MS',
    'AI_WIN_UI_MEASURE_DUMP',
    'AI_WIN_UI_TEXT_DUMP',
    'AI_WIN_UI_CHILD_PROCESS',
    'AI_WIN_UI_IGNORE_ENV',
    'AI_WIN_UI_DISABLE_WINDOW_SCROLL'
)

$cleared = @()
foreach ($n in $names) {
    if (Test-Path "Env:$n") {
        Remove-Item "Env:$n" -ErrorAction SilentlyContinue
        $cleared += $n
    }
}

# Also clear any other AI_WIN_UI_* leftovers
Get-ChildItem Env:AI_WIN_UI_* -ErrorAction SilentlyContinue | ForEach-Object {
    Remove-Item "Env:$($_.Name)" -ErrorAction SilentlyContinue
    if ($cleared -notcontains $_.Name) { $cleared += $_.Name }
}

if ($cleared.Count -eq 0) {
    Write-Host "No AI_WIN_UI_* process env vars were set."
} else {
    Write-Host "Cleared:"
    $cleared | ForEach-Object { Write-Host "  $_" }
}
