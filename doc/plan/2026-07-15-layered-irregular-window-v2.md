# Plan: Layered Irregular Window v2

## Goal

Enable **per-pixel alpha** windows via `WS_EX_LAYERED` + `UpdateLayeredWindow`, so the UI can show true rounded / floating shapes with transparent margins (desktop shows through).

## Scope (v2)

| In | Out |
|----|-----|
| `AI_WIN_UI_CHROME=layered` opt-in | Full Skia layered present (v2.1) |
| Direct2D offscreen WIC RT → ULW | Arbitrary mesh hit-testing beyond alpha sample |
| Transparent clear + rounded content | Runtime hot-switch layered ↔ hwnd mid-frame |
| Alpha-aware `WM_NCHITTEST` (threshold) | Multi-monitor color management polish |
| Demo layout + script | Soft drop-shadow filter library |

## Architecture

```
App clear(0,0,0,0) + UI tree (opaque rounded card)
        │
        ▼
Direct2DRenderer (PresentMode::Layered)
  WIC 32bppPBGRA bitmap + ID2D1RenderTarget
        │ EndFrame
        ▼
DIB section / memory DC → UpdateLayeredWindow(ULW_ALPHA)
        │
        ▼
WindowChrome::Layered (WS_POPUP | WS_EX_LAYERED, same caption hit-test as custom)
```

## Mode matrix

| Mode | Frame | Present |
|------|-------|---------|
| system | OS caption | HWND RT |
| custom | UI caption | HWND RT |
| layered | UI caption | UpdateLayeredWindow |

## Exit criteria

- [ ] Layered demo shows rounded window; desktop visible in corners
- [ ] Drag / min / max / close still work
- [ ] Clicks on fully transparent pixels pass through (`HTTRANSPARENT`)
- [ ] System/custom modes unchanged
