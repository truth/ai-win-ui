# AI WinUI MVP Roadmap

## Goal

This document tracks the practical MVP state of `ai-win-ui`.

It is meant to answer two questions clearly:

1. What is already working in the repository today?
2. What is still missing before the current MVP can be considered complete?

## MVP Definition

The current MVP is a retained-mode Win32 UI demo that can:

- create a Win32 window and render a retained UI tree
- load layouts from `resource/layouts/` in XML or JSON form
- load images and other resources from `resource/` or `assets.zip`
- support a usable set of basic controls and interactions
- run on the existing Direct2D backend and the in-progress Skia backend
- use Yoga as the active layout engine without leaking backend types into the UI API

## Current MVP Status

### Completed

- Window creation, resize, paint, and DPI-awareness flow are working.
- Public UI interfaces no longer leak Direct2D, DirectWrite, or WRL types.
- `IRenderer` is in place and supports backend selection.
- Direct2D renderer remains available as the stable baseline backend.
- Skia renderer can build and launch through the local SDK path.
- `UIContext` now carries renderer, text measurer, layout engine, resource provider, and event resolver.
- Layout loading from `resource/layouts/*.xml` and `resource/layouts/*.json` works.
- `DirectoryResourceProvider`, `ZipResourceProvider`, and fallback resource loading are in place.
- `assets.zip` fallback behavior works.
- Yoga is the active layout engine through `ILayoutEngine`.
- `Panel` supports `row` and `column` layout, `alignItems`, `justifyContent`, spacing, padding, and child margin.
- Flex-related child properties currently supported:
  - `flex`
  - `flexGrow`
  - `flexShrink`
  - `flexBasis`
  - `alignSelf`
  - `minWidth` / `maxWidth`
  - `minHeight` / `maxHeight`
- Container wrap support currently supported:
  - `wrap="wrap"`
- Core controls are present:
  - `Label`
  - `Button`
  - `TextInput`
  - `Image`
  - `Checkbox`
  - `RadioButton`
  - `Slider`
  - `Spacer`
- Interaction chain is present for the basic control set:
  - click
  - hover
  - pressed
  - focus
  - keyboard navigation
  - text input
- Vertical scrolling and focus-driven bring-into-view behavior are working at the app layer.
- Launcher scripts exist for running layout demos directly.
- Validation layouts now exist for focused regression checks:
  - `resource/layouts/yoga_measure_cases.xml`
  - `resource/layouts/skia_image_cases.xml`
  - `resource/layouts/core_validation.xml`
- Text measurement now follows the active renderer backend instead of always
  assuming the DirectWrite path.
- Single-line controls now measure against `NoWrap` expectations so layout and
  rendering are more closely aligned.

### In Progress

- Wave 1 (L1 engine trust) **closed** 2026-07-16.
- Wave 2 (L2 product capability) **in progress** — C4/C5/S4/R6/H5-draft done;
  see `doc/plan/2026-07-15-directui-productization-personas.md` § Wave2 进度.

### Not Yet Complete (Wave2 remainder / Wave3)

- Pixel-perfect Skia↔D2D raster match is **not** required; known differences
  are documented in `doc/skia-integration.md` (R5). Metrics ≤2px @96DPI is the
  L1 bar and is met for core/text-wrap dumps.
- Interaction automation beyond headless smoke + measure golden is still light
  (Q6/Q7 optional).
- DSL visual polish **R6–R8 done** (ellipsis, gradient/shadow, Svg tint/cache).
  Optional further: multi-stop gradients, true Gaussian blur shadows.
- Embed library target + sample (H5b/H6) is Wave3.

## MVP Completion Checklist

### 1. Runtime Foundation

- [x] Win32 window lifecycle is stable
- [x] DPI awareness and resize path work
- [x] UI tree can render through an abstract renderer
- [x] Resource loading works from `resource/`
- [x] `assets.zip` fallback path works

### 2. Layout and UI Construction

- [x] XML layout loading works
- [x] JSON layout loading works
- [x] Layout parser can build the retained UI tree
- [x] Yoga is connected behind `ILayoutEngine`
- [x] Row and column panel layouts are available
- [x] Common spacing and alignment properties are available
- [x] Flex wrap support is available
- [x] Flex shorthand support is available

