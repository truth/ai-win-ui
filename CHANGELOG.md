# Changelog

All notable changes to **ai-win-ui** are documented in this file.

Format based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).
Versioning follows [SemVer](https://semver.org/) (`AI_WIN_UI_VERSION_*` in `include/ai_win_ui/version.h`).

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
