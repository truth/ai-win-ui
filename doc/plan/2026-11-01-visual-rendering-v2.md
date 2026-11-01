# UI 视觉渲染与绘制表现力升级归档 (Visual Rendering v2)

- **日期**：2026-11-01
- **分支**：`feature/visual-rendering-v2`
- **状态**：已完成并合入 `master`

## 1. 背景与问题根因

通过对 `ai-win-ui` 渲染管线与组件实现的深入排查，制约现代高保真 UI 绘制质感与还原度的 4 大核心根因如下：

1. **盒模型原语简化（`box_decoration.cpp` / `layout_parser.cpp`）**：
   - 虽然 `CornerRadius` 与 `BorderSpec` 定义了四角与四边字段，但 `Panel` 仅保存单值 `float cornerRadius`，且 `DrawBoxDecoration` 直接取 `MaxRadius()` 和 `MaxBorderWidth()` 绘制整圈边框，无法绘制单边分割线（如 `border="0,0,0,1"` / `borderBottom="1,#EEF2F7"`）或非对称圆角（如 `cornerRadius="20,0,0,20"`）。
   - XML 解析器此前未解析 `Panel` 上的 `border="1,#EAEFF5"`、`borderBottom`、`borderTop` 以及 `marginLeft/Right/Top/Bottom` 简写属性，导致布局中声明的微边框与外边距被静默忽略。
   - `FillSoftShadow` 在 Skia 后端使用多层同心圆角矩形近似，未使用原生 `SkMaskFilter::MakeBlur` 高斯模糊。
2. **字体排版与字重层级缺失（`ui_panels.h` / `layout_parser.cpp` / `skia_text_layout.cpp`）**：
   - `<Label>` 此前仅解析 `text`、`fontSize`、`color`、`ellipsis`，忽略了 XML 中的 `bold="true"`、`fontWeight`、`fontFamily`、`lineHeight` 与对齐属性，导致所有标题与大号数字均以普通 400 字重渲染。
   - 底层 `TextRenderOptions` 仅支持布尔 `bold`（400/700 两档），缺乏现代 UI 必需的 `Medium(500)`、`SemiBold(600)`、`ExtraBold(800)` 字重以及 `fontFamily`、`lineHeight` 控制；Skia 表面开启了 `kUseDeviceIndependentFonts_Flag` 导致桌面小字号边缘发虚。
3. **矢量图标与内联路径原语缺失（`ui_media.h`）**：
   - 缺少内联 `<Path data="M..." />` 与内置现代矢量图标库 `<Icon name="search|bell|calendar|..." />`，导致布局文件被迫使用粗糙的 Unicode 字符代替图标，且 Direct2D 后端下 `SvgIcon` 只能显示占位框。
4. **图表绘制原语生硬（`ui_chart.h`）**：
   - 折线图与面积图仅支持折角直线（`DrawPolyline`）与纯色半透明填充（`FillPolygon`），缺少 Catmull-Rom 平滑曲线（`smooth="true"`）、面积图垂直线性渐变填充（`FillPolygonLinearGradient`）、圆角胶囊柱状图（`barRadius`）、单柱高亮与悬浮胶囊标签（`highlightIndex` / `highlightColor`）、柱状背景槽（`barTrackColor`）以及虚线网格（`DrawDashedLine`）。

---

## 2. 核心架构与实现方案

### 2.1 盒模型与阴影 (`src/graphics_types.h`, `src/box_decoration.*`, `src/renderer_skia.cpp`, `src/renderer.cpp`)
- **真高斯模糊软阴影**：Skia 后端 `FillSoftShadow` 接入 `SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, sigma)` 原生高斯模糊；Direct2D 后端采用 12 层高斯衰减权重曲线平滑过渡。
- **四角独立圆角 (`CornerRadius`)**：`IRenderer` 新增 `FillRRect` 与 `DrawRRect`，Skia 使用 `SkRRect::setRectRadii`，Direct2D 使用 `ID2D1PathGeometry` 四角独立圆弧段。
- **四边独立边框与 XML 简写解析**：`DrawBoxDecoration` 在 `!d.border.IsUniform()` 时按 `top/bottom/left/right` 独立绘制单边线条；`layout_parser.cpp` 完整支持 `border="1,#EAEFF5"`、`borderBottom="1,#EEF2F7"`、`borderTop`、`borderLeft`、`borderRight` 以及 `marginLeft/Top/Right/Bottom`、`paddingLeft/Top/Right/Bottom`。

