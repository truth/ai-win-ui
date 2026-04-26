# Advanced Rendering And Component Workplan

## Purpose

This document defines the next delivery stage after MVP baseline closure.
It focuses on richer drawing capabilities and new reusable UI components:

- statistics components
- table component
- seagull animation effect

The goal is to keep implementation incremental, testable, and aligned with the
existing XML/JSON layout pipeline.

## Status Update — 2026-04-26

Beyond the original three deliverables above, four out-of-scope tracks
landed this sprint while the components matured. They are now part of
the rendering baseline:

- **Declarative style system v1** — `BoxDecoration`, `ComponentStyle`,
  seven `StyleState` values, `Button` / `TextInput` / `Panel` /
  `Checkbox` / `RadioButton` migrated to the new pipeline. Border now
  participates in the Yoga box model (long-standing latent bug).
  Plan: `doc/plan/2026-04-26-declarative-style-system-v1.md`.
- **D2D DPI fix** — `SetDpi(96, 96)` aligns the Direct2D coordinate
  space to client pixels so hover / hit-test stops drifting on >100%
  DPI displays.
- **Theme tokens v1** — `$color`, `$spacing`, `$radius`, `$fontSize`,
  `$borderWidth` lookups against `resource/themes/default.json`,
  magenta sentinel for unknown keys, five primary layouts already
  migrated. Plan: `doc/plan/2026-04-26-theme-tokens-v1.md`.
- **SVG via SkSVGDOM** — `IRenderer::DrawSvg`, `SvgIcon` UIElement,
  CMake auto-detection of `third_party/skia-m124`, Direct2D placeholder
  so missing-backend cases stay visible.
  Plan: `doc/plan/2026-04-26-skia-svg-integration-v1.md`.

Open follow-ups (queued for the next sprint):

- `Slider` / `ListBox` / `ComboBox` / `TabControl` / `ListView` /
  `TreeView` need a sub-style schema (`decoration.track`,
  `decoration.thumb`, `itemStyle`) before they can adopt
  `ComponentStyle`. Until then they keep their per-field properties.
- `cornerRadius` / `fontSize` token migration of the remaining
  layouts (only the five primary layouts are migrated so far).
- Layout colour-field migration is intentionally deferred — it requires
  a colour-naming decision separate from the token mechanic itself.
- `DefaultStyle()` factories still hard-code hex so they work without
  any theme; lazy theme resolution at `SetContext` time is a v2 task.

## Current Baseline And Constraint Notes

Current repository state:

- retained UI tree and parser are in place
- Direct2D and Skia backends are both available
- Yoga is active as the default stack layout engine

Important constraint:

- Yoga support is practical but not fully complete as a full Flexbox feature
  parity implementation.
- `Panel` stack layout is Yoga-backed, but `GridPanel` is still bespoke and not
  replaced by a generalized Yoga table/grid strategy.
- New component design should avoid assuming 100 percent CSS/Flex parity.

## Delivery Goals

1. Add richer drawing primitives required by charts and animation.
2. Add reusable statistics components for dashboard scenarios.
3. Add a reusable data table component for structured data display.
4. Add a configurable seagull animation component.
5. Expand component coverage toward common WinForms usage patterns.
6. Keep both XML and JSON authoring paths supported.
7. Add validation layouts and acceptance checks for each new capability.

## Implementation Progress

As of 2026-04-22 on branch `feat/advanced-rendering-components`:

- Phase 1 is implemented:
  - `IRenderer` now includes line and polyline primitives.
  - Direct2D and Skia backends implement those primitives.
  - timer dispatch now supports global UI tree ticks instead of only focused controls.
- Phase 2 is implemented:
  - `StatCard` and `SparklineChart` exist in `src/ui.h`.
  - parser support exists for XML and JSON.
  - validation layouts added:
    - `resource/layouts/stats_components.xml`
    - `resource/layouts/stats_components.json`
