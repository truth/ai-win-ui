# Window Chrome

This document describes the optional **custom title bar** path for `ai-win-ui`.

## Modes

| Mode | Behavior |
|------|----------|
| `system` (default) | Standard `WS_OVERLAPPEDWINDOW` frame and title bar |
| `custom` | Borderless client area; UI draws the title bar; Win32 NC hit-testing bridges drag / resize |

Irregular windows with per-pixel alpha (`UpdateLayeredWindow`) are **out of scope** for v1. Custom mode may still use DWM rounded corners on Windows 11.

## Enabling custom chrome

Priority (highest first):

1. Environment variable `AI_WIN_UI_CHROME=custom` (or `system`)
2. Root layout attribute `chrome="custom"` / `chrome="system"`
3. Default: `system`

Examples:

```powershell
$env:AI_WIN_UI_CHROME = "custom"
$env:AI_WIN_UI_LAYOUT = "layouts/custom_chrome_demo.xml"
.\build\Release\ai_win_ui.exe
```

Or:

```powershell
.\scripts\run_custom_chrome_demo.ps1 -BuildIfMissing
```

## Layout attributes

| Attribute | Where | Meaning |
|-----------|--------|---------|
| `chrome="custom\|system"` | Root element | Request window chrome mode when env is unset |
| `hitTest="caption\|client"` | Any element | Caption = draggable; client = interactive |
| `role="titleBar"` | Panel | Convenience alias for caption region |

System button event ids (wired in `main.cpp`):

| `onClick` id | Action |
|--------------|--------|
| `windowMinimize` | Minimize |
| `windowMaximize` | Maximize / restore |
| `windowClose` | Close |

Caption button visuals (`Button` `variant`):

| `variant` | Glyph | Hover |
|-----------|--------|--------|
| `caption-minimize` | horizontal stroke | soft white wash |
| `caption-maximize` | square stroke | soft white wash |
| `caption-close` | X stroke | crimson `#E81123` + white icon |

Icons are drawn with renderer lines/rects (not text glyphs), so they stay crisp on Direct2D and Skia.

Demo layout: `resource/layouts/custom_chrome_demo.xml`.

## Implementation map

| Piece | Role |
|-------|------|
| `src/window_chrome.*` | Styles, `WM_NCCALCSIZE`, `WM_NCHITTEST`, `WM_GETMINMAXINFO`, DWM |
| `src/main.cpp` | Create window, NC message dispatch, hit-region refresh, button commands |
| `src/ui.h` | `HitTestRole`, `WindowChromeRequest`, region collection |
| `src/layout_parser.*` | Parse `chrome` / `hitTest` / `role` |

### Hit-test order (custom)

1. Resize border (DPI-aware)
2. Explicit / focusable **client** regions
3. **Caption** regions from the UI tree
4. Fallback top strip (default 40 DIP) when regions are empty
5. `HTCLIENT`

## Limits (v1)

- No layered true irregular shapes
- No runtime hot-switch of chrome without restart (style can be reapplied once from layout after load)
- Custom mode disables the system `WS_VSCROLL` style; use client-area scrolling as before
- DWM corner preference is best-effort (fails open on older OS builds)

## Related plan

- `doc/plan/2026-07-15-custom-chrome-irregular-window-v1.md`