### 3. Rendering

- [x] Direct2D backend is available
- [x] Skia backend can build and launch
- [x] Images render through both backends
- [x] Rounded clipping exists in both backends
- [x] Skia text behavior is aligned closely enough with layout expectations (≤2px metrics)
- [x] Skia and Direct2D rendering parity is documented; core validation pages measure-golden both backends

### 4. Controls and Interaction

- [x] `Label`, `Button`, and `TextInput` are usable
- [x] `Checkbox`, `RadioButton`, and `Slider` are usable
- [x] Click, hover, pressed, focus, keyboard navigation, and input all exist
- [x] Vertical scrolling works
- [x] Focus can bring controls back into view
- [x] Interaction regression coverage is documented as a stable acceptance checklist

### 5. Developer Workflow

- [x] Layout demo launcher script exists
- [x] Dashboard demo launcher script exists
- [x] Core validation page exists
- [x] Docs are cross-linked (README / layout-spec / acceptance / personas)
- [x] A repeatable acceptance flow is documented for MVP sign-off

## MVP Sign-Off Status

L1 / Wave1 exit criteria are met (text metrics ≤2px, dual-backend measure
goldens, ScrollViewer matrix, acceptance docs). Use `doc/mvp-acceptance.md`
for a repeatable manual pass when declaring a given build “MVP verified.”

Product work continues under **Wave 2** (not MVP-blocking): R7/R8 visuals,
optional C6/C7, Q tooling, then Wave3 embed packaging.

Recent validation improvements that help close those gaps:

- `justifyContent="spaceBetween"` is now parsed the same way as
  `space-between`, so the shared validation page matches the authored DSL more
  reliably.
- `resource/layouts/yoga_measure_cases.xml` now includes a regression block for
  row-stretch cards and fixed inner host heights so the "infinite height"
  failure mode is easier to catch during manual verification.

## Recommended Next Milestones

### Milestone A: Skia Text Stability

Focus:

- line wrapping
- vertical alignment consistency
- clipping correctness
- text input readability
- parity checks against Direct2D

Exit criteria:

- `core_validation.xml` looks stable on Skia for wrapped labels, buttons, and text input
- text no longer appears as the main blocker in everyday demo layouts

### Milestone B: Yoga Predictability

Focus:

- `space-between`
- `stretch`
- `flexBasis`
- `min/max` constraints
- row sizing under pressure
- wrap edge cases and shorthand semantics under pressure

Exit criteria:

- `core_validation.xml` and `yoga_measure_cases.xml` behave consistently across repeated manual checks

### Milestone C: Interaction Acceptance

Focus:

- finalize focus traversal expectations
- verify bring-into-view behavior
- document mouse and keyboard acceptance flow
- define the MVP acceptance checklist

Exit criteria:

- one documented pass over the validation pages can confirm MVP readiness

## Immediate Execution Plan

Follow the persona Wave board:

1. Finish Wave2 remaining: **R7** gradients/shadow, **R8** Svg tint (optional polish).
2. Keep C4/C5/S4 demos green: `popup-theme`, `virtual-list`, `text-ellipsis`.
3. Wave3 when packaging matters: H5b `ui_host.cpp` + H6 `ai_win_ui_lib`.

Detailed lists:

- `doc/plan/2026-07-15-directui-productization-personas.md`
- `doc/mvp-execution-plan.md`
- `doc/mvp-acceptance.md`

## Non-MVP Work

These items are valuable, but they are not required to close the current MVP:

- perfect dashboard screenshot recreation
- richer visual DSL primitives
- shadow system
- separator primitives
- icon packs
- advanced theming
- remote resources
- hot reload
- cross-platform renderer targets beyond the current Windows-first path

## Suggested Priority Order

1. ~~Stabilize Skia text / Yoga / acceptance~~ (Wave1 done).
2. Wave2 product demos (C4/C5/S4/R6) — **current**.
3. R7/R8 visual primitives when demos need them.
4. Wave3 embed lib + version/CHANGELOG for external consumers.