### 2.2 字体排版与清晰度 (`src/renderer.h`, `src/skia_font_shared.h`, `src/skia_text_layout.*`, `src/ui_panels.h`)
- **多级字重与字体族**：`TextRenderOptions` 新增 `fontWeight`（100..900，支持 `medium`/`semibold`/`bold` 语义名及数值）、`fontFamily`、`letterSpacing`、`lineHeightFactor`，Skia（`SkFontStyle`）与 DirectWrite（`DWRITE_FONT_WEIGHT`）双后端同步支持。
- **桌面级字形锐度**：移除 Skia `SkSurfaceProps` 中的 `kUseDeviceIndependentFonts_Flag`，恢复桌面端像素级 Hinting 与抗锯齿锐度。
- **`<Label>` 排版属性补齐**：支持 `bold`、`italic`、`fontWeight`、`fontFamily`、`lineHeight`、`align`/`horizontalAlign`、`verticalAlign`。

### 2.3 内置矢量图标库与内联路径 (`src/ui_icons.h`, `src/ui_media.h`)
- **`<Icon>` 控件 (`VectorIcon`)**：内置 24+ 常用现代 Lucide 风格矢量图标（`search`、`bell`、`calendar`、`user`、`users`、`check`、`plus`、`chevron-down`、`chevron-right`、`dashboard`、`activity`、`file-text`、`pill`、`credit-card`、`message-square`、`settings`、`help-circle`、`trending-up`、`clock`、`filter`、`heart`、`star`、`more-horizontal`、`arrow-up-right`），支持 `size`、`color`、`fill`、`strokeWidth`。
- **`<Path>` 控件 (`VectorPath`)**：支持任意内联 SVG Path 数据（`data="M..." viewBox="0,0,24,24"`），Skia 使用 `SkParsePath::FromSVGString`，Direct2D 内置轻量 SVG Path 指令解析器（支持 `M/L/H/V/C/A/Z` 绝对与相对指令并自动转换椭圆弧至 `D2D1::ArcSegment`），实现双后端一致渲染。

### 2.4 图表平滑曲线与渐变精修 (`src/ui_chart.h`)
- **Catmull-Rom 丝滑曲线 (`smooth="true"`)**：对折线图与面积图关键点自动执行 Catmull-Rom 样条插值。
- **面积垂直线性渐变填充 (`areaGradient="true"`)**：通过 `IRenderer::FillPolygonLinearGradient` 实现从主色高光到底部透明的自然垂直渐变。
- **柱状图精修**：支持圆角柱（`barRadius`）、背景轨道槽（`barTrackColor`）、指定索引高亮柱（`highlightIndex` / `highlightColor`）及柱顶悬浮胶囊数值标签、柔和虚线水平网格（`dashedGrid="true"`）与无边框坐标轴（`showAxes="false"`）。

---

## 3. 验证结果

- **编译验证**：`.\build.ps1` Debug 构建 0 Error / 0 Warning。
- **CI 自动化回归测试**：`.\scripts\run_ci_local.ps1 -SkipBuild` 7/7 项测试（Gallery Coverage、Skia Smoke、Direct2D Smoke、Core Golden、Yoga Golden、ScrollViewer Golden）100% PASS。
- **双后端看板验证**：`medlio_dashboard.xml` 在 Skia 与 Direct2D 后端均無错误渲染并正常退出（`exit=0`）。

