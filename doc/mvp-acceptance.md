# AI WinUI MVP Acceptance Checklist

## Purpose

This document defines the repeatable manual validation flow for MVP sign-off.

It should be used after a build completes successfully and before declaring
that the current repository state is ready for MVP acceptance.

## Validation Pages

Use these pages as the shared acceptance surface:

- `layouts/core_validation.xml`
  - primary check for Skia text, Yoga flex behavior, focus, keyboard input,
    and focus-driven scrolling
- `layouts/yoga_measure_cases.xml`
  - focused Yoga sizing, row/column, and wrap behavior checks
- `layouts/skia_image_cases.xml`
  - focused image, clipping, and rounded-corner checks
- `layouts/stats_components.xml`
  - advanced statistics controls (`StatCard`, `SparklineChart`)
- `layouts/table_components.xml`
  - advanced table control (`DataTable`)
- `layouts/seagull_animation.xml`
  - timer-driven animation control (`SeagullAnimation`)
- `layouts/core_controls_v2.xml`
  - core WinForms-like control set (`ProgressBar`, `ListBox`, `ComboBox`, `TabControl`)
- `layouts/navigation_components.xml`
  - navigation controls (`ListView`, `TreeView`)

## Recommended Run Commands

Run the shared launcher script from the repository root.

Skia:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\run_core_validation.ps1 `
  -Renderer skia
```

Direct2D:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\run_core_validation.ps1 `
  -Renderer direct2d
```

Repeat the same pattern for:

- `layouts/yoga_measure_cases.xml`
- `layouts/skia_image_cases.xml`
- `layouts/stats_components.xml`
- `layouts/table_components.xml`
- `layouts/seagull_animation.xml`
- `layouts/core_controls_v2.xml`
- `layouts/navigation_components.xml`

Suite option:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\run_validation_suite.ps1 `
  -Renderer both `
  -Profile full
```

## Acceptance Pass Order

Use this order during manual verification:

1. Build the backend you want to validate.
2. Run `core_validation.xml`.
3. Run `yoga_measure_cases.xml`.
4. Run `skia_image_cases.xml`.
5. Repeat the `core_validation.xml` pass on the other backend when parity
   needs to be checked.

## Core Validation Checklist

### 1. Wrapped Text and Clipping

Pass if all of the following are true:

- wrapped labels stay inside their cards
- first-line baseline placement looks stable
- last lines do not clip unexpectedly at the bottom
- text in taller containers does not drift vertically
- button text remains readable in constrained widths
- text input text remains readable while focused

### 2. Yoga Row and Column Behavior

Pass if all of the following are true:

- `space-between` rows distribute items predictably
- fixed-width cards stay fixed
- flex items expand into remaining space without obvious collapse
- `minWidth`, `minHeight`, and `alignSelf` behave as authored
- wrap cases move to the next row cleanly when width is exhausted

### 3. Interaction and Focus Chain

Pass if all of the following are true:

- `Tab` moves through the controls in a stable order
- the focused control remains visibly highlighted
- text input accepts typing and keeps the caret visible
- checkbox and radio button react to mouse and keyboard activation
- slider responds to keyboard input

### 4. Scroll and Bring-Into-View

Pass if all of the following are true:

- repeated `Tab` navigation reaches below-the-fold controls
- the app auto-scrolls to keep the active control visible
- scroll position does not overshoot or clamp incorrectly near the bottom

## Yoga Measurement Checklist

Run `layouts/yoga_measure_cases.xml`.

Pass if all of the following are true:

- row and column sizing remain stable across repeated launches
- mixed fixed and flexible children do not overlap or collapse unexpectedly
- label wrapping under constrained widths remains predictable
- wrap layouts break onto new lines in a stable way

## Image and Clipping Checklist

Run `layouts/skia_image_cases.xml`.

Pass if all of the following are true:

- images render at all expected locations
- rounded clipping stays aligned with the visual container
- image scaling does not show obvious backend-specific breakage
- clipping boundaries do not leak outside rounded shapes

## Advanced Component Checklist

Run the advanced layouts in this order:

1. `layouts/stats_components.xml`
2. `layouts/table_components.xml`
3. `layouts/seagull_animation.xml`
4. `layouts/core_controls_v2.xml`
5. `layouts/navigation_components.xml`

Pass if all of the following are true:

- `StatCard` and `SparklineChart` render without clipping or overlap
- `DataTable` header/row boundaries stay readable and stable
- `SeagullAnimation` updates continuously without visible artifacts
- `ProgressBar`, `ListBox`, `ComboBox`, and `TabControl` respond to mouse and keyboard input
- `ListView` and `TreeView` support stable selection and keyboard traversal

Shortcut: `run_layout_demo.ps1` also accepts aliases:

- `stats-components`
- `table-components`
- `seagull-animation`
- `core-controls-v2`
- `navigation-components`

## Backend Comparison Notes

When comparing Skia against Direct2D, record only differences that are still
meaningful for MVP.

Record:

- layout shifts that change usability
- clipped or unreadable text
- visibly broken alignment
- incorrect focus or interaction behavior

Do not block MVP on minor raster differences that do not change readability or
interaction correctness.

## Sign-Off Rule

The current build can be considered ready for MVP sign-off when:

1. the app builds successfully for the required backend
2. `core_validation.xml` passes on the target backend
3. Yoga validation is stable on `yoga_measure_cases.xml`
4. image and clipping validation is stable on `skia_image_cases.xml`
5. remaining backend differences are documented and judged non-blocking for MVP
