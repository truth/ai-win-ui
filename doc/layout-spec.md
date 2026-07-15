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

**Recent rule impact (2026-07):** cross-file **style catalog** (`$style.xxx`),
nested composite styles, **`ShapePanel`**, and **layered / multi-process
shaped windows** change how you write `style` and a few root/window attributes.
See also `doc/style-catalog.md` and `doc/window-chrome.md`.

## Supported Layout Model

The DSL is built around composition primitives plus leaf controls. Both **XML**
and **JSON** describe the same tree; property names are largely shared.

### Containers

| Type | Role |
|------|------|
| `Panel` | Main row/column composition, cards, shells |
| `ScrollViewer` | Clipped viewport + scroll offset (prefer over window scroll for nested regions) |
| `Grid` | Uniform tile matrix |
| `ShapePanel` / `Shape` | Parametric filled polygon (heart / petal / oval / star) |

### Text & chrome

| Type | Role |
|------|------|
| `Label` | Short text / headings |
| `Button` | Actions; caption variants for custom chrome |
| `TextInput` | Single-line edit |
| `Spacer` | Elastic gap |

### Media

| Type | Role |
|------|------|
| `Image` | Raster via WIC / Skia |
| `SvgIcon` | Vector via Skia `SkSVGDOM` (D2D shows placeholder) |

### Core / nav / advanced (layout-parse supported)

`Checkbox`, `RadioButton`, `Slider`, `ProgressBar`, `ListBox`, `ComboBox`,
`TabControl`, `ListView`, `TreeView`, `MenuStrip`, `ToolStrip`, `StatusStrip`,
`ContextMenu`, `StatCard`, `SparklineChart`, `DataTable`, `NumericUpDown`,
`DateTimePicker`, `RichTextBox`, `SeagullAnimation`.

---

## XML vs JSON grammar

### XML

```xml
<Panel direction="column" padding="24,24,24,24" spacing="12" background="#0B121A">
  <Label text="Hello" fontSize="18" color="#F5F8FC" />
  <Button text="Go" style="$style.primaryButton" onClick="primaryAction" />
</Panel>
```

- Element tag = type name (`Panel`, `Button`, …).
- Attributes = props (strings; numbers parsed with `stof` / tokens).
- Children = nested elements.
- **Events:** `onClick="eventId"` on `Button` (attribute).

### JSON

```json
{
  "type": "Panel",
  "props": {
    "direction": "column",
    "padding": [24, 24, 24, 24],
    "spacing": 12,
    "background": "#0B121A"
  },
  "children": [
    {
      "type": "Label",
      "props": { "text": "Hello", "fontSize": 18, "color": "#F5F8FC" }
    },
    {
      "type": "Button",
      "props": {
        "text": "Go",
        "style": "$style.primaryButton"
      },
      "events": {
        "onClick": "primaryAction"
      }
    }
  ]
}
```

- `type` + `props` + optional `children`.
- Thickness / padding: JSON arrays `[l,t,r,b]`; XML comma strings `"l,t,r,b"`.
- **Events:** `events.onClick` only — **not** `props.onClick` (ignored if put in props).

### Common properties (both formats)

Applied via `ApplyCommonJsonProps` / `ApplyCommonXmlAttributes` on every element
type that calls them:

| Property | Notes |
|----------|--------|
| `name` / `id` | Stable id for measure dumps (`AI_WIN_UI_MEASURE_DUMP`) |
| `width` / `height` | Fixed size |
| `minWidth` / `maxWidth` / `minHeight` / `maxHeight` | Also kebab-case in JSON |
| `margin` | `[l,t,r,b]` |
| `border` | Box-model border thickness (Yoga) |
| `flex` / `flexGrow` / `flexShrink` / `flexBasis` | Flex; `flex="1"` shorthand |
| `alignSelf` | `auto\|stretch\|start\|center\|end` |
| `disabled` | JSON bool / forces disabled style state |
| `style` | **Object** (JSON only) **or** **catalog string** (`$style.name`) |
| `styleRef` | Catalog bare name (no `$style.` required) |
| `hitTest` | `caption` \| `client` (custom/layered chrome) |
| `role` | `titleBar` → caption hit region |
| `chrome` | Root: `system` \| `custom` \| `layered` \| `irregular` |

---

## `Panel`

Main composition primitive for shells, rows, columns, cards, toolbars.

Layout properties:

