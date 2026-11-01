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
  - repeatable manual validation checklist for MVP sign-off (+ Gallery Q4 表)
- `doc/overlay-popup-contract.md`
  - overlay z-order / 点外关闭 / Esc（C2→C4）
- `doc/keyboard-matrix.md`
  - 控件键盘操作矩阵
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
  - **Modern Components v2 (Avatar, Badge, Switch, SearchBox, CircularProgress, Donut, Expander, SplitView):** `2026-11-01-modern-components-v2.md`
  - **Visual Rendering v2 (Shadows, Typography, Vector Icons & Smooth Charts):** `2026-11-01-visual-rendering-v2.md`
  - **Medlio Dashboard UI Alignment:** `2026-11-01-medlio-dashboard-alignment.md`
  - **Component Include (Phase 1):** `2026-10-31-component-include-v1.md`
  - **Modular Parsers (pugixml & yyjson):** `2026-10-31-modular-parsers-v1.md`
  - **Wave5 keyboard matrix:** `2026-09-26-keyboard-matrix-v1.md`
  - **Wave5 Chart v2 (Phase 1):** `2026-10-23-chart-v2.md`
  - **Wave5 UI Automation:** `2026-10-09-uia-automation-basics.md`
  - **Wave5 transition DSL (Q8):** `2026-09-26-declarative-transition-dsl-v1.md`
  - **Wave5 observable binding:** `2026-09-26-observable-binding-v1.md`
  - **Wave5 flex & chrome specs:** `2026-09-26-flex-chrome-spec-v1.md`
  - **Wave4 architecture & performance:** `2026-07-26-architecture-refactoring-and-optimization-v1.md`
  - **active productization:** `2026-07-15-directui-productization-personas.md`
    （按 6 个人物/角色分工：R 文本 · L 布局 · C 控件 · S 样式 · H 宿主 · Q 质量）
  - Wave2 embed API draft: `2026-07-16-embed-api-v1.md`
  - P0 hardening: `2026-07-15-directui-p0-hardening.md`
  - archived examples: declarative style v1, theme tokens, layered window v2
- `doc/reports/`
  - dated test and QA regression reports
  - **Modular Parsers Test Report:** `2026-10-31-modular-parsers-test-report.md`

## Current System Snapshot

The project is organized around a retained UI tree with explicit backend
abstractions:

