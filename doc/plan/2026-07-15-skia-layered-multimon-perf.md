# Plan: Skia Layered + Multi-Monitor + Perf

## Goals

1. **Skia layered present** — same `UpdateLayeredWindow` path as Direct2D
2. **Multi-monitor** — place/maximize/DPI against the correct monitor
3. **Performance** — zero-copy present buffers, skip redundant work

## Design

### Skia

- Back Skia canvas with a top-down BGRA **DIB section** (N32 premul)
- **HWND**: `BitBlt` memDC → window DC (replaces `StretchDIBits`)
- **Layered**: `UpdateLayeredWindow` from the same memDC (zero extra copy)
- `SampleOpaque` reads DIB alpha for hit-testing

### Multi-monitor

- Initial custom/layered placement: center on **monitor under cursor** (`MonitorFromPoint` + `rcWork`)
- Maximize / `WM_NCCALCSIZE`: already uses `MonitorFromWindow` work area
- `WM_DPICHANGED`: existing path when crossing DPI boundaries
- `WM_DISPLAYCHANGE`: refresh DPI + layout after topology change

### Perf

| Optimization | Where |
|--------------|--------|
| Skip resize if size unchanged | D2D + Skia |
| Skip present while minimized | D2D + Skia |
| Dirty-frame present gate | Both |
| Shared DIB / zero-copy Skia | Skia |
| Remove layered→force-D2D | `main.cpp` |

## Exit criteria

- [x] Layered demo runs with `AI_WIN_UI_RENDERER=skia`
- [x] Layered demo still runs with Direct2D
- [x] Cursor-monitor centering for custom/layered
- [x] Build Release green