- Phase 3 is implemented:
  - `DataTable` static v1 component is added.
  - parser support exists for XML and JSON.
  - validation layouts added:
    - `resource/layouts/table_components.xml`
    - `resource/layouts/table_components.json`
- Phase 4 is implemented:
  - `SeagullAnimation` component is added with timer-driven flight and wing swing.
  - parser support exists for XML and JSON.
  - validation layouts added:
    - `resource/layouts/seagull_animation.xml`
    - `resource/layouts/seagull_animation.json`
- Phase 5 is implemented:
  - `scripts/run_layout_demo.ps1` supports layout aliases for validation pages.
  - `scripts/run_validation_suite.ps1` adds one-command smoke/full validation runs.
  - `doc/README.md` and `doc/mvp-acceptance.md` include advanced validation paths.
- Phase 6 is implemented (v1):
  - added `ProgressBar`, `ListBox`, `ComboBox`, and `TabControl` in `src/ui.h`.
  - parser support exists for XML and JSON.
  - validation layouts added:
    - `resource/layouts/core_controls_v2.xml`
    - `resource/layouts/core_controls_v2.json`
- Phase 7 is in progress:
  - added `ListView` and `TreeView` in `src/ui.h`.
  - parser support exists for XML and JSON.
  - validation layouts added:
    - `resource/layouts/navigation_components.xml`
    - `resource/layouts/navigation_components.json`
  - shell strips are pending:
    - `MenuStrip`
    - `ToolStrip`
    - `StatusStrip`
    - `ContextMenu`

## WinForms Component Gap Analysis

Current custom component set in this repository:

- `Panel`
- `GridPanel`
- `Label`
- `Button`
- `TextInput`
- `Image`
- `Checkbox`
- `RadioButton`
- `Slider`
- `Spacer`
- `ProgressBar`
- `ListBox`
- `ComboBox`
- `TabControl`
- `ListView`
- `TreeView`

Common WinForms components not yet available:

- navigation and shell:
  - menu strip
  - tool strip
  - status strip
  - context menu
- data presentation:
  - data grid view style table features beyond static table v1
- input controls:
  - numeric up-down
  - date time picker
  - rich text box

Suggested delivery priority:

1. MenuStrip, ToolStrip, StatusStrip, ContextMenu
2. NumericUpDown, DateTimePicker, RichTextBox

## Execution Plan

### Phase 1: Rendering Foundation Upgrade

Scope:

- extend `IRenderer` with line and polyline style primitives needed by charts
- align Direct2D and Skia implementations for those primitives
- upgrade timer/event flow so non-focused animated elements can update each
  frame tick

Primary files:

- `src/renderer.h`
- `src/renderer.cpp`
- `src/renderer_skia.cpp`
- `src/main.cpp`

Exit criteria:

- chart and animation primitives are available in both renderers
- frame tick can invalidate animation elements even when no control is focused

### Phase 2: Statistics Component Set

Scope:

- add `StatCard` component
- add `SparklineChart` component
- support value, trend text, color accents, and numeric point lists

Primary files:

- `src/ui.h`
- `src/layout_parser.cpp`
- `resource/layouts/stats_components.xml`
- `resource/layouts/stats_components.json`

Exit criteria:

- both components render in Direct2D and Skia
- XML and JSON parsing produce equivalent output

### Phase 3: Data Table Component

Scope:

- add `DataTable` for static dashboard data presentation
- support header row, data rows, column widths, row striping, and cell padding
- first version is display-only (no editing, no virtual scroll, no sorting)

Primary files:

- `src/ui.h`
- `src/layout_parser.cpp`
- `resource/layouts/table_components.xml`
- `resource/layouts/table_components.json`

Exit criteria:

- table layout is stable for medium datasets (for example 50 to 100 rows)
- text clipping and row boundaries remain readable in both backends

### Phase 4: Seagull Animation

Scope:

- add `SeagullAnimation` component
- support path movement and wing swing parameters
- allow instance count, speed, amplitude, size, and opacity tuning

