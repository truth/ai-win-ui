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

### In Progress

- Skia text rendering has moved past the original single-line placeholder and now handles wrapped text more reliably, but it still does not yet match DirectWrite closely enough to be considered finished.
- Yoga integration is active and useful, but the predictable-flex surface is still growing and still needs more behavior calibration around edge cases.

### Not Yet Complete

- Skia text measurement and Skia text rendering still use different engines.
- There is no fully verified parity between Direct2D and Skia for text, wrapping, clipping, and DPI behavior.
- There is still no stronger automated regression layer for layout and interaction; validation is mostly manual through sample pages.
- DSL visual expression is still limited:
  - no shadows
  - no separators as first-class primitives
  - no vector icon system
  - no gradients
  - no richer text layout controls

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
- [ ] Skia text behavior is aligned closely enough with layout expectations
- [ ] Skia and Direct2D rendering parity is manually verified for the main validation pages

### 4. Controls and Interaction

- [x] `Label`, `Button`, and `TextInput` are usable
- [x] `Checkbox`, `RadioButton`, and `Slider` are usable
- [x] Click, hover, pressed, focus, keyboard navigation, and input all exist
- [x] Vertical scrolling works
- [x] Focus can bring controls back into view
- [ ] Interaction regression coverage is documented as a stable acceptance checklist

### 5. Developer Workflow

- [x] Layout demo launcher script exists
- [x] Dashboard demo launcher script exists
- [x] Core validation page exists
- [ ] Docs are fully normalized and cross-linked
- [ ] A repeatable acceptance flow is documented for MVP sign-off

## What Is Still Missing For MVP Sign-Off

If we stay strict about MVP completion, these are the remaining gaps that matter most:

1. Finish the Skia text path enough that `Label`, `Button`, and `TextInput` behave predictably under the Skia backend.
2. Close the biggest Yoga calibration gaps, especially the remaining flex behavior and wrap edge cases that affect predictability.
3. Turn the current manual validation pages into a documented acceptance checklist so we can say "this build passed MVP verification" with confidence.

Everything else, including richer dashboard recreation and more expressive visuals, is useful but not blocking MVP sign-off.

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

The roadmap above defines what is left.

The recommended execution order for the next work sessions is:

1. Skia text parity
   - close the gap between text measurement and Skia text drawing
   - validate `Label`, `Button`, and `TextInput` first
2. Yoga behavior calibration
   - re-check `spaceBetween`, stretch behavior, `flexBasis`, `min/max`, and wrap pressure cases
   - use `yoga_measure_cases.xml` and `core_validation.xml` as the primary repro surfaces
3. MVP acceptance flow
   - document the manual validation pass only after the main visual and layout issues are stable enough to describe

For the detailed ordered task list, see:

- `doc/mvp-execution-plan.md`

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

1. Stabilize Skia text.
2. Finish the most important Yoga flex behavior.
3. Formalize interaction and acceptance validation.
4. Only then return to richer DSL and visual expression.
