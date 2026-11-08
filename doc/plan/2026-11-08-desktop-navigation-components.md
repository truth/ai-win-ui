# Desktop Navigation & Data View Components v3

**Date:** 2026-11-08  
**Author:** AI WinUI Core Team  
**Status:** Completed  
**Branch:** `feature/desktop-navigation-components` (merged into `master`)  
**Scope:** `<TreeView>`, `<TreeItem>`, `<Pagination>`, `<Breadcrumb>`, `<BreadcrumbItem>`

---

## 1. Background & Objectives

Following modern desktop design conventions (Windows 11 File Explorer, VS Code, Ant Design / Radix UI), multi-level file/category navigation and paginated tabular data presentation are foundational needs for rich enterprise desktop software.

In this release, we deliver:
1. **`<Breadcrumb>` & `<BreadcrumbItem>`**: Path navigation with auto `/` separators, clickable history crumbs, and active leaf state.
2. **`<Pagination>`**: Standard page navigation with Previous / Next buttons, direct numbered pages, jump-to-page input box, and total count indicator.
3. **`<TreeView>` & `<TreeItem>` Upgrade**:
   - Hierarchical indenting (`indent="18"`).
   - Expand/collapse chevrons (`▼` / `▶`) with smooth state transitions.
   - Node-level checkboxes (`showCheckBox="true"`).
   - Vector icon integration (`folder`, `file`, `home`, `code`, `database`).
   - Guide lines between tree nodes (`showLines="true"`).
   - Declarative nested `<TreeItem>` parsing in both XML and JSON while preserving legacy `items="A/B/C"` string compatibility.

---

## 2. Component Design & Implementation

### 2.1 Breadcrumb (`src/ui_widgets.h`)
- **`BreadcrumbItem`**: Clickable navigation segment with hover highlighting, active-crumb color distinction, and customizable route target (`href` / `onClick`).
- **`Breadcrumb`**: Horizontal flow container rendering child items separated by customizable separator glyphs (`/`, `>`, `›`, `\`).

### 2.2 Pagination (`src/ui_widgets.h`)
- **`Pagination`**:
  - `page`: current 1-based page index.
  - `total`: total items count.
  - `pageSize`: items per page (default: 10 or 20).
  - Calculated `PageCount() = (total + pageSize - 1) / pageSize`.
  - Previous (`<`) and Next (`>`) button states with disabled styling at boundary pages.
  - Compact sliding window of numbered page buttons (showing first, last, and window around active page with ellipsis `...`).
  - Total items display label (`Total {count} items`).
  - Page change callback `OnPageChanged(int newPage)` triggered on click or keyboard Enter.

### 2.3 TreeView & TreeItem (`src/ui_views.h`)
- **`TreeItem`**:
  - Properties: `header` (display text), `icon` (vector icon name), `isExpanded` (boolean), `isSelected` (boolean), `isChecked` (boolean), `value` (identifier).
  - Children: Recursive list of nested child items.
- **`TreeView`**:
  - Upgraded from simple string list to full tree view with deep hierarchy.
  - `showLines`: renders dashed/dotted vertical connector lines and horizontal branching guides between sibling and child nodes.
  - `showCheckBox`: renders interactive tri-state/boolean checkboxes with toggle hit-testing.
  - Node selection, expansion toggle, hover highlight, and hit-testing on chevrons vs item labels.

### 2.4 Vector Icons (`src/ui_icons.h`)
- Added standard vector glyph paths in `VectorIcon::LookupIconPath`:
  - `folder`: desktop folder outline.
  - `file`: document/file outline.
  - `home`: home directory glyph.
  - `code`: code/bracket glyph.
  - `database`: database cylinder glyph.

### 2.5 Layout Parser (`src/layout_parser.cpp`)
- Added XML and JSON tags:
  - `<Breadcrumb>` and `<BreadcrumbItem>` (attributes: `text`, `active`, `href`, `color`, `activeColor`).
  - `<Pagination>` (attributes: `page`, `total`, `pageSize`, `showTotal`, `showJumper`).
  - `<TreeItem>` (attributes: `header`, `icon`, `expanded`, `checked`, `selected`, `value`).
  - `<TreeView>` attributes: `showLines`, `showCheckBox`, `itemHeight`, `indent`.

---

## 3. Demo & Verification

- Created high-fidelity demo layout: `resource/layouts/desktop_navigation_demo.xml` (1280x860).
  - Left panel: Multi-tier Project Explorer with collapsible folders, checkboxes, and connector lines.
  - Main panel: Top breadcrumb navigation trail (`Home / Projects / ai-win-ui / src / ui_views.h`).
  - Data list with bottom pagination control (`< 1 2 3 ... 50 >`, Total 1,000 items).
- Mounted card in `resource/layouts/demo_gallery.xml` under Controls section.
- Executed `run_ci_local.ps1 -SkipBuild`:
  - **7/7 CI regression tests 100% PASS** (Build skipped, Gallery coverage, Skia smoke, Direct2D smoke, Core measure, Yoga measure, Scroll viewer measure).
