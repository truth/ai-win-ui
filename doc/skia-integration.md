# Skia Integration Notes

## Why This Exists

The project now has a stable Yoga-based layout path and a clean `IRenderer`
abstraction. That makes it a good time to prepare the rendering layer for a
second backend without destabilizing the current Direct2D runtime.

This document records the current state, the integration seam that is already
in place, and the next tasks required to keep growing the current
`SkiaRenderer`.

## Current Decision

As of 2026-04-17, the repository is choosing `aseprite/skia` prebuilt Windows
packages as the near-term Skia integration path.

Why:

- official Skia does not provide a ready-made Windows C++ release package
- the local `vcpkg` path has not been reliable enough for this sprint
- `aseprite/skia` publishes Windows prebuilt packages that are intended to be
  unpacked and consumed as a local Skia directory

The supporting investigation is recorded in:

- `doc/skia-prebuilt-survey.md`

## Current Status

The codebase is now ready for incremental Skia integration in these ways:

- `IRenderer` remains the only rendering interface consumed by the UI layer.
- Renderer selection is now explicit through `RendererBackend`.
- App startup can request a backend through `AI_WIN_UI_RENDERER`.
- A first-pass `renderer_skia.cpp` now exists.
- The current Skia implementation is CPU/raster based and presents pixels to the
  window through Win32.
- The `aseprite/skia` local-SDK path has been built successfully in this repo.
- The app has been launched successfully with `AI_WIN_UI_RENDERER=skia`.
- If Skia is not compiled into the build, requesting `skia` still falls back to
  Direct2D.
- Window title text shows the active backend and whether a fallback happened.

This means we can begin landing Skia work behind the renderer seam without
rewiring the UI tree, Yoga layout flow, event dispatch, or resource parsing.

## Runtime Behavior

The app understands the environment variable below:

```powershell
$env:AI_WIN_UI_RENDERER = "direct2d"
```

or:

```powershell
$env:AI_WIN_UI_RENDERER = "skia"
```

Today there are two runtime outcomes:

- if the build does not include Skia, the renderer factory falls back to
  Direct2D and the title reflects that fallback
- if the build includes Skia through a supported package path, the app can instantiate the
  Skia renderer directly

There is also a safety fallback during startup: if the Skia renderer can be
created but fails `Initialize(HWND)`, the app now retries Direct2D before
giving up. That keeps backend experiments from turning into app-start failures.

One verified runtime example on this repository is:

- window title: `AI WinUI Renderer [Skia] - layouts/yoga_measure_cases.xml`

That confirms the app can launch through the Skia path directly rather than
silently falling back to Direct2D.

## Build Preparation

`CMakeLists.txt` now exposes two knobs for the future Skia work:

- `AI_WIN_UI_ENABLE_SKIA`
- `AI_WIN_UI_SKIA_PROVIDER`
- `AI_WIN_UI_SKIA_DIR`

The repository currently supports two conceptual ways to surface Skia into the
build:

- `local-sdk`
- `vcpkg`

The chosen near-term path is `local-sdk`, specifically targeting the
`aseprite/skia` package layout.

### Chosen Path: `aseprite/skia` as Local SDK

Current intent:

- unpack an `aseprite/skia` Windows x64 package into a local directory
- point `AI_WIN_UI_SKIA_DIR` at that directory
- teach the CMake `local-sdk` branch how to consume the package layout
- keep `vcpkg` as a later follow-up instead of the active delivery path

The `aseprite/skia` releases are useful here because they are published as
prebuilt archives intended to be unpacked and reused like a local Skia build
output.

The local SDK branch now expects a package layout compatible with this shape:

- `<SKIA_DIR>/include/...`
- `<SKIA_DIR>/out/Debug-x64/skia.lib`
- `<SKIA_DIR>/out/Release-x64/skia.lib`

If needed, the defaults can be overridden with:

