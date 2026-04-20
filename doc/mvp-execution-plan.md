# AI WinUI MVP Execution Plan

## Purpose

This document turns the MVP roadmap into an execution order that can be used
for the next implementation passes.

It answers three practical questions:

1. What should we work on next?
2. In what order should we do that work?
3. What evidence is enough to say a task is actually done?

## Current Priority

The current MVP is not blocked by missing controls or missing architecture.

It is blocked by three closing tasks:

1. Skia text parity and stability
2. Yoga behavior calibration
3. MVP acceptance and validation flow

Work outside these areas is useful, but it should not take priority until the
three blockers above are in a clearly better state.

## Execution Order

### Phase 1: Stabilize Skia Text

Why this phase goes first:

- Skia text is still the largest visible quality gap in the MVP.
- The layout system already depends on text measurement for `Label`,
  `Button`, and `TextInput`.
- The current measurement and draw paths are still split, which makes parity
  work harder than it should be.

Main files:

- `src/renderer_skia.cpp`
- `src/text_measurer.h`
- `src/text_measurer_dwrite.cpp`
- `src/ui.h`
- `resource/layouts/core_validation.xml`

Implementation tasks:

1. Extract or consolidate shared text layout rules for:
   - line splitting
   - wrap decisions
   - line height
   - vertical alignment
   - clipping expectations
2. Reduce the mismatch between Skia text draw behavior and the current text
   measurement path.
3. Check `Label`, `Button`, and `TextInput` specifically on the Skia backend.
4. Compare the same validation page against Direct2D to identify only the
   differences that are still meaningful for MVP.

Exit criteria:

- wrapped labels no longer clip unexpectedly at the top or bottom
- button text remains readable and aligned in constrained widths
- text input text remains legible in normal and focused states
- `core_validation.xml` is no longer primarily failing because of Skia text

### Phase 2: Calibrate Yoga Behavior

Why this phase comes second:

- Yoga is already active and feature-complete enough for MVP-level layouts.
- The remaining issues are mostly predictability gaps, not missing
  integration.
- It is easier to reason about remaining layout differences after the Skia
  text noise is reduced.

Main files:

- `src/layout_engine.cpp`
- `src/ui.h`
- `resource/layouts/yoga_measure_cases.xml`
- `resource/layouts/core_validation.xml`
- `doc/yoga-integration.md`

Implementation tasks:

1. Re-check the current behavior of:
   - `justifyContent="spaceBetween"`
   - `alignItems="stretch"`
   - `alignSelf`
   - `flexBasis`
   - `minWidth` / `maxWidth`
   - `minHeight` / `maxHeight`
   - `wrap="wrap"`
2. Verify row sizing under constrained widths with mixed fixed and flexible
   children.
3. Decide which behaviors must match old custom layout semantics and which can
   follow Yoga semantics as the new source of truth.
4. Update the validation layouts if an unclear case needs a more focused
   repro.

Exit criteria:

- `yoga_measure_cases.xml` behaves consistently across repeated runs
- `core_validation.xml` no longer shows obvious flex instability
- the remaining intentional Yoga semantics are documented, not accidental

### Phase 3: Formalize MVP Acceptance

Why this phase comes third:

- The project already has useful manual validation pages.
- What is missing is a repeatable pass/fail routine.
- MVP sign-off needs a stable check path more than it needs more demo surface
  area.

Main files:

- `doc/mvp-roadmap.md`
- `doc/README.md`
- `resource/layouts/core_validation.xml`
- `resource/layouts/yoga_measure_cases.xml`
- `resource/layouts/skia_image_cases.xml`
- `scripts/run_layout_demo.ps1`

Implementation tasks:

1. Create a small acceptance checklist organized by:
   - backend
   - validation page
   - expected behaviors
2. Define the minimum Direct2D and Skia checks needed for MVP sign-off.
3. Record the expected interaction pass for:
   - mouse hover and click
   - keyboard focus traversal
   - text input
   - slider keyboard control
   - scroll and bring-into-view
4. Cross-link the checklist from the roadmap and docs index.

Exit criteria:

- one documented manual pass can answer whether MVP verification succeeded
- the docs clearly identify which layouts to run and what to inspect
- MVP readiness is no longer implicit or memory-based

## Immediate Next Tasks

If work resumes now, the best next sequence is:

1. Refactor the Skia text path so draw and measurement logic use more closely
   related layout rules.
2. Run the shared validation page under Skia and note the remaining visible
   failures.
3. Fix the highest-signal Yoga calibration issues found in the validation
   pages.
4. Write the MVP acceptance checklist only after the validation pages are
   stable enough to describe confidently.

## Session-By-Session Checklist

Use this lightweight progression rule to keep work focused:

1. Start with one validation page and one backend.
2. Fix the most visible blocker in code.
3. Re-run the same page before changing scope.
4. Document any behavior that is intentional and not a bug.
5. Only move to the next phase when the current one is no longer the main MVP
   blocker.

## What To Avoid Right Now

Avoid spending the next implementation pass on:

- dashboard polish
- richer DSL primitives
- visual effects such as shadows and gradients
- renderer expansion beyond the current Windows MVP path
- broad refactors that do not reduce MVP verification risk

Those are valid follow-up tasks, but they compete with the current sign-off
work.
