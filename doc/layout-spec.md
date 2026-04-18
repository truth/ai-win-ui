# Layout Specification

## Purpose

This document defines the current layout authoring conventions for the
`ai-win-ui` retained UI demo. It is meant to help us build repeatable page
layouts with the existing XML/JSON DSL, especially for dashboard-style screens.

The goal is not pixel-perfect design tooling. The goal is a practical layout
grammar that:

- stays inside the current `UIElement` and `LayoutParser` feature set
- produces stable Yoga-backed structures
- is easy to read and maintain in source control
- is expressive enough for card dashboards, tool panels, and image-driven pages

## Supported Layout Model

The current DSL is built around a small set of primitives:

- `Panel`
- `Grid`
- `Label`
- `Button`
- `TextInput`
- `Image`
- `Checkbox`
- `RadioButton`
- `Slider`
- `Spacer`

### `Panel`

`Panel` is the main composition primitive.

Use it for:

- page shells
- rows
- columns
- cards
- toolbars
- stacked content groups

Supported layout properties:

- `direction="row|column"`
- `padding="l,t,r,b"`
- `spacing="n"`
- `background="#RRGGBB"`
- `cornerRadius="n"`
- `alignItems="stretch|start|center|end"`
- `justifyContent="start|center|end|space-between"`
- `width`
- `height`
- `margin`
- `flexGrow`
- `flexShrink`
- `flexBasis`

### `Grid`

`Grid` is best for uniform tiles.

Use it for:

- icon galleries
- image matrices
- repeated fixed-size cards

Supported properties:

- `columns`
- `spacing`
- `rowHeight`
- `padding`
- `background`
- `cornerRadius`
- common size and margin properties

### Leaf Controls

Use leaf controls with simple, explicit responsibilities:

- `Label` for short text blocks and headings
- `Button` for strong actions, pills, and chips
- `TextInput` for search bars and editable fields
- `Image` for illustrations and thumbnails
- `Spacer` for elastic gaps inside row/column containers

## Layout Authoring Rules

### 1. Prefer Nested Structure Over Absolute Positioning

There is no absolute positioning system in the current DSL.

Build complex screens with:

- a page shell
- nested rows
- nested columns
- cards inside cards
- `Spacer` for flexible gaps

### 2. Use One Container Per Visual Responsibility

Recommended pattern:

- page background panel
- content shell panel
- sidebar panel
- main content panel
- section panels
- card panels

Avoid one giant panel with too many mixed concerns.

### 3. Fixed Width for Landmarks, Flex for Fill Areas

For dashboard layouts:

- fix widths for sidebars, promo cards, and tool strips
- use `flexGrow="1"` with `flexBasis="0"` for major content columns
- use `Spacer` to push controls to the edge of a row

### 4. Split Long Headlines Into Multiple Labels When Needed

The current text system is still simpler than a full design tool.

For hero headlines or tightly controlled wraps:

- use multiple `Label` rows
- avoid depending on automatic line wrapping for critical title composition

### 5. Treat Cards as Reusable Surface Blocks

A dashboard card should usually define:

- its own `background`
- its own `cornerRadius`
- local `padding`
- local `spacing`

This keeps the outer page shell clean and makes card sections portable.

## Visual Tokens

These are recommended defaults for dashboard pages in the current demo:

### Surface Hierarchy

- page background: soft neutral or tinted background
- app shell: near-white container
- section card: white or lightly tinted panel
- accent card: blue or violet emphasis surface

### Radius Scale

- page shell: `28` to `32`
- section card: `18` to `22`
- tile or pill: `10` to `14`
- compact chip: `8` to `10`

### Spacing Scale

- page padding: `24` to `32`
- section gap: `16` to `20`
- card padding: `14` to `20`
- internal row spacing: `8` to `12`

### Typography Scale

- page hero: `28` to `36`
- section title: `17` to `20`
- body copy: `13` to `15`
- meta copy: `11` to `12`

## Dashboard Composition Pattern

The reference dashboard style works well with this structure:

1. Root page panel
2. Rounded white application shell
3. Fixed-width left sidebar
4. Flexible main content column
5. Top navigation row
6. Hero plus highlight row
7. Three-card information row
8. Bottom action and summary row

This is exactly the structure used by:

- `resource/layouts/dashboard_reference.xml`

## Limits To Keep In Mind

The current DSL does not yet offer:

- absolute positioning
- z-index layering
- drop shadows
- vector icons
- advanced text layout controls
- gradients

Because of that, screenshot recreation should prioritize:

- layout structure
- surface hierarchy
- spacing
- proportions
- color blocking

Do not expect perfect iconography or shadow fidelity yet.

## Workflow

To preview a layout file directly:

```powershell
$env:AI_WIN_UI_LAYOUT = "layouts/dashboard_reference.xml"
Start-Process -FilePath ".\build\presets\dev-debug-skia-local-sdk\Debug\ai_win_ui.exe"
```

Or use the helper script:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\run_dashboard_reference.ps1
```

If the executable is missing and you want the script to build it first:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\run_dashboard_reference.ps1 -BuildIfMissing
```

Or compare the same layout on Direct2D:

```powershell
$env:AI_WIN_UI_RENDERER = "direct2d"
$env:AI_WIN_UI_LAYOUT = "layouts/dashboard_reference.xml"
Start-Process -FilePath ".\build\Debug\ai_win_ui.exe"
```

For a reusable launcher with explicit layout and backend parameters:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\run_layout_demo.ps1 `
  -Renderer skia `
  -Layout layouts/dashboard_reference.xml
```

To validate the configuration without opening a window:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\run_layout_demo.ps1 `
  -Renderer skia `
  -Layout layouts/dashboard_reference.xml `
  -BuildIfMissing `
  -NoLaunch
```

## Naming Guidance

Recommended naming patterns:

- `ui.xml` for the default sample
- `*_cases.xml` for focused regression layouts
- `*_reference.xml` for image/reference-driven recreations
- `*_prototype.xml` for exploratory layouts

This keeps intent clear when several demo screens live in the same repository.