- `AI_WIN_UI_SKIA_INCLUDE_DIR`
- `AI_WIN_UI_SKIA_LIBRARY_DIR`

### vcpkg Manifest

The repo now includes a `vcpkg.json` feature:

- feature name: `skia`
- package name: `skia`
- CMake package expected from vcpkg: `unofficial-skia`
- CMake target expected from vcpkg: `unofficial::skia::skia`

The enabled feature set is intentionally narrow and Windows-oriented:

- `jpeg`
- `png`
- `webp`

This remains in the repository as an alternative dependency path, but it is not
the selected short-term integration route anymore.

The manifest now includes a `builtin-baseline`, but this path is intentionally
de-prioritized until the local SDK integration is complete.

### Configure Examples

To keep the current build path unchanged:

```powershell
cmake -S . -B build
```

To enable the Skia scaffolding with a local SDK layout:

```powershell
cmake -S . -B build `
  -DAI_WIN_UI_ENABLE_SKIA=ON `
  -DAI_WIN_UI_SKIA_PROVIDER=local-sdk `
  -DAI_WIN_UI_SKIA_DIR=F:/deps/skia-m124
```

For the chosen `aseprite/skia` route, the expected next step is to point this
directory at the unpacked Windows package root.

If the package uses a different include root or library directory, use:

```powershell
cmake -S . -B build `
  -DAI_WIN_UI_ENABLE_SKIA=ON `
  -DAI_WIN_UI_SKIA_PROVIDER=local-sdk `
  -DAI_WIN_UI_SKIA_DIR=F:/deps/skia-m124 `
  -DAI_WIN_UI_SKIA_INCLUDE_DIR=F:/deps/skia-m124 `
  -DAI_WIN_UI_SKIA_LIBRARY_DIR=F:/deps/skia-m124/out
```

To enable the vcpkg package path:

```powershell
cmake -S . -B build `
  -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake `
  -DVCPKG_MANIFEST_FEATURES=skia `
  -DAI_WIN_UI_ENABLE_SKIA=ON `
  -DAI_WIN_UI_SKIA_PROVIDER=vcpkg
```

When `AI_WIN_UI_SKIA_PROVIDER=vcpkg`, CMake now does:

```cmake
find_package(unofficial-skia CONFIG REQUIRED)
target_link_libraries(ai_win_ui PRIVATE unofficial::skia::skia)
```

That means the repo still retains a `vcpkg` option, but the recommended next
work should happen on the `local-sdk` side first.

### CMake Presets

The repo now also includes `CMakePresets.json` with useful starting points:

- `dev-debug`
- `dev-debug-skia-local-sdk`
- `dev-debug-skia-vcpkg`
- `dev-debug-skia-vs-vcpkg`

Use them like this:

```powershell
cmake --preset dev-debug
cmake --build --preset build-dev-debug
```

and, once `VCPKG_ROOT` is configured:

```powershell
cmake --preset dev-debug-skia-vcpkg
cmake --build --preset build-dev-debug-skia-vcpkg
```

For the chosen prebuilt package path:

```powershell
cmake --preset dev-debug-skia-local-sdk
cmake --build --preset build-dev-debug-skia-local-sdk
```

To fetch the selected `aseprite/skia` package into the repository's default
location, use:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\setup_skia_prebuilt.ps1
```

## Quick Start Examples

### Example 1: Build the Default Direct2D Path

```powershell
cmake --preset dev-debug
cmake --build --preset build-dev-debug
```

### Example 2: Build the Skia Local-SDK Path

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\setup_skia_prebuilt.ps1
cmake --preset dev-debug-skia-local-sdk
cmake --build --preset build-dev-debug-skia-local-sdk
```

### Example 3: Run the App with the Default Gallery Layout on Skia

The default `resource/layouts/ui.xml` and `resource/layouts/ui.json` already
include a small image gallery with multiple `Image` controls. This is the
easiest way to validate Skia bitmap decode, clipping, rounded corners, and
Yoga-driven layout in one pass.