- `direction="row|column"`
- `wrap="nowrap|wrap"`
- `padding="l,t,r,b"`
- `spacing="n"`
- `background="#RRGGBB"` / `#AARRGGBB` / `transparent` / `$color.x`
- `borderColor`, `border="[l,t,r,b]"` (border participates in Yoga)
- `cornerRadius`
- `alignItems="stretch|start|center|end"`
- `justifyContent="start|center|end|space-between"`
- size / margin / flex / `alignSelf` (common props)

### `ScrollViewer`

Nested scroll region (P0). Children are measured with unbounded height when
`scrollY` is true, then clipped to the viewer bounds.

| Prop | Default | Notes |
|------|---------|--------|
| `scrollY` | true | Vertical wheel scroll |
| `scrollX` | false | Horizontal; Shift+wheel when both axes exist |
| `showScrollbars` | true | Pill scrollbars |
| `barThickness` | 10 | DIP |
| `background` / `trackColor` / `thumbColor` | optional | |
| `height` / `flexGrow` | — | Give the viewport a finite size |

Demo: `resource/layouts/scroll_viewer_cases.xml` (alias `scroll-viewer`).

Measure dump after layout:

```powershell
$env:AI_WIN_UI_MEASURE_DUMP = "1"   # or path to .ndjson
$env:AI_WIN_UI_QUIT_AFTER_MS = "800"
# or: .\scripts\run_measure_dump.ps1 -Layout scroll-viewer
```

### Style catalog impact on `Panel`

- `style="$style.surfaceCard"` (or JSON string / `styleRef`) assigns a full
  `ComponentStyle` from `resource/styles/`.
- On paint, if a layout style was set, **decoration** from `m_style` (background,
  border color/width, corner radius) is folded into drawing when the panel would
  otherwise stay transparent / unbordered.
- Catalog `padding` inside style specs is **not** automatically copied into
  Yoga padding today — still set `padding` on the element for layout inset.
- Prefer: shared surface tokens in styles JSON + explicit layout padding on the node.

---

## `ShapePanel` (`Shape`)

Filled parametric polygon for previews and irregular layered bodies.
Uses `IRenderer::FillPolygon` (Direct2D + Skia).

| Prop | Values / notes |
|------|----------------|
| `kind` / `shape` | `heart` (default), `petal` / `flower`, `oval` / `ellipse` / `circle`, `star` |
| `fill` / `background` | Fill color |
| `stroke` / `borderColor` | Outline color |
| `strokeWidth` | Outline width (0 = none) |
| `segments` | Tessellation 12–256 (default ~96) |
| `text` | Optional centered label |
| `textColor` / `color` | Label color |
| `fontSize` | Label size |
| common | `width` / `height` / flex / `hitTest` / `style` |

XML:

```xml
<ShapePanel kind="heart" width="120" height="110" fill="#E85D75"
            stroke="#FFB3C0" strokeWidth="2" text="Heart" hitTest="caption" />
```

JSON:

```json
{
  "type": "ShapePanel",
  "props": {
    "kind": "petal",
    "width": 96,
    "height": 88,
    "fill": "#7B6CFF",
    "text": "petal"
  }
}
```

For layered windows: root `chrome="layered"` + transparent background + large
`ShapePanel` as opaque body; see `doc/window-chrome.md`.

---

## Style System

### 1) Inline `style` object (JSON)

Every element accepts an optional `style` object. Each state only overrides
fields it sets; missing fields inherit from `base`. States:
`base` (normal), `hover`, `pressed`, `focused`, `disabled`, `selected`, `readonly`.

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

- `background` — solid colour (hex / theme token)
- `border` — `{ "width": n | [l,t,r,b], "color": "#..." }`
- `cornerRadius` — single radius
- `foreground` — text / indicator colour
- `fontSize`
- `padding`, `margin`, `borderWidth` — `[l,t,r,b]`
- `opacity` — `0.0..1.0`
- `track` / `thumb` / `fill` — composite controls (Slider, ProgressBar, …)

`disabled: true` forces the disabled state regardless of hover / focus.

### 2) Nested composite styles (JSON object only, or via catalog)

Inside a `style` object (layout JSON **or** styles JSON file):

| Key | Consumers (examples) |
|-----|----------------------|
| `itemStyle` | ListBox, ListView, TreeView, menu items |
| `tabStyle` | TabControl tabs |
| `dropdownStyle` | ComboBox dropdown panel / rows |

Nested value is a full `ComponentStyle` (can itself have `base` / `hover` / …).