| Area | Files |
|------|--------|
| Controls | `src/ui.h`, `src/ui_panels.*`, `src/ui_media.h`, **`src/ui_icons.h`** (`<Icon>` / `<Path>`), **`src/ui_widgets.h`** (`<Avatar>`, `<Badge>`, `<Divider>`, `<SegmentedControl>`, `<Switch>`, `<SearchBox>`, `<Alert>`, `<CircularProgress>`, `<EmptyState>`), **`src/ui_containers.h`** (`<Expander>`, `<SplitView>`) |
| **Chart** | **`src/ui_chart.h`** (smooth spline line / vertical gradient area / rounded pill bar + highlight badge / pie & donut ring charts, dashed grid, axes, legend, tooltip) |
| Renderer | `src/renderer.h`, `src/renderer.cpp` (D2D), `src/renderer_skia.cpp` (Skia) |
| Text measure & layout | `src/text_measurer.h`, `src/skia_font_shared.h`, `src/skia_text_layout.*` (100..900 fontWeight, fontFamily, lineHeight) |
| Layout engine | `src/layout_engine.h` (Yoga) |
| Context | `src/ui_context.h` (includes `StyleCatalog*`) |
| Box decoration | `src/box_decoration.*` (Gaussian soft shadow, 4-corner `CornerRadius`, 4-side independent borders) |
| Component style | `src/style.*` |
| **Style catalog** | **`src/style_catalog.*`** |
| Theme tokens | `src/theme.*` |
| Window chrome | `src/window_chrome.*` |
| Layout parse | `src/json_parser.*`, `src/xml_parser.*`, `src/layout_parser.*`, `src/layout_parser_panels.*`, `src/layout_parser_controls.*` |
| **UiHost / multi-host** | **`src/ui_host.h`**, `src/ui_host_input.cpp`, `src/ui_host_overlay.cpp`, `src/application.*` |
| Animation & Reactive | `src/animation_controller.*`, `<ai_win_ui/observable.h>` |
| **C-API Interop** | **`<ai_win_ui/c_api.h>`**, `src/c_api.cpp` |
| Host impl (TU) | `src/main.cpp` (`class UiHost`) |

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
- **Component Include**: `<Include src="..." />` (XML) or `type: "Include"` (JSON) with relative path resolution and attribute overrides
- **`ShapePanel`**: heart / petal / oval / star via `FillPolygon`
- **Visual Rendering v2 & Modern Components v2**:
  - True Gaussian blur drop shadows (`SkMaskFilter::MakeBlur` on Skia, 12-step Gaussian falloff on Direct2D)
  - 4-corner independent `CornerRadius` (`cornerRadius="tl,tr,br,bl"`) and 4-side independent borders (`borderBottom="1,#EEF2F7"`, etc.)
  - Multi-weight typography (`fontWeight="100..900|medium|semibold|bold"`, `fontFamily`, `lineHeight`)
  - Built-in Lucide-style vector icons (`<Icon name="search|bell|calendar|..." />`) and inline SVG paths (`<Path data="M..." />`) across both Skia and Direct2D
  - Universal `tooltip="..."` attribute on all `UIElement` instances
  - Modern widgets: `<Avatar>`, `<Badge>`, `<Divider>`, `<SegmentedControl>`, `<Switch>`, `<SearchBox>`, `<Alert>`, `<CircularProgress>`, `<EmptyState>`, `<Expander>`, and `<SplitView>`
  - Smooth Catmull-Rom spline curves (`smooth="true"`), vertical linear gradient area fills, rounded pill bar charts with highlight badges, and Pie/Donut ring charts (`type="pie|donut"`)
- custom chrome + **layered** per-pixel alpha windows; multi-process shaped demos
- external SVG files via `SvgIcon` on Skia (`SkSVGDOM`)

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
- `resource/layouts/modern_components_v2.xml` — showcase of all 12 Modern Components v2 primitives & Donut chart
- `resource/layouts/medlio_dashboard.xml` — high-fidelity SaaS medical dashboard (Visual Rendering v2 showcase)
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
- **`resource/layouts/virtual_list_demo.xml`** — C5 VirtualListBox 1k
- **`resource/layouts/popup_theme_demo.xml`** — C4/S4 Popup + theme switch
- **`resource/layouts/text_ellipsis_cases.xml`** — R6 Label ellipsis
- **`resource/layouts/gradient_shadow_demo.xml`** — R7 gradient + soft shadow
- **`resource/layouts/svg_tint_demo.xml`** — R8 SvgIcon tint + cache

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
- **`scripts/run_ci_local.ps1`** — Wave2 **Q8** local CI: gallery + smoke + measure golden

Example:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\run_layout_demo.ps1 `
  -Renderer skia `
  -Layout style-catalog

powershell -ExecutionPolicy Bypass -File .\scripts\run_shaped_windows_demo.ps1
```

## Productization focus (Wave2)

Wave1 (engine trust) is closed. Wave2 product capability — current:

| Area | Status | Demo / doc |
|------|--------|------------|
| Virtual list 1k (C5) | done | `virtual-list` |
| Popup + Combo overlay (C4) | done | `popup-theme`, `overlay-popup-contract.md` |
| Runtime theme (S4) | done | `applyTheme:` + `$color` / catalog rebind |
| Label ellipsis (R6) | done | `text-ellipsis` |
| Gradient + shadow (R7) | done | `gradient-shadow` |
| SVG tint + cache (R8) | done | `svg-tint` |
| Local CI smoke+golden (Q8) | done | `run_ci_local.ps1` + GHA D2D |
| Embed lib (H5b/H6/H7) | **done 0.3.1** | `ai_win_ui_lib` + parent child embed + `samples/embed_host` |

Board: `doc/plan/2026-07-15-directui-productization-personas.md`.  
Acceptance: `doc/mvp-acceptance.md`.