```powershell
$env:AI_WIN_UI_RENDERER = "skia"
Remove-Item Env:AI_WIN_UI_LAYOUT -ErrorAction SilentlyContinue
Start-Process -FilePath ".\build\presets\dev-debug-skia-local-sdk\Debug\ai_win_ui.exe"
```

What to look for:

- the window title should say `AI WinUI Renderer [Skia]`
- the image strip in the middle of the page should load from
  `resource/images/`
- rounded image corners should be clipped cleanly
- the bottom action row should still be positioned by Yoga correctly

### Example 4: Run the Yoga Measurement Sample on Skia

```powershell
$env:AI_WIN_UI_RENDERER = "skia"
$env:AI_WIN_UI_LAYOUT = "layouts/yoga_measure_cases.xml"
Start-Process -FilePath ".\build\presets\dev-debug-skia-local-sdk\Debug\ai_win_ui.exe"
```

This sample is useful when checking layout behavior first and image behavior
second, because it puts row/column sizing and text measurement under more
pressure than the default gallery layout.

### Example 5: Compare Direct2D and Skia Quickly

```powershell
$env:AI_WIN_UI_RENDERER = "direct2d"
Start-Process -FilePath ".\build\Debug\ai_win_ui.exe"

$env:AI_WIN_UI_RENDERER = "skia"
Start-Process -FilePath ".\build\presets\dev-debug-skia-local-sdk\Debug\ai_win_ui.exe"
```

This makes it easier to compare:

- text sharpness
- rounded clipping
- bitmap scaling and aspect ratio
- overall spacing under the same Yoga layout

## Layout and Image Examples

### Minimal XML Image Example

```xml
<Panel padding="24" spacing="12">
  <Label text="Skia image sample" />
  <Image source="images/img1.jpg" cornerRadius="14" />
</Panel>
```

This exercises:

- `LayoutParser` loading image bytes through `IResourceProvider`
- `Image` creating a backend `BitmapHandle`
- `SkiaRenderer::CreateBitmapFromBytes`
- `SkiaRenderer::DrawBitmap`

### Minimal JSON Image Example

```json
{
  "type": "Panel",
  "props": { "padding": 24, "spacing": 12 },
  "children": [
    { "type": "Label", "props": { "text": "Skia image sample" } },
    {
      "type": "Image",
      "props": {
        "source": "images/img1.jpg",
        "cornerRadius": 14
      }
    }
  ]
}
```

### Mixed Yoga + Image Gallery Example

The current default layout already demonstrates a more realistic mixed case:

- a vertical page shell
- a row-oriented toolbar
- a multi-image content strip
- rounded image clipping
- bottom actions held in place by Yoga flex behavior

Representative XML fragment:

```xml
<Panel spacing="16">
  <Panel direction="row" spacing="12">
    <Button text="Preview" />
    <Button text="Publish" />
    <Spacer flexGrow="1" flexBasis="0" />
    <Label text="3 tasks left" />
  </Panel>

  <Panel direction="row" spacing="12">
    <Image source="images/img1.jpg" cornerRadius="10" />
    <Image source="images/3d-design_9390515.png" cornerRadius="10" />
    <Image source="images/ai_10439488.png" cornerRadius="10" />
    <Image source="images/airplane_10521325.png" cornerRadius="10" />
  </Panel>
</Panel>
```

This is a good first manual verification target because it exercises both the
Yoga integration and the Skia bitmap path without needing any new test assets.

## Recommended Integration Sequence

### Step 1

Keep Yoga work stable.

- Do not mix major Yoga behavior changes with the first Skia renderer landing.
- Use the existing `resource/layouts/yoga_measure_cases.xml` and
  `resource/layouts/yoga_measure_cases.json` samples as a layout baseline.

### Step 2

Grow the dedicated `renderer_skia.cpp` in small slices.

