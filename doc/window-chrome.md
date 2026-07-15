# Window Chrome

This document describes the optional **custom title bar** path for `ai-win-ui`.

## Modes

| Mode | Behavior |
|------|----------|
| `system` (default) | Standard `WS_OVERLAPPEDWINDOW` frame and title bar |
| `custom` | Borderless client area; UI draws the title bar; Win32 NC hit-testing bridges drag / resize |
| `layered` | Same as custom chrome + `WS_EX_LAYERED` + Direct2D `UpdateLayeredWindow` present (per-pixel alpha) |

`layered` is the **v2 irregular window** path: transparent clear outside rounded UI, desktop shows through corners, clicks on fully transparent pixels return `HTTRANSPARENT`.

## Enabling custom chrome

Priority (highest first):

1. Environment variable `AI_WIN_UI_CHROME=custom|layered|system`
2. Root layout attribute `chrome="custom|layered|system"`
3. Default: `system`

Examples:

```powershell
# Custom title bar
.\scripts\run_custom_chrome_demo.ps1 -BuildIfMissing

# Layered irregular (per-pixel alpha)
.\scripts\run_layered_chrome_demo.ps1 -BuildIfMissing

$env:AI_WIN_UI_CHROME = "layered"
$env:AI_WIN_UI_LAYOUT = "layouts/layered_chrome_demo.xml"
.\build\Release\ai_win_ui.exe
```

## Layout attributes

| Attribute | Where | Meaning |
|-----------|--------|---------|
| `chrome="custom\|layered\|system"` | Root element | Request window chrome mode when env is unset |
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

## Inactive chrome

When the window loses activation (`WM_ACTIVATE` / `WM_NCACTIVATE`):

- caption band is dimmed with a dark translucent overlay
- caption button glyphs reduce opacity
- outer 1px edge uses a darker stroke

`UIContext::windowActive` carries this state into caption button rendering.

## Layered present (v2)

| Piece | Role |
|-------|------|
| `PresentMode::Layered` | Direct2D DC RT + top-down BGRA DIB |
| `UpdateLayeredWindow` | Per-pixel alpha blit each frame |
| `IRenderer::SampleOpaque` | Alpha threshold for hit-testing |
| Demo | `resource/layouts/layered_chrome_demo.xml` |

Notes:

- Layered present is supported on **Direct2D and Skia**. Prefer via `AI_WIN_UI_RENDERER=skia|direct2d`.
- Clear color is fully transparent; UI must paint opaque content where the window should be solid.
- Resize borders remain active on the HWND rect even over transparent margins; caption/client hits honor alpha.
- App paints a soft multi-pass drop shadow under the first child card of the root panel.
- When maximized, floating padding and card corner radius collapse so the shell fills the work area.
- Placement centers on the **monitor under the cursor** (not always the primary).
- Skia layered uses a shared DIB backing (zero-copy ULW); minimized windows skip presents.
- Startup forces an initial layered present + foreground raise (`AttachThreadInput`) so command-line / script launches do not leave the window only on the taskbar.

```powershell
# Skia layered (default for the script)
.\scripts\run_layered_chrome_demo.ps1 -Renderer skia

# Direct2D layered
.\scripts\run_layered_chrome_demo.ps1 -Renderer direct2d
```

## Limits

- No runtime hot-switch of chrome without restart (style can be reapplied once from layout after load)
- Custom/layered modes disable the system `WS_VSCROLL` style; use client-area scrolling as before
- DWM corner preference applies to custom (HWND) mode; layered skips DWM frame extend
- Soft shadow is a multi-pass card underlay, not a general filter library
- Mesh / non-alpha hit-testing is not supported

## Related plans

- `doc/plan/2026-07-15-custom-chrome-irregular-window-v1.md`
- `doc/plan/2026-07-15-layered-irregular-window-v2.md`
- `doc/plan/2026-07-15-skia-layered-multimon-perf.md`
