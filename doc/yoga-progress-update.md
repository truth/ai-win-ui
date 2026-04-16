# Yoga Integration Progress Update

## Current State

The project has now completed the first practical Yoga integration milestone.

- `Panel` no longer depends on handwritten layout as the primary path.
- `ILayoutEngine` is now the stable seam between UI code and the layout backend.
- The active backend uses Yoga for stack layout calculation.
- `Panel` supports both `column` and `row` direction.
- Child layout metadata now supports `margin`, `flexGrow`, `flexShrink`, and `flexBasis`.
- A dedicated `Spacer` element is available for elastic gaps.
- Yoga leaf measurement callbacks are now wired for leaf controls through the layout backend.
- The Yoga leaf path now keeps a lightweight per-node measurement cache during layout building.
- The app can now switch layout samples through `AI_WIN_UI_LAYOUT` at runtime.
- Layout examples in both XML and JSON have been refreshed to demonstrate:
  - vertical stacking
  - row toolbars
  - pinned footer trays
  - `Spacer + flexGrow + flexBasis`

## Files Touched By This Milestone

- `src/layout_engine.h`
- `src/layout_engine.cpp`
- `src/ui.h`
- `src/layout_parser.cpp`
- `resource/layouts/ui.xml`
- `resource/layouts/ui.json`
- `doc/yoga-integration.md`

## What Is Working

- Yoga-backed `Panel` layout in the normal app runtime path.
- Row and column direction switching through layout files.
- Flex-style spacing using `Spacer`.
- Yoga-driven leaf measurement for leaf UI nodes.
- Lightweight cached leaf measurements during Yoga layout passes.
- Layout parsing from both XML and JSON.
- Debug build generation through CMake.

## Known Limits

The current bridge is still intentionally conservative.

- `GridPanel` is still independent from Yoga.
- `justifyContent=space-between` is close to the previous behavior, but not yet fully normalized against the old handwritten implementation.
- There is not yet a generalized flex container model beyond the current `Panel`.
- The current Yoga measure path does not yet include dedicated caching or regression-oriented verification cases.
- Dedicated measurement examples now exist, but automated regression checks still do not.
- Sample switching is now easy for manual verification, but there is still no automated assertion layer.

## Recommended Next Tasks

### Priority 1

Normalize and harden the Yoga measure callback path.

Goal:
- validate the new Yoga-driven intrinsic sizing behavior
- add confidence around wrapped text, exact sizing, and at-most constraints

Suggested targets:
- `Label`
- `Button`
- `TextInput`

### Priority 2

Normalize layout behavior and add validation cases.

Goal:
- verify `space-between`, stretch, center, and end alignment against expected screenshots or layout assertions

Suggested work:
- build a few focused sample layouts
- add a lightweight layout verification path if the project later gains tests

### Priority 3

Decide whether `GridPanel` should remain bespoke or move toward Yoga-based composition.

Two reasonable directions:
- keep `GridPanel` custom for now and treat it as a specialized control
- replace it later with a more general Yoga-powered container strategy

### Priority 4

Begin Skia readiness work only after Yoga measurement is stable.

That means:
- preserve `IRenderer` as the backend seam
- avoid coupling Skia migration with ongoing Yoga behavior changes

## Short Planning Recommendation

The next sprint can be kept small and steady:

1. Add Yoga measure callbacks for text and button-like controls.
2. Add 2-3 focused layout samples specifically for alignment edge cases.
3. Rebuild and manually verify behavior before touching rendering backend work.
