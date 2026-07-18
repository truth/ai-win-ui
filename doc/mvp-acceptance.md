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
  - navigation and shell controls (`ListView`, `TreeView`, `MenuStrip`, `ToolStrip`, `StatusStrip`, `ContextMenu`)

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
- `MenuStrip`, `ToolStrip`, and `ContextMenu` respond to arrow keys and mouse selection
- `StatusStrip` segments render without overlap or clipping

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

---

## ScrollViewer nested wheel matrix (Wave1 · L2)

Run `layouts/scroll_viewer_cases.xml` (alias `scroll-viewer`).

```powershell
.\scripts\run_layout_demo.ps1 -Layout scroll-viewer -Renderer skia
```

| Case | Action | Pass if |
|------|--------|---------|
| A vertical | Wheel / drag content / drag thumb over `scroller` | Content moves; labels stay clipped |
| B nested ListView | Wheel over nested ListView | List rows move; **outer** `outerNest` does not move until list is at top/bottom |
| B nested ListBox | Wheel over nested ListBox | Same as ListView |
| B outer padding | Wheel over nest intro / pad labels (not lists) | Outer nest scrolls |
| C dual-axis Y | Wheel (no Shift) over `dualAxis` | Vertical offset changes |
| C dual-axis X | Shift+wheel or drag H thumb | Horizontal offset changes |

Measure golden (both backends):

```powershell
.\scripts\run_measure_golden.ps1 -Layout scroll-viewer -Renderer both
.\scripts\run_measure_golden.ps1 -Layout core-validation -Renderer both
.\scripts\run_measure_golden.ps1 -Layout yoga-measure -Renderer both
```

## IME acceptance (Wave1 · C3)

Requires a CJK IME (e.g. Microsoft Pinyin) on Windows.

```powershell
.\scripts\run_layout_demo.ps1 -Layout advanced-inputs -Renderer skia
# also: core-validation TextInput
```

| Step | Pass if |
|------|---------|
| Focus `TextInput` / `RichTextBox` | Caret visible |
| Start composition (拼音) | Composition window anchors near caret top-left (not top-left of screen) |
| Candidate list | Appears near caret / bottom of field; does not permanently cover the whole field (`CFS_EXCLUDE`) |
| Commit characters | Text appears in control; caret advances |
| Move caret with arrows mid-field | IME window follows on next composition |
| Do **not** call `ImmSetCompositionWindow` from `WM_IME_NOTIFY` | No stack overflow (see `doc/skills.md`) |

## Wave2 product checks (C4 / C5 / S4 / R6)

### C5 Virtual list (1k)

```powershell
.\scripts\run_layout_demo.ps1 -Layout virtual-list -Renderer skia
```

| Step | Pass if |
|------|---------|
| Open page | List fills viewport with “Row #N” |
| Wheel / PageDown | Scrolls smoothly; no multi-second hitch |
| Jump near end | Row #999 reachable |
| Select + arrows | Selection visible; bring-into-view works |

### C4 Popup + Combo shared overlay

```powershell
.\scripts\run_layout_demo.ps1 -Layout popup-theme -Renderer skia
```

| Step | Pass if |
|------|---------|
| Open Popup flyout | Body above siblings |
| Open Combo | Dropdown uses same placement rules |
| Click outside | Both dismiss (no stuck overlay) |
| Esc | DismissAllOverlays closes open flyouts |

### S4 Runtime theme

On `popup-theme`:

| Step | Pass if |
|------|---------|
| Theme: Light | Root/card labels + primary buttons lighten (token surfaces) |
| Theme: Dark | Return to dark tokens |
| Title bar | Shows `Theme: themes/...` |

### R6 Text ellipsis

```powershell
.\scripts\run_layout_demo.ps1 -Layout text-ellipsis -Renderer skia
# optional parity: -Renderer direct2d
```

| Step | Pass if |
|------|---------|
| Narrow English `ellipsis=true` | Single line ends with `…`, no wrap |
| Wrap control (no ellipsis) | Multi-line wrap as before |
| CJK `ellipsis=true` | Truncates with `…` inside fixed width |

### R7 Gradient + soft shadow

```powershell
.\scripts\run_layout_demo.ps1 -Layout gradient-shadow -Renderer skia
.\scripts\run_layout_demo.ps1 -Layout gradient-shadow -Renderer direct2d
```