```json
"style": {
  "base": { "background": "#121B25", "cornerRadius": 8 },
  "itemStyle": {
    "base": { "background": "#121B25", "foreground": "#E8EEF6" },
    "hover": { "background": "#1A2838" },
    "selected": { "background": "#2D6CDF", "foreground": "#FFFFFF" }
  }
}
```

**XML cannot embed nested objects** in attributes. Put nested styles in
`resource/styles/*.json` and reference with `style="$style.myList"`.

### 3) Style catalog reference (XML + JSON) — **new rule**

Named styles live under `resource/styles/` (see `doc/style-catalog.md`).

| Format | How to reference |
|--------|------------------|
| JSON | `"style": "$style.primaryButton"` or `"style": "primaryButton"` or `"styleRef": "primaryButton"` |
| XML | `style="$style.primaryButton"` or `style="primaryButton"` or `styleRef="primaryButton"` |
| Inherit in styles file | `"extend": "$style.surfaceCard"` then override `base` / states |

**Rules of thumb:**

1. Prefer `$style.xxx` for shared buttons/cards so layouts stay short.
2. Prefer inline `style` objects for one-off demos (`test_styles.json`).
3. Do not mix both on one node expecting merge: string/object forms are **exclusive**
   (`style` object **or** string **or** `styleRef`).
4. Catalog loads at startup (`styles/default.json` + optional `AI_WIN_UI_STYLES`).
5. Theme tokens (`$color.x`) still work **inside** style field values.

### Components that honor `style` / catalog

Broadly: `Button`, `TextInput`, `Checkbox`, `RadioButton`, `Slider`, `ProgressBar`,
`ListBox`, `ComboBox`, `TabControl`, `ListView`, `TreeView`, shell strips,
`StatCard`, `SparklineChart`, `DataTable` shell, and **`Panel` decoration** when
a layout style is set. Others still accept quick-set color fields; many also
call `DefaultStyle(theme)` when no layout style was provided.

---

## Theme Tokens

Layouts can reference named values from `resource/themes/default.json`
instead of hard-coding hex / numbers. The active theme is loaded once at
startup; override via `AI_WIN_UI_THEME=themes/<name>.json`.

Token syntax inside layout JSON (and style/theme-aware string fields):

- Colour: `"$color.<key>"`
- Spacing: `"$spacing.<key>"` (whole value or array element)
- Radius: `"$radius.<key>"`
- Font size: `"$fontSize.<key>"`
- Border width: `"$borderWidth.<key>"`

Unknown tokens → magenta `#FF00FF` + debug warning.

**Do not confuse with catalog:**

| Prefix | Meaning |
|--------|---------|
| `$color.` / `$spacing.` / … | Theme scalar |
| `$style.` | Named `ComponentStyle` in StyleCatalog |

`DefaultStyle()` factories keep hard-coded hex fallbacks so C++ construction
without a theme still works.

---

## Window chrome & events (layout side)

Root / common:

- `chrome="layered|custom|system"` on root when env `AI_WIN_UI_CHROME` is unset
- `hitTest="caption|client"`, `role="titleBar"`

Built-in `onClick` ids (wired in `main.cpp`):

| Id | Action |
|----|--------|
| `primaryAction` / `secondaryAction` | Demo title feedback |
| `windowMinimize` / `windowMaximize` / `windowClose` | Window commands |
| `openHeartWindow` / `openPetalWindow` / `openOvalWindow` / `openStarWindow` | Spawn **child process** with layered shape layout |

Child process also gets `AI_WIN_UI_CHROME=layered` and `AI_WIN_UI_SIZE=WxH`.
Details: `doc/window-chrome.md`.

Optional env for any process:

| Env | Meaning |
|-----|---------|
| `AI_WIN_UI_LAYOUT` | Layout path under resource |
| `AI_WIN_UI_RENDERER` | `skia` \| `direct2d` |
| `AI_WIN_UI_THEME` | Theme JSON path |
| `AI_WIN_UI_STYLES` | Extra styles JSON to merge |
| `AI_WIN_UI_CHROME` | `system` \| `custom` \| `layered` |
| `AI_WIN_UI_SIZE` | Client size `420x460` or `420,480` |
| `AI_WIN_UI_QUIT_AFTER_MS` | Headless auto-close |

---

## Layout Authoring Rules

### 1. Prefer Nested Structure Over Absolute Positioning

There is no absolute positioning system in the current DSL.

