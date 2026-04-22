# AI WinUI Docs

This directory contains the working documentation for `ai-win-ui`.

The docs are intended to describe the repository as it exists today, not an
older design snapshot.

## Main Documents

- `doc/README.md`
  - high-level overview and document index
- `doc/mvp-roadmap.md`
  - current MVP definition, completed work, and remaining gaps
- `doc/mvp-execution-plan.md`
  - ordered next-step work plan with phase goals and exit criteria
- `doc/mvp-acceptance.md`
  - repeatable manual validation checklist for MVP sign-off
- `doc/advanced-rendering-workplan.md`
  - phased plan for statistics components, table component, and seagull animation
- `doc/layout-spec.md`
  - layout DSL authoring guidance
- `doc/resource-packaging.md`
  - resource loading and packaging model
- `doc/yoga-integration.md`
  - Yoga integration notes and current layout-engine boundaries
- `doc/skia-integration.md`
  - Skia backend integration notes and runtime workflow
- `doc/skia-prebuilt-survey.md`
  - comparison of prebuilt Skia package options

## Current System Snapshot

The project is now organized around a retained UI tree with explicit backend
abstractions:

- `src/ui.h`
  - retained controls and layout-facing primitives
- `src/renderer.h`
  - renderer abstraction consumed by the UI layer
- `src/text_measurer.h`
  - text measurement abstraction
- `src/layout_engine.h`
  - layout engine abstraction, currently backed by Yoga
- `src/ui_context.h`
  - runtime dependency container for UI construction

The app currently supports:

- XML and JSON layout loading
- resources from `resource/` and optional `assets.zip`
- Direct2D and Skia renderer backends
- Yoga-backed row and column layout
- mouse, keyboard, focus, text input, and vertical scrolling

## Recommended Validation Pages

These layouts are the most useful when checking the current implementation:

- `resource/layouts/ui.xml`
  - default mixed demo page
- `resource/layouts/yoga_measure_cases.xml`
  - Yoga sizing and row/column checks
- `resource/layouts/skia_image_cases.xml`
  - image and clipping regression checks
- `resource/layouts/core_validation.xml`
  - core Skia text, Yoga flex, focus, and interaction checks
- `resource/layouts/stats_components.xml`
  - advanced statistics component checks (`StatCard`, `SparklineChart`)
- `resource/layouts/table_components.xml`
  - advanced table component checks (`DataTable`)
- `resource/layouts/seagull_animation.xml`
  - animation checks for `SeagullAnimation` and timer-driven rendering
- `resource/layouts/core_controls_v2.xml`
  - core WinForms-like controls checks (`ProgressBar`, `ListBox`, `ComboBox`, `TabControl`)
- `resource/layouts/navigation_components.xml`
  - navigation controls checks (`ListView`, `TreeView`)

## Launcher Scripts

The repository includes helper scripts to launch layouts directly:

- `scripts/run_layout_demo.ps1`
  - generic launcher with backend and layout selection
- `scripts/run_core_validation.ps1`
  - convenience wrapper for the core MVP validation page
- `scripts/run_dashboard_reference.ps1`
  - convenience wrapper for the dashboard sample
- `scripts/run_validation_suite.ps1`
  - one-command validation suite (`smoke`/`full`, single or dual renderer)

Example:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\run_layout_demo.ps1 `
  -Renderer skia `
  -Layout layouts/core_validation.xml
```

The same launcher also supports aliases:

- `core-validation`
- `yoga-measure`
- `skia-image`
- `stats-components`
- `table-components`
- `seagull-animation`
- `core-controls-v2`
- `navigation-components`

Or use the dedicated wrapper:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\run_core_validation.ps1 `
  -Renderer skia
```

Or run a suite:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\run_validation_suite.ps1 `
  -Renderer both `
  -Profile smoke
```

## MVP Focus

The main remaining MVP gaps are now:

- Skia text parity and stability
- a more complete flex behavior surface
- formalized acceptance criteria for interaction and layout validation

For the detailed status, see:

- `doc/mvp-roadmap.md`
- `doc/mvp-execution-plan.md`
- `doc/mvp-acceptance.md`
