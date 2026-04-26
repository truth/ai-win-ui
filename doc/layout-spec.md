# Layout Specification

## Purpose

This document defines the current layout authoring conventions for the
`ai-win-ui` retained UI demo.

The goal is not pixel-perfect design tooling. The goal is a practical layout
grammar that:

- stays inside the current `UIElement` and `LayoutParser` feature set
- produces stable Yoga-backed structures
- is easy to read and maintain in source control
- is expressive enough for cards, tool panels, image pages, and validation pages

## Supported Layout Model

The current DSL is built around a small set of primitives:

- `Panel`
- `Grid`
- `Label`
- `Button`
- `TextInput`
- `Image`
- `SvgIcon`
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
- `wrap="nowrap|wrap"`
- `padding="l,t,r,b"`
- `spacing="n"`
- `background="#RRGGBB"`
- `borderColor="#RRGGBB"`
- `border="[l, t, r, b]"` (participates in the Yoga box model)
- `cornerRadius="n"`
- `alignItems="stretch|start|center|end"`
- `justifyContent="start|center|end|space-between"`
- `width`
- `height`
- `minWidth`
- `maxWidth`
- `minHeight`
- `maxHeight`
- `margin`
- `flex="1"` or `flex="1 1 160"`
- `flexGrow`
- `flexShrink`
- `flexBasis`
- `alignSelf="auto|stretch|start|center|end"`

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
- `SvgIcon` for vector icons that stay crisp at any DPI / size
- `Spacer` for elastic gaps inside row/column containers

### `SvgIcon`

`SvgIcon` renders an SVG file from the resource provider via Skia's
`SkSVGDOM`. Requires the Skia backend at runtime — under Direct2D the
icon paints a "SVG" placeholder so the missing dependency is visible
rather than silent.

Supported properties:

- `source="icons/<name>.svg"` (resource path)
- `stretch="fill|uniform|uniformToFill"` (default `uniform`)
- `width`, `height`, plus the common size and margin properties

Example:

```json
{
  "type": "SvgIcon",
  "props": { "source": "icons/check.svg", "width": 24, "height": 24 }
}
```

To see real SVG output:

```powershell
$env:AI_WIN_UI_RENDERER = "skia"
.\build\Debug\ai_win_ui.exe
```

v1 limits: no tint / colour override, no cross-instance caching, no SMIL
animation, no external `xlink:href` resources.

## Style System

Every element accepts an optional `style` object that drives its visual
state. Each state spec only overrides the fields it sets — missing fields
inherit from `base`. Recognised states:
`normal` (== base), `hover`, `pressed`, `focused`, `disabled`,
`selected`, `readonly`.

```json
{
  "type": "Button",
  "props": {
    "text": "Submit",
    "style": {
      "base": {
        "background": "#2D2D30",
        "border": { "width": 1, "color": "#6A6A6A" },
        "cornerRadius": 4,
        "foreground": "#FFFFFF",
        "fontSize": 14
      },
      "hover":    { "background": "#3E3E42" },
      "pressed":  { "background": "#0E639C" },
      "focused":  { "border": { "width": 2, "color": "#FFFFFF" } },
      "disabled": { "background": "#1F1F1F", "opacity": 0.6 }
    }
  }
}
```

Field summary inside any state spec:

- `background` — solid colour
- `border` — `{ "width": n | [l,t,r,b], "color": "#..." }`
- `cornerRadius` — single radius
- `foreground` — text / indicator colour
- `fontSize`
- `padding`, `margin`, `borderWidth` — `[l,t,r,b]`
- `opacity` — `0.0..1.0`

`disabled: true` on a component forces the disabled state regardless of
hover / focus.

Components currently driven by `style`: `Button`, `TextInput`, `Panel`,
`Checkbox`, `RadioButton`. Other components still use their per-field
quick-set properties (background, cornerRadius, fontSize) which are
visually equivalent.

## Theme Tokens

Layouts can reference named values from `resource/themes/default.json`
instead of hard-coding hex / numbers. The active theme is loaded once at
startup; override file via env var `AI_WIN_UI_THEME=themes/<name>.json`.

Token syntax inside layout JSON:

