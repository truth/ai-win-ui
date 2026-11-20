# Changelog

All notable changes to **ai-win-ui** are documented in this file.

Format based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).
Versioning follows [SemVer](https://semver.org/) (`AI_WIN_UI_VERSION_*` in `include/ai_win_ui/version.h`).

## [Unreleased]

### Added

- **React JSX Native UI (`feature/react-jsx-ui`)**
  - Vendored `QuickJS-ng v0.18.0` C worker thread + `ai_win_ui_react` dual-thread bridge with typed integer `MutationBatch` protocol (`nodeId` / `NodeTypeId` / `PropId`)
  - Minimal TypeScript React reconciler & hooks (`jsx`/`Fragment`, hooks, keyed diff, 19 native primitives including `<FinancialChart>` + `useWindowKeyDown`)
  - Optimistic input echo guard (`eventSeq`/`ackSeq`), visual-only batch fast path (`EndBatchUpdate(false)`), React Showcase & Flush-style Stock Terminal apps, `react_unit_test`, and `scripts/bench_react_vs_xml.ps1`
- **Skia Multi-Dirty-Rect & Text Diagnostics (`feature/skia-textblob-dirty-region`)**
  - `ai_win_ui::DirtyRegion` (`src/dirty_region.h`, max 8 rects, adjacent coalescing, 85% full-repaint threshold) & `IRenderer::SetDirtyRects` / `GetDirtyRects`
  - `dirty_rects,dirty_area_pct` columns in `FrameProfiler` CSV output, `samples/skia_unit_test/` (4 tests), and `scripts/bench_skia_dirty_region.ps1`

### Performance

- **Skia Text Pipeline & Partial Repaint (`feature/skia-textblob-dirty-region`)**
  - UTF-16 direct batch width measurement (`SkFont::textToGlyphs(..., kUTF16)` + `getWidths`), zero-copy 4096-entry LRU `AcquireSkiaTextLayout`, and 4096-entry `SkTextBlob` LRU cache (`allocRunPosH`) + single-line `DrawTextW` fast path + `FontMetrics`/`SkDashPathEffect` caches
  - Win32 `GetUpdateRgn`/`GetRegionData` extraction prior to `BeginPaint`, `SkRegion` clipping + per-rect `BitBlt` presentation on Skia (Direct2D strictly preserved as full-frame), widget `IntersectsDirtyRect` culling, caret-only blink invalidation (`InvalidateSelf(4.0f)`), F12 corner HUD clipping, and batched updates in `samples/stock_terminal/main.cpp`
  - Release gains vs `12fa270`: full-frame `resizeSweep` render `-27.6%` (XML) / `-21.3%` (React), Stock XML `tick500ms` paint `-70.3%` (`4.932 -> 1.464 ms`), `typingChar` paint `-83.5%~-83.9%`, and Day-K disjoint-corner XML Skia `paint 0.336 ms` (`3.84%` dirty area, **20.8×** faster than Direct2D `6.976 ms`)

## [0.3.2] — 2026-09-26

### Added

- **File drag & drop**
  - OLE `IDropTarget` on the host window (`DragEnter`/`DragOver`/`DragLeave`/`Drop`)
  - `UIElement` hooks `OnDragEnter/OnDragOver/OnDragLeave/OnDrop` + `FindDropTargetAt`
  - `allowDrop="true"` XML attribute; `Panel` default drop appends dropped file names
  - `resource/layouts/drag_drop_cases.xml` demo
- **Cursor control**
  - `CursorType` enum + `UIElement::SetCursor/GetCursor`, `cursor="..."` XML attribute
  - `WM_SETCURSOR` maps element cursor to system cursors (hand/wait/ibeam/resize/…)
  - `TextInput`/`RichTextBox` default to I-beam; `DataTable` shows resize cursor on column edges
- `UIContext::requestLayout` / `invalidate` callbacks so elements can request relayout/repaint

### Fixed

- Drag & drop never registered: the UI thread was initialized as MTA by the renderer,
  so `OleInitialize` failed with `RPC_E_CHANGED_MODE` and `RegisterDragDrop` was skipped.
  The host now establishes an STA **before** renderer COM init.
- `Panel::OnDrop` filename parsing used an invalid `L"\/"` escape (matched `/` only),
  breaking basename extraction for Windows backslash paths.

## [0.3.1] — 2026-07-16

### Added

- **Child HWND embedding**
  - `HostCreateInfo::parent` creates `WS_CHILD` surface filling the parent client
  - `Host::FitToParent` / `ResizeClient` / `GetParentHwnd` / `IsEmbeddedChild`
  - `samples/embed_host` shell window + external message pump demo

## [0.3.0] — 2026-07-16

### Added

- **Embeddable library (Wave3 H5b/H6/H7)**
  - Static library target `ai_win_ui_lib` (`ai_win_ui::lib`)
  - Public headers: `include/ai_win_ui/host.h`, `include/ai_win_ui/version.h`
  - `ai_win_ui::Host::Create` / `Run` / `ProcessMessage` / `GetHwnd`
  - Sample: `samples/embed_host`
  - Version API: `ai_win_ui::Version()` → `"0.3.0"`
- **CI (Q8)**
  - `scripts/run_ci_local.ps1` — gallery + smoke + measure golden
  - `scripts/setup_ci_deps.ps1` — unpack/build Yoga for clean machines
  - `.github/workflows/ci.yml` — Windows Direct2D smoke+golden (Skia not in git)

### Wave2 product capabilities (prior work in 0.2.x line)

- Virtual list, Popup/overlay contract, runtime theme, ellipsis, gradient/shadow, SVG tint

## [0.2.0] — 2026-07-15

### Added

- Wave1 engine trust: text parity tooling, ScrollViewer, multi-host, measure goldens
- Style catalog, layered/custom chrome, large control set

## [0.1.0] — 2026-04

### Added

- Initial retained UI, Yoga, Direct2D/Skia backends, XML/JSON layouts