| Step | Pass if |
|------|---------|
| Left card | Diagonal blue→purple fill + soft drop shadow under card |
| Middle card | Vertical green→gold gradient |
| Right card | Solid surface + deep soft shadow (no gradient) |
| Dual backend | Both backends show gradient + shadow (softness may differ) |

### R8 SVG tint + cache

```powershell
.\scripts\run_layout_demo.ps1 -Layout svg-tint -Renderer skia
```

| Step | Pass if |
|------|---------|
| Untinted row | Icons show original SVG colors |
| Tinted check row | Four checks in blue/green/gold/red (SrcIn) |
| Same path multiple times | No parse stutter; tints independent |
| Direct2D | Placeholder boxes follow tint colors (not full SVG) |

## Local CI gate (Wave2 · Q8)

One-shot automated gate (gallery coverage + headless smoke + measure goldens):

```powershell
# Full gate (build + skia/d2d smoke + all goldens)
.\scripts\run_ci_local.ps1

# Faster when already built
.\scripts\run_ci_local.ps1 -SkipBuild -Renderer both -SmokeProfile smoke

# Golden + gallery only
.\scripts\run_ci_local.ps1 -SkipBuild -SkipSmoke
```

| Step | Pass if |
|------|---------|
| build | Debug (or Release) exits 0 |
| gallery coverage | every `openDemo:` path exists |
| headless smoke | profile pages exit 0 per backend |
| measure golden | `core-validation`, `yoga-measure`, `scroll-viewer` × backends within tol |

GitHub Actions: `.github/workflows/ci.yml` runs the same script on `windows-latest`.

## Host exit / COM stability (Wave1 · H4)

| Step | Pass if |
|------|---------|
| Skia: open `core-validation`, close with X, repeat ≥10 | No WER / AV |
| Direct2D: same | No WER / AV |
| Gallery: open child host, close child, close main | Process exits cleanly; D2D releases RT before `CoUninitialize` |
| Headless smoke | `.\scripts\run_headless_smoke.ps1 -Profile full -Renderer both` exits 0 |

## Gallery 手工验收表（Wave1 · Q4）

入口：`layouts/demo_gallery.xml`（别名 `gallery` / `demo-gallery`）。

```powershell
.\scripts\clear_ai_win_env.ps1   # 建议先清粘连 env
.\scripts\run_layout_demo.ps1 -Layout gallery -Renderer skia
# 或 headless 仅冒烟：
.\scripts\run_headless_smoke.ps1 -Profile full -Renderer skia -SkipBuild
.\scripts\check_gallery_coverage.ps1
```

对 **每一张卡** 勾选（打开子窗 → 关子窗 → 主窗仍活）：

| 卡 / 布局 | 打开 OK | 关子窗 OK | 无崩 | 备注 |
|-----------|---------|-----------|------|------|
| Core Validation | ☐ | ☐ | ☐ | |
| Yoga Measure | ☐ | ☐ | ☐ | |
| Core Controls V2 | ☐ | ☐ | ☐ | Combo 点外/Esc 关列表 |
| Navigation | ☐ | ☐ | ☐ | |
| Advanced Inputs | ☐ | ☐ | ☐ | |
| Table | ☐ | ☐ | ☐ | |
| Stats | ☐ | ☐ | ☐ | |
| Scroll Viewer | ☐ | ☐ | ☐ | 拖拽 + 滚轮 |
| Text Wrap Cases | ☐ | ☐ | ☐ | R2 |
| CJK Render | ☐ | ☐ | ☐ | |
| Skia Image | ☐ | ☐ | ☐ | |
| Style Catalog | ☐ | ☐ | ☐ | |
| Shaped Hub | ☐ | ☐ | ☐ | layered 子窗 |
| Custom / Layered Chrome | ☐ | ☐ | ☐ | |
| Dashboard | ☐ | ☐ | ☐ | |
| Seagull | ☐ | ☐ | ☐ | |
| demo_gallery 自身 | ☐ | — | ☐ | 滚到底无裁切错乱 |

**进程内多窗：** 默认 in-process；关子窗不得 `PostQuit` 整个进程。  
**粘连 env：** 异常启动先 `clear_ai_win_env` 或 `AI_WIN_UI_IGNORE_ENV=1`。

覆盖脚本（路径存在性，非交互）：`scripts/check_gallery_coverage.ps1` 必须 PASS。