Build complex screens with nested rows/columns, cards, and `Spacer`.

### 2. Use One Container Per Visual Responsibility

- page background panel
- content shell panel
- sidebar / main / section / card panels

### 3. Fixed Width for Landmarks, Flex for Fill Areas

- fix widths for sidebars and tool strips
- `flexGrow="1"` + `flexBasis="0"` for fill columns
- `wrap="wrap"` for chip rows
- `minWidth` / `minHeight` to prevent collapse

### 4. Shared visuals → styles JSON; layout structure → layouts XML/JSON

After the style catalog:

- Put brand buttons/cards in `resource/styles/`
- Keep page structure and copy in `resource/layouts/`
- Use `$style.xxx` instead of copy-pasting large `style` objects into every JSON layout

### 5. Split Long Headlines When Needed

Use multiple `Label` rows if exact line breaks matter.

### 6. Cards as Portable Surfaces

A card usually defines background, radius, padding, spacing — either as
quick-set props or as `$style.surfaceCard` (+ explicit padding).

---

## Visual Tokens (dashboard defaults)

### Surface Hierarchy

- page background: soft neutral or tinted
- app shell: near-white or dark container
- section card: white / lightly tinted / dark elevated
- accent card: blue or violet emphasis

### Radius / Spacing / Type scales

- page shell radius: `28`–`32`; section card: `18`–`22`; pill: `10`–`14`
- page padding: `24`–`32`; section gap: `16`–`20`; card padding: `14`–`20`
- hero: `28`–`36`; section title: `17`–`20`; body: `13`–`15`; meta: `11`–`12`

---

## Limits

The current DSL does not yet offer:

- absolute positioning / z-index stacking API
- general-purpose drop shadows / gradients as layout props
  (layered mode has a special soft card shadow in the app shell)
- run-time theme or catalog hot-reload
- token-in-token inside theme files
- XML inline nested style objects (use catalog instead)
- JSON `props.onClick` (use `events.onClick`)
- SVG tint / SMIL / external `xlink:href`
- mesh geometry hit-testing (layered hits use **alpha** sampling)

---

## Workflow

```powershell
# Generic launcher (aliases: style-catalog, shaped-hub, table-components, …)
powershell -ExecutionPolicy Bypass -File .\scripts\run_layout_demo.ps1 `
  -Renderer skia `
  -Layout style-catalog

# Irregular multi-window hub
powershell -ExecutionPolicy Bypass -File .\scripts\run_shaped_windows_demo.ps1

# Headless smoke
powershell -ExecutionPolicy Bypass -File .\scripts\run_headless_smoke.ps1 `
  -Profile full -Renderer skia
```

---

## Recommended Validation Layouts

| Layout | Focus |
|--------|--------|
| `yoga_measure_cases.xml` | Yoga sizing / row-column |
| `skia_image_cases.xml` | Image / clip |
| `core_validation.xml` | Text, flex, focus |
| `core_controls_v2.xml` | Progress / List / Combo / Tab |
| `navigation_components.xml` | ListView / TreeView / strips |
| `advanced_inputs.xml` | Numeric / DateTime / RichText |
| `stats_components.xml` | StatCard / Sparkline |
| `table_components.xml` | DataTable v3 |
| `test_styles.json` | Inline multi-state `style` |
| `test_theme.json` | Theme tokens + magenta fallback |
| `test_svg.json` | SvgIcon |
| **`style_catalog_demo.xml` / `.json`** | **`$style` catalog + ShapePanel** |
| **`shaped_windows_hub.xml`** | **Open layered shape children** |
| **`demo_gallery.xml`** | **Grid of all cases · click Open (manual QA hub)** |
| `layered_chrome_demo.xml` | Rounded layered card |
| `shaped_*_window.xml` | Single-shape layered shells |

### Demo gallery events

Buttons may use:

```
onClick="openDemo:layouts/core_validation.xml"
onClick="openDemo:layouts/layered_chrome_demo.xml|layered|1000x700"
```

Format: `openDemo:<path>[|<chrome>][|<size>]` where `chrome` is `system` / `custom` / `layered`.
Opens an in-process secondary window (hub stays alive).

## Naming Guidance

- `ui.xml` — default sample
- `*_cases.xml` — focused regression
- `*_reference.xml` — image/reference-driven
- `*_demo.xml` — feature demos (chrome, catalog, shapes)
- `styles/*.json` — shared named styles (not layouts)