- Colour fields:    `"$color.<key>"` (e.g. `"$color.surface-1"`)
- Spacing fields:   `"$spacing.<key>"` — accepted both as the whole value
  (`"padding": "$spacing.lg"`) and per array element
  (`"padding": ["$spacing.lg", 12, ...]`)
- Radius fields:    `"$radius.<key>"`
- Font size fields: `"$fontSize.<key>"`
- Border width:     `"$borderWidth.<key>"`

Categories live in `resource/themes/default.json` under `colors`,
`spacings`, `radii`, `fontSizes`, `borderWidths`. Unknown tokens render
as **magenta `#FF00FF`** plus a debug warning so typos surface
immediately rather than degrading silently.

`DefaultStyle()` factories on each component keep their hard-coded hex
values so the framework still runs without any theme file (`new Button()`
in C++ stays self-sufficient).

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

For dashboard and tool layouts:

- fix widths for sidebars, promo cards, and tool strips
- use `flexGrow="1"` with `flexBasis="0"` for major content columns
- use `flex="1"` when you want the common grow/shrink/fill pattern without spelling out three properties
- use `Spacer` to push controls to the edge of a row
- use `minWidth` and `minHeight` to keep flexible cards from collapsing too far
- use `alignSelf` when one child needs to opt out of the container's cross-axis rule
- use `wrap="wrap"` on narrow row containers when chips, pills, or fixed-width cards should flow onto the next line

### 4. Split Long Headlines Into Multiple Labels When Needed

The text system is improving, but it is still not a full design tool.

For hero headlines or tightly controlled wraps:

- use multiple `Label` rows if exact composition matters
- do not depend on automatic wrapping for critical art-direction decisions

### 5. Treat Cards as Reusable Surface Blocks

A dashboard card should usually define:

- its own `background`
- its own `cornerRadius`
- local `padding`
- local `spacing`

This keeps the outer shell clean and makes card sections portable.

## Visual Tokens

Recommended defaults for dashboard-style pages:

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

## Limits To Keep In Mind

The current DSL does not yet offer:

- absolute positioning
- z-index layering
- drop shadows
- gradients
- run-time theme switching (theme is read once at startup)
- token-in-token (a theme entry referencing another theme entry)
- styled `Slider` / `ListBox` / `ComboBox` / `TabControl` / `ListView` /
  `TreeView` (these still use per-field properties — `style` block is
  ignored on them until sub-style support lands)
- SVG tint / colour override
- advanced text layout controls

Vector icons via `SvgIcon` are now supported on the Skia backend.

Because of that, screenshot recreation should prioritize:

- layout structure
- surface hierarchy
- spacing
- proportions
- color blocking

## Workflow

To launch the core validation page:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\run_layout_demo.ps1 `
  -Renderer skia `
  -Layout layouts/core_validation.xml
```

To validate the configuration without opening a window:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\run_layout_demo.ps1 `
  -Renderer skia `
  -Layout layouts/core_validation.xml `
  -BuildIfMissing `
  -NoLaunch
```

To launch the dashboard reference page:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\run_dashboard_reference.ps1
```

## Recommended Validation Layouts

- `resource/layouts/yoga_measure_cases.xml`
  - Yoga sizing and row/column checks
- `resource/layouts/skia_image_cases.xml`
  - image rendering and clipping checks
- `resource/layouts/core_validation.xml`
  - Skia text, Yoga flex, focus, and interaction checks
- `resource/layouts/dashboard_reference.xml`
  - reference-driven layout exploration, not the primary MVP gate
- `resource/layouts/test_styles.json`
  - `style` block four-state colour cycling (Button hover/pressed/focused/disabled)
  - exercises the inset-border rule (`Panel border=4`)
- `resource/layouts/test_theme.json`
  - theme token resolution across colour, spacing, radius, font size
  - includes a deliberately broken `$color.does-not-exist` to verify the
    magenta fallback + warning path
- `resource/layouts/test_svg.json`
  - `SvgIcon` intrinsic vs scaled rendering, three stretch modes,
    16/24/48/96 px scale check (Skia backend only — D2D shows placeholders)

## Naming Guidance

Recommended naming patterns:

- `ui.xml` for the default sample
- `*_cases.xml` for focused regression layouts
- `*_reference.xml` for image/reference-driven recreations
- `*_prototype.xml` for exploratory layouts
