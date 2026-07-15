# Window Chrome

This document describes the optional **custom title bar** and **layered /
irregular window** paths for `ai-win-ui`.

## Modes

| Mode | Behavior |
|------|----------|
| `system` (default) | Standard `WS_OVERLAPPEDWINDOW` frame and title bar |
| `custom` | Borderless client area; UI draws the title bar; Win32 NC hit-testing bridges drag / resize |
| `layered` | Same as custom chrome + `WS_EX_LAYERED` + `UpdateLayeredWindow` present (per-pixel alpha) |

`layered` is the **irregular window** path: transparent clear outside opaque UI,
desktop shows through empty alpha, clicks on fully transparent pixels return
`HTTRANSPARENT`.

Shape is **not** a separate Win32 region API in v2 — opacity of painted pixels
defines the interactive silhouette. Use a rounded `Panel` card, or a
`ShapePanel` (heart / petal / oval / star), as the opaque body.

## Enabling custom / layered chrome

Priority (highest first):

1. Environment variable `AI_WIN_UI_CHROME=custom|layered|system`
2. Root layout attribute `chrome="custom|layered|system"` (alias `irregular` → layered)
3. Default: `system`

Optional client size (useful for small shaped windows):

- `AI_WIN_UI_SIZE=420x460` or `420,480`

Examples:

```powershell
# Custom title bar
.\scripts\run_custom_chrome_demo.ps1 -BuildIfMissing

# Layered irregular (per-pixel alpha) — rounded card
.\scripts\run_layered_chrome_demo.ps1 -BuildIfMissing

# Hub that opens heart / petal / oval / star child windows
.\scripts\run_shaped_windows_demo.ps1

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

System / demo button event ids (wired in `main.cpp`):

| `onClick` id | Action |
|--------------|--------|
| `windowMinimize` | Minimize |
| `windowMaximize` | Maximize / restore |
| `windowClose` | Close |
| `openHeartWindow` | `CreateProcess` child: layered heart layout |
| `openPetalWindow` | Child: petal |
| `openOvalWindow` | Child: oval |
| `openStarWindow` | Child: star |

Caption button visuals (`Button` `variant`):

| `variant` | Glyph | Hover |
|-----------|--------|--------|
| `caption-minimize` | horizontal stroke | soft white wash |
| `caption-maximize` | square stroke | soft white wash |
| `caption-close` | X stroke | crimson `#E81123` + white icon |

Icons are drawn with renderer lines/rects (not text glyphs), so they stay crisp
on Direct2D and Skia.

Demo layouts:

- `resource/layouts/custom_chrome_demo.xml` — custom HWND chrome
- `resource/layouts/layered_chrome_demo.xml` — rounded layered card
- `resource/layouts/shaped_windows_hub.xml` — launcher for shape children
- `resource/layouts/shaped_heart_window.xml` (and petal / oval / star)

## Shaped multi-window (layout rules)

### Why a separate process?

`WM_DESTROY` posts `PostQuitMessage` for the process. Opening a second top-level
window in the **same** process would quit the hub when the shape closed.
Buttons such as `openHeartWindow` therefore spawn **the same executable** as a
child process with:

| Env set for child | Value |
|-------------------|--------|
| `AI_WIN_UI_LAYOUT` | e.g. `layouts/shaped_heart_window.xml` |
| `AI_WIN_UI_CHROME` | `layered` |
| `AI_WIN_UI_SIZE` | e.g. `420x460` |
| `AI_WIN_UI_RENDERER` | inherits hub backend when parent had none set |

Parent env vars are restored after `CreateProcess`.

### Recommended shape layout pattern (XML)

```xml
<Panel chrome="layered" direction="column" background="transparent"
       padding="12,12,12,12" alignItems="stretch">
  <ShapePanel kind="heart" fill="#E85D75" stroke="#FFB3C0" strokeWidth="2"
              flexGrow="1" hitTest="caption" text="Heart" />
  <Panel direction="row" height="40" justifyContent="center" alignItems="center">
    <Button text="Close" style="$style.dangerButton"
            onClick="windowClose" hitTest="client" width="100" />
  </Panel>
</Panel>
```

Rules:

1. Root **transparent** background so only the shape is opaque.
2. `ShapePanel` (or opaque card) provides alpha for hit-testing.
3. Interactive controls use `hitTest="client"` so they are not treated as caption.
4. Draggable body can use `hitTest="caption"` on the shape.
5. `$style.*` still works in child processes (each loads `styles/default.json`).

JSON equivalent uses the same props; close button needs `events.onClick`.

## Implementation map

| Piece | Role |
|-------|------|
| `src/window_chrome.*` | Styles, `WM_NCCALCSIZE`, `WM_NCHITTEST`, `WM_GETMINMAXINFO`, DWM |
| `src/main.cpp` | Create window, NC dispatch, layered paint, `LaunchShapedChild`, size env |
| `src/ui.h` | `HitTestRole`, `WindowChromeRequest`, `ShapePanel` |
| `src/layout_parser.*` | Parse `chrome` / `hitTest` / `role` / `ShapePanel` |
| `src/renderer.*` / `renderer_skia.*` | Layered present + `FillPolygon` |

### Hit-test order (custom / layered)

1. Resize border (DPI-aware)
2. Explicit / focusable **client** regions
3. **Caption** regions from the UI tree
4. Fallback top strip when regions are empty
5. `HTCLIENT` — layered also requires alpha above threshold via `SampleOpaque`

## Inactive chrome

When the window loses activation:

- caption band dimmed with a dark translucent overlay
- caption button glyphs reduce opacity
- outer 1px edge uses a darker stroke (HWND custom path)

`UIContext::windowActive` carries this into caption button rendering.

## Layered present

| Piece | Role |
|-------|------|
| `PresentMode::Layered` | Offscreen BGRA + `UpdateLayeredWindow` |
| `IRenderer::SampleOpaque` | Alpha threshold for hit-testing |
| `IRenderer::FillPolygon` | Heart / petal / star bodies |

Notes:

- Layered present works on **Direct2D and Skia**. Prefer via `AI_WIN_UI_RENDERER`.
- Clear color is fully transparent; UI must paint opaque content where solid.
- Resize borders remain active on the HWND rect even over transparent margins.
- Soft multi-pass drop shadow under the first child card of the root (card demo);
  pure `ShapePanel` shells may not use that underlay path.
- When maximized, floating padding and card corner radius collapse (card shell).
- Placement centers on the **monitor under the cursor**.
- Startup forces an initial layered present + foreground raise so script launches
  do not leave the window only on the taskbar.

```powershell
.\scripts\run_layered_chrome_demo.ps1 -Renderer skia
.\scripts\run_layered_chrome_demo.ps1 -Renderer direct2d
.\scripts\run_shaped_windows_demo.ps1 -Renderer skia
```

## Limits

- No runtime hot-switch of chrome without restart
- Custom/layered disable system `WS_VSCROLL`; use client-area scrolling
- DWM corner preference applies to custom (HWND) mode; layered skips DWM frame extend
- Soft shadow is a multi-pass card underlay, not a general filter library
- Hit-testing is **alpha-based**, not arbitrary mesh / region APIs
- Multi-window shapes are **multi-process**, not MDI inside one message loop

## Related docs

- `doc/layout-spec.md` — XML/JSON grammar, `ShapePanel`, `$style`
- `doc/style-catalog.md` — styles files used by shape demos
- `doc/plan/2026-07-15-custom-chrome-irregular-window-v1.md`
- `doc/plan/2026-07-15-layered-irregular-window-v2.md`
- `doc/plan/2026-07-15-skia-layered-multimon-perf.md`