- Keep the current Direct2D renderer untouched as a fallback and comparison
  backend.
- Avoid changing the UI-facing renderer interface while closing feature gaps.

### Step 3

Choose the first Skia surface strategy.

Two realistic options:

- CPU raster first:
  - easiest proof of concept
  - simplest dependency shape
  - slower, but good for correctness work
- D3D11-backed Skia surface:
  - better long-term path on Windows
  - more integration work up front
  - better aligned with a production renderer

For this project, the steady path is:

1. land CPU raster first if we need fast validation
2. move to D3D11-backed Skia after API parity is stable

### Step 4

Port renderer features in small slices.

Recommended order:

1. `BeginFrame` / `EndFrame`
2. solid rect and rounded rect drawing
3. clipping
4. bitmap upload and draw
5. text draw

This order gives visible progress while keeping regressions easier to isolate.

## API Mapping Checklist

The current `IRenderer` surface already gives a concrete checklist for the Skia
backend:

- `FillRect`
- `FillRoundedRect`
- `DrawRect`
- `DrawRoundedRect`
- `PushRoundedClip`
- `PopLayer`
- `DrawTextW`
- `CreateBitmapFromBytes`
- `GetBitmapSize`
- `DrawBitmap`

The remaining design question is not the UI API. It is the Windows surface and
text strategy used inside the Skia implementation.

## Text and Bitmap Notes

Skia integration should respect the abstractions already in place:

- UI code should keep using `BitmapHandle`, not Skia image objects directly.
- UI code should keep using `ITextMeasurer`, not Skia text APIs directly.
- If Skia eventually owns text measurement too, that should happen behind
  `ITextMeasurer`, not by leaking Skia types into `ui.h`.

This preserves the architecture work that already removed Direct2D,
DirectWrite, and WRL details from the public UI layer.

## Current Implementation Snapshot

The current `renderer_skia.cpp` is intentionally small but real.

Implemented behind `AI_WIN_UI_HAS_VCPKG_SKIA` or `AI_WIN_UI_HAS_LOCAL_SKIA`:

- raster surface creation via `SkSurfaces::WrapPixels`
- frame clear
- filled and stroked rectangles
- filled and stroked rounded rectangles
- rounded clipping via save/clip/restore
- encoded image decode via `SkImages::DeferredFromEncodedData`
- image draw into destination rect
- first-pass UTF-8 text draw using `SkFont` and `drawSimpleText`

The current image path specifically looks like this:

1. `LayoutParser` loads `resource/images/...` bytes through the active
   `IResourceProvider`
2. `Image` stores those bytes and lazily requests a `BitmapHandle`
3. the renderer decodes the encoded payload into a backend bitmap resource
4. `Image::Render` calls `DrawBitmap`
5. Skia draws the image into the destination rect, optionally inside a rounded
   clip

Current limits:

- text draw is single-line and does not yet match DirectWrite wrapping behavior
- presentation is CPU/raster only
- bitmap scaling and text fidelity still need a deeper side-by-side comparison
  against the Direct2D backend
- the current `vcpkg` manifest intentionally avoids `direct3d`, `freetype`,
  `harfbuzz`, and `icu` for the first bring-up, so advanced text behavior is
  not the target yet
- the `vcpkg` Skia path has not been verified in this repository yet because it
  still depends on local `VCPKG_ROOT` setup

## Next Concrete Tasks

1. Improve wrapped text rendering so it better matches the existing UI layout
   expectations.
2. Compare image presentation, clipping, and DPI behavior against Direct2D
   using the default gallery layout.
3. Decide whether image scaling should stay with the current simple
   `drawImageRect` path or move to a more explicit sampling policy.
4. Add one or two dedicated Skia-focused sample layouts if image-heavy scenarios
   need clearer manual regression targets.
5. Only after raster parity is acceptable, revisit either `vcpkg` or a
   self-build workflow.
