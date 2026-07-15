# AI WinUI Docs

This directory contains the working documentation for `ai-win-ui`.

The docs are intended to describe the repository as it exists today, not an
older design snapshot.

## Main Documents

- `doc/README.md`
  - high-level overview and document index
- `doc/layout-spec.md`
  - **layout DSL authoring** (XML/JSON rules, style forms, ShapePanel, chrome attrs)
- `doc/style-catalog.md`
  - **cross-file styles** (`resource/styles/*.json`, `$style.xxx`, import/extend)
- `doc/window-chrome.md`
  - custom title bar, layered irregular windows, multi-process shape demos
- `doc/resource-packaging.md`
  - resource loading and packaging model
- `doc/mvp-roadmap.md`
  - current MVP definition, completed work, and remaining gaps
- `doc/mvp-execution-plan.md`
  - ordered next-step work plan with phase goals and exit criteria
- `doc/mvp-acceptance.md`
  - repeatable manual validation checklist for MVP sign-off
- `doc/advanced-rendering-workplan.md`
  - phased plan for statistics components, table component, and seagull animation
- `doc/yoga-integration.md`
  - Yoga integration notes and current layout-engine boundaries
- `doc/skia-integration.md`
  - Skia backend integration notes and runtime workflow
- `doc/skia-prebuilt-survey.md`
  - comparison of prebuilt Skia package options
- `doc/plan/`
  - dated implementation plans (shipped and active)
  - active: `2026-07-15-directui-p0-hardening.md`（文本 parity / ScrollViewer / 多根窗口 / measure golden）
  - archived examples: declarative style v1, theme tokens, layered window v2

## Current System Snapshot

The project is organized around a retained UI tree with explicit backend
abstractions:

| Area | Files |
|------|--------|
| Controls | `src/ui.h` |
| Renderer | `src/renderer.h` (+ D2D / Skia) |
| Text measure | `src/text_measurer.h` |
| Layout engine | `src/layout_engine.h` (Yoga) |
| Context | `src/ui_context.h` (includes `StyleCatalog*`) |
| Box decoration | `src/box_decoration.*` |
| Component style | `src/style.*` |
| **Style catalog** | **`src/style_catalog.*`** |
| Theme tokens | `src/theme.*` |
| Window chrome | `src/window_chrome.*` |
| Layout parse | `src/layout_parser.*` |

The app currently supports:

- XML and JSON layout loading
- resources from `resource/` and optional `assets.zip`
- Direct2D and Skia backends (Skia auto-detected when `third_party/skia-m124` is present)
- Yoga-backed row/column layout (border participates in the box model)
- mouse, keyboard, focus, text input, vertical scrolling; pill scrollbars on lists
- declarative `style` blocks with multi-state overrides and nested
  `itemStyle` / `tabStyle` / `dropdownStyle`
- **named style catalog**: `resource/styles/*.json` with `import` / `extend`;
  layouts reference via `style="$style.name"` (XML) or `"style": "$style.name"` (JSON)
- theme tokens (`$color.x`, `$spacing.x`, …) at load time, magenta fallback for unknowns
- **`ShapePanel`**: heart / petal / oval / star via `FillPolygon`
- custom chrome + **layered** per-pixel alpha windows; multi-process shaped demos
- vector icons via `SvgIcon` on Skia (`SkSVGDOM`); D2D shows a placeholder

### Layout / style rule impact (quick reference)

| Topic | XML | JSON |
|-------|-----|------|
| Inline multi-state style | Not in attributes — use catalog | `"style": { "base": … }` |
| Catalog reference | `style="$style.primaryButton"` | `"style": "$style.primaryButton"` |
| Nested item/tab styles | Via catalog file only | Inline object or catalog |
| Button click | `onClick="id"` attribute | `"events": { "onClick": "id" }` only |
| Layered root | `chrome="layered"` | `"chrome": "layered"` |
| Shape body | `<ShapePanel kind="heart" …/>` | `"type": "ShapePanel"` |

Full detail: `doc/layout-spec.md` + `doc/style-catalog.md`.

## Recommended Validation Pages

- `resource/layouts/ui.xml` — default mixed demo
- `resource/layouts/yoga_measure_cases.xml` — Yoga sizing
- `resource/layouts/skia_image_cases.xml` — image / clip
- `resource/layouts/core_validation.xml` — core MVP checks
- `resource/layouts/stats_components.xml` — StatCard / Sparkline
- `resource/layouts/table_components.xml` — DataTable v3
- `resource/layouts/seagull_animation.xml` — timer animation
- `resource/layouts/core_controls_v2.xml` — Progress / List / Combo / Tab
- `resource/layouts/navigation_components.xml` — ListView / TreeView / strips
- `resource/layouts/advanced_inputs.xml` — Numeric / DateTime / RichText
- `resource/layouts/test_styles.json` — inline style states
- `resource/layouts/test_theme.json` — theme tokens
- `resource/layouts/test_svg.json` — SvgIcon
- **`resource/layouts/style_catalog_demo.xml`** — `$style` + ShapePanel
- **`resource/layouts/shaped_windows_hub.xml`** — open layered shape children
- `resource/layouts/layered_chrome_demo.xml` — rounded layered card

## Launcher Scripts

- `scripts/run_layout_demo.ps1` — generic launcher (aliases include
  `style-catalog`, `shaped-hub`, `table-components`, …)
- `scripts/run_shaped_windows_demo.ps1` — irregular multi-window hub
- `scripts/run_layered_chrome_demo.ps1` — layered rounded card
- `scripts/run_custom_chrome_demo.ps1` — custom HWND chrome
- `scripts/run_core_validation.ps1` — core MVP page
- `scripts/run_dashboard_reference.ps1` — dashboard sample
- `scripts/run_headless_smoke.ps1` — quit-after-ms smoke suite
- `scripts/run_validation_suite.ps1` — smoke/full multi-layout suite

Example:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\run_layout_demo.ps1 `
  -Renderer skia `
  -Layout style-catalog

powershell -ExecutionPolicy Bypass -File .\scripts\run_shaped_windows_demo.ps1
```

## MVP Focus

Main remaining MVP gaps:

- Skia text parity and stability
- a more complete flex behavior surface
- formalized acceptance criteria for interaction and layout validation

See `doc/mvp-roadmap.md`, `doc/mvp-execution-plan.md`, `doc/mvp-acceptance.md`.