Primary files:

- `src/ui.h`
- `src/layout_parser.cpp`
- `resource/layouts/seagull_animation.xml`
- `resource/layouts/seagull_animation.json`

Exit criteria:

- animation is smooth under normal demo load
- no visible rendering artifacts in either backend

### Phase 5: Validation And Documentation

Scope:

- add launcher aliases or documented commands for new validation layouts
- update acceptance checklist for new components
- cross-link docs index and execution notes

Primary files:

- `doc/README.md`
- `doc/mvp-acceptance.md`
- `scripts/run_layout_demo.ps1`

Exit criteria:

- one pass document clearly tells what to run and what pass/fail means

### Phase 6: Core WinForms-Like Controls

Scope:

- add `ProgressBar`
- add `ComboBox`
- add `ListBox`
- add `TabControl`

Primary files:

- `src/ui.h`
- `src/layout_parser.cpp`
- `resource/layouts/core_controls_v2.xml`
- `resource/layouts/core_controls_v2.json`

Exit criteria:

- controls are keyboard and mouse usable
- control states render consistently in both backends

### Phase 7: Structured Navigation Components

Scope:

- add `ListView`
- add `TreeView`
- add shell-level strips:
  - `MenuStrip`
  - `ToolStrip`
  - `StatusStrip`
  - `ContextMenu`

Primary files:

- `src/ui.h`
- `src/layout_parser.cpp`
- `resource/layouts/navigation_components.xml`
- `resource/layouts/navigation_components.json`

Exit criteria:

- hierarchical selection and expansion states are stable
- menu and strip interactions are predictable and testable

### Phase 8: Advanced Input Components

Scope:

- add `NumericUpDown`
- add `DateTimePicker`
- add `RichTextBox` (v1 formatting subset)

Primary files:

- `src/ui.h`
- `src/layout_parser.cpp`
- `resource/layouts/advanced_inputs.xml`
- `resource/layouts/advanced_inputs.json`

Exit criteria:

- values can be edited safely
- focus and keyboard behavior match the existing interaction model

## Proposed Component DSL (Initial)

### StatCard

Initial props:

- `title`
- `value`
- `deltaText`
- `accentColor`
- `background`
- `cornerRadius`

### SparklineChart

Initial props:

- `points` (array of numeric values)
- `lineColor`
- `fillColor`
- `strokeWidth`
- `minValue` (optional)
- `maxValue` (optional)

### DataTable

Initial props:

- `columns` (column labels and optional fixed widths)
- `rows` (array of row records)
- `headerBackground`
- `rowBackgroundA`
- `rowBackgroundB`
- `cellPadding`

### SeagullAnimation

Initial props:

- `count`
- `speed`
- `wingAmplitude`
- `pathWidth`
- `pathHeight`
- `scale`
- `opacity`

## Risks And Guardrails

- Do not start with a large parser rewrite. Add only minimal parser branches for
  new components.
- Avoid backend-specific behavior in component logic.
- Keep first versions intentionally narrow and stable before adding advanced
  interactions.
- Treat Yoga as the primary stack engine but keep table internals explicit and
  deterministic.
- For WinForms-like controls, prioritize behavior parity over visual parity.
- Add each component with a minimal property surface first, then expand only
  after validation coverage is in place.

## Step-By-Step Start Checklist

1. Implement phase 1 rendering primitive additions.
2. Add one validation layout dedicated to rendering primitive verification.
3. Implement `StatCard` and `SparklineChart`.
4. Validate statistics components on both backends.
5. Implement `DataTable`.
6. Validate table readability and row stability.
7. Implement `SeagullAnimation`.
8. Validate animation smoothness and clipping safety.
9. Update acceptance docs and launcher guidance.
10. Implement phase 6 core WinForms-like controls.
11. Implement phase 7 navigation components.
12. Implement phase 8 advanced input components.
13. Add acceptance cases for each new component family.
