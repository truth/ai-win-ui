# Skia Prebuilt Survey

## Purpose

This document records the investigation into ready-made Windows Skia binaries
for this project and captures the decision for the next integration phase.

Investigation date:

- 2026-04-17

Decision:

- choose `aseprite/skia` prebuilt Windows packages as the short-term Skia
  integration path
- keep `vcpkg` and self-build workflows as follow-up work after the renderer is
  integrated and validated

Current follow-up status:

- the repository now builds successfully against an unpacked `aseprite/skia`
  package through the `local-sdk` path
- the app has also been launched successfully with `AI_WIN_UI_RENDERER=skia`
- the immediate remaining work is now renderer parity and documentation, not
  package discovery

## What Was Evaluated

### Option 1: Official `google/skia` Releases

Result:

- not suitable

Reason:

- the GitHub releases page for `google/skia` does not provide packaged release
  artifacts for Windows C++ consumers

Notes:

- official Skia documentation still treats source build workflows as the normal
  path

Sources:

- https://github.com/google/skia/releases
- https://skia.org/docs/user/build/

### Option 2: `vcpkg` Skia Port

Result:

- technically viable in principle
- not suitable as the immediate delivery path for this repository right now

Reason:

- the package exists and is actively maintained
- however, the current local `vcpkg` path has been too slow and unstable for
  this integration sprint
- repeated configure/install attempts did not produce a reliable, quick loop for
  day-to-day renderer work

Notes:

- `vcpkg` remains a valid later option once the renderer is already working with
  a known-good binary package
- it is still useful for future reproducibility and long-term dependency
  management

Sources:

- https://vcpkg.io/en/package/skia.html
- https://learn.microsoft.com/vcpkg/consume/manifest-mode

### Option 3: `rust-skia` Prebuilt Binaries

Result:

- not chosen

Reason:

- prebuilt Windows binaries do exist for `x86_64-pc-windows-msvc`
- however, they are published for Rust bindings consumption rather than as a
  straightforward C++ SDK
- library layout, expected feature combinations, and integration assumptions are
  less predictable for this C++ project

Sources:

- https://github.com/rust-skia/rust-skia
- https://github.com/rust-skia

### Option 4: `aseprite/skia` Prebuilt Packages

Result:

- chosen

Reason:

- the repository explicitly publishes prebuilt binary packages on its releases
  page
- the release notes describe these archives as usable by unpacking them into a
  local Skia directory, similar to a local build output
- the related `aseprite/laf` CMake flow already documents consuming such a
  prebuilt package via `SKIA_DIR` and `SKIA_LIBRARY_DIR`
- this matches the repository's existing `local-sdk` idea better than the other
  options

Relevant release:

- `Skia-m124`
- release date: 2024-06-11
- includes `Skia-Windows-Debug-x64.zip`

Important packaging notes from the release page:

- Windows package is built with `clang`
- Windows package uses `-MT`

Sources:

- https://github.com/aseprite/skia
- https://github.com/aseprite/skia/releases
- https://github.com/aseprite/laf

## Decision Summary

The repository will use `aseprite/skia` prebuilt packages as the near-term
integration strategy.

This means:

- treat Skia as a local SDK style dependency first
- prioritize getting the current `renderer_skia.cpp` linked and running
- postpone `vcpkg` cleanup and self-build automation until after integration is
  proven

## Planned Repository Direction

### Short-Term

1. Download and unpack an `aseprite/skia` Windows x64 package into a local SDK
   directory.
2. Adjust the current `local-sdk` branch of the CMake setup to match that
   package layout.
3. Link the existing `renderer_skia.cpp` against the unpacked package.
4. Validate runtime switching with `AI_WIN_UI_RENDERER=skia`.

### Medium-Term

1. Compare Skia raster output against Direct2D for text, clipping, bitmap draw,
   and DPI behavior.
2. Decide whether to stay on the prebuilt package longer or move back to a
   fully managed dependency path.

### Long-Term

1. Revisit `vcpkg` once the renderer is already functional.
2. Revisit source builds only after integration behavior is understood and
   stable.

## Risks To Track

- `aseprite/skia` is a third-party fork, not the official Skia distribution
- the packaged branch version may differ from the latest upstream Skia API
- Windows binaries are built with a specific toolchain choice and CRT mode,
  which must be checked against this repository's build settings before final
  adoption

## Working Conclusion

For this project and this phase, `aseprite/skia` gives the best balance between
practical delivery speed and acceptable integration risk.
