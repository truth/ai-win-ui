# Skia Integration Notes

## Why This Exists

The project now has a stable Yoga-based layout path and a clean `IRenderer`
abstraction. That makes it possible to grow a second backend without rewiring
the UI layer.

This document records the current Skia path, the build and runtime workflow,
and the main remaining gaps.

## Current Decision

As of 2026-04-18, the repository is using `aseprite/skia` prebuilt Windows
packages as the near-term Skia integration path.

Why:

- official Skia does not provide a ready-made Windows C++ release package
- the local `vcpkg` path has not been reliable enough for this sprint
- `aseprite/skia` publishes Windows prebuilt packages that can be unpacked and
  consumed as a local SDK directory

Supporting investigation:

- `doc/skia-prebuilt-survey.md`

## Current Status

The codebase is now ready for incremental Skia integration in these ways:

- `IRenderer` remains the only rendering interface consumed by the UI layer
- renderer selection is explicit through `RendererBackend`
- app startup can request a backend through `AI_WIN_UI_RENDERER`
- `renderer_skia.cpp` exists and builds through the local SDK path
- the current Skia implementation is CPU/raster based and presents pixels to
  the window through Win32
- the app has been launched successfully with `AI_WIN_UI_RENDERER=skia`
- wrapped text rendering is better than the original single-line placeholder
- focus-driven scrolling now helps the shared validation pages stay usable
- if Skia is unavailable or initialization fails, the app can fall back to Direct2D

## Build Preparation

`CMakeLists.txt` exposes these Skia-facing knobs:

- `AI_WIN_UI_ENABLE_SKIA`
- `AI_WIN_UI_SKIA_PROVIDER`
- `AI_WIN_UI_SKIA_DIR`

Supported integration routes in the repo:

- `local-sdk`
- `vcpkg`

The chosen short-term path is `local-sdk`, targeting the `aseprite/skia`
package layout.

### Expected Local SDK Layout

The repository expects a shape compatible with:

- `<SKIA_DIR>/include/...`
- `<SKIA_DIR>/out/Debug-x64/skia.lib`
- `<SKIA_DIR>/out/Release-x64/skia.lib`

If needed, the defaults can be overridden with:

- `AI_WIN_UI_SKIA_INCLUDE_DIR`
- `AI_WIN_UI_SKIA_LIBRARY_DIR`

### Useful Presets

`CMakePresets.json` includes:

- `dev-debug`
- `dev-debug-skia-local-sdk`
- `dev-debug-skia-vcpkg`
- `dev-debug-skia-vs-vcpkg`

Example:

```powershell
cmake --preset dev-debug-skia-local-sdk
cmake --build --preset build-dev-debug-skia-local-sdk
```

To fetch the selected prebuilt package into the repository default location:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\setup_skia_prebuilt.ps1
```

## Runtime Examples

### Run the Default Skia Demo

```powershell
$env:AI_WIN_UI_RENDERER = "skia"
Remove-Item Env:AI_WIN_UI_LAYOUT -ErrorAction SilentlyContinue
Start-Process -FilePath ".\build\presets\dev-debug-skia-local-sdk\Debug\ai_win_ui.exe"
```

### Run the Yoga Measurement Page

```powershell
$env:AI_WIN_UI_RENDERER = "skia"
$env:AI_WIN_UI_LAYOUT = "layouts/yoga_measure_cases.xml"
Start-Process -FilePath ".\build\presets\dev-debug-skia-local-sdk\Debug\ai_win_ui.exe"
```

### Run the Image Regression Page

```powershell
$env:AI_WIN_UI_RENDERER = "skia"
$env:AI_WIN_UI_LAYOUT = "layouts/skia_image_cases.xml"
Start-Process -FilePath ".\build\presets\dev-debug-skia-local-sdk\Debug\ai_win_ui.exe"
```

### Run the Core Validation Page

```powershell
$env:AI_WIN_UI_RENDERER = "skia"
$env:AI_WIN_UI_LAYOUT = "layouts/core_validation.xml"
Start-Process -FilePath ".\build\presets\dev-debug-skia-local-sdk\Debug\ai_win_ui.exe"
```

This is currently the best manual validation page for:

- wrapped text
- button and text input text rendering
- Yoga flex behavior under pressure
- keyboard focus and bring-into-view scrolling

### Or Use the Shared Launcher

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\run_layout_demo.ps1 `
  -Renderer skia `
  -Layout layouts/core_validation.xml
```

## Current Implementation Snapshot

The current `renderer_skia.cpp` is intentionally small but real.

Implemented today:

- raster surface creation via `SkSurfaces::WrapPixels`
- frame clear
- filled and stroked rectangles
- filled and stroked rounded rectangles
- rounded clipping via save/clip/restore
- encoded image decode via `SkImages::DeferredFromEncodedData`
- image draw into destination rect with explicit linear sampling
- wrapped UTF-8 text draw using `SkFont`, subpixel/LCD-oriented font settings,
  manual line breaking, clipping, and baseline calculation

## Current Limits

- text draw is improved, but it still does not yet match DirectWrite behavior closely enough
- presentation is CPU/raster only
- bitmap scaling and text fidelity still need deeper side-by-side comparison
  against the Direct2D backend
- Skia text measurement is not yet the same engine as the current layout
  measurement path
- the `vcpkg` Skia path still is not the preferred delivery route in this repo

## Recommended Next Tasks

1. Continue improving Skia text so it is closer to the existing text measurement path.
2. Compare image presentation, clipping, DPI behavior, and text layout against Direct2D.
3. Use `resource/layouts/core_validation.xml` as the shared manual acceptance page while growing parity.
4. Continue tuning image scaling quality and compare the current explicit linear sampling policy against Direct2D.
5. Only after raster parity is acceptable, revisit either `vcpkg` or a self-build workflow.
