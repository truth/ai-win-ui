# 现代基础组件库扩展归档 (Modern Components v2)

- **日期**：2026-11-01
- **分支**：`feature/modern-components-v2`
- **状态**：已完成并合入 `master`

## 1. 背景与目标

在完成 Visual Rendering v2（高斯软阴影、多级字重排版、矢量图标及平滑曲线）后，`ai-win-ui` 在构建现代桌面应用与 SaaS 看板时，仍需频繁使用基础 `<Panel>` + `<Label>` 拼装头像、状态徽标、分段切换器、拨动开关、复合搜索框、环形占比图与可折叠/可分栏容器。

本次迭代新增 **12 项现代高频基础组件与通用能力**，在 Skia 与 Direct2D 双后端提供一致的矢量绘制与交互体验。

---

## 2. 新增组件与架构设计

### 2.1 现代看板与高频展示微组件 (`src/ui_widgets.h`)
1. **`<Avatar>` (`Avatar`)**：
   - 支持圆形 (`shape="circle"`, 默认) 与圆角方形 (`shape="rounded"`)；
   - 支持首字母缩写 (`initials="DC"`) 与内置矢量图标回退 (`icon="user"`)；
   - 支持右下角带白边的在线状态指示圆点：`status="online|busy|away|offline"`。
2. **`<Badge>` (`Badge`)**：
   - 支持 6 种语义预设：`variant="success|warning|danger|info|primary|neutral"`；
   - 支持前置状态圆点 (`dot="true"`) 与前置矢量图标 (`icon="trending-up"`)；
   - 根据文本宽度与图标自动测量胶囊宽度。
3. **`<Divider>` (`Divider`)**：
   - 支持水平 (`orientation="horizontal"`) 与垂直 (`orientation="vertical"`)、实线与虚线 (`dashed="true"`)，以及居中嵌入文字 (`text="SEMANTIC BADGES"`)。
4. **`<SegmentedControl>` (`SegmentedControl`)**：
   - 支持 `items="Daily|Weekly|Monthly"`、`selectedIndex` 与 `onChange` 回调，选中项渲染带高斯软阴影的白色高光胶囊。

### 2.2 现代交互与表单控件 (`src/ui_widgets.h`)
5. **`<Switch>` (`Switch`)**：
   - 胶囊拨动开关，支持 `checked`、`label`、`onText`/`offText`、`activeColor`、鼠标点击与键盘 Space/Enter 切换，并映射 `UIA::CheckBoxControlType`。
6. **`<SearchBox>` (`SearchBox`)**：
   - 复合搜索输入框，内置左侧搜索矢量图标 (`icon="search"`)、占位符 (`placeholder`)、非空时右侧一键清除按钮 (`clearable="true"`) 以及空态右侧快捷键徽标 (`shortcut="Ctrl+K"`)。

### 2.3 反馈与状态指示组件 (`src/ui_element.h` & `src/ui_widgets.h`)
7. **基类级通用 `tooltip` 属性 (`UIElement::m_tooltip`)**：
   - 任意控件支持声明 `tooltip="..."`，悬停时在 `UIElement::RenderOverlay` 第二遍绘制通道自动渲染带高斯阴影的深色悬浮气泡。
8. **`<Alert>` (`AlertBanner`)**：
   - 页内嵌状态横幅：`variant="info|success|warning|error"`、`title`、`message`、`closable="true"`，点击右侧关闭图标后自动收起并重排布局。
9. **`<CircularProgress>` (`CircularProgress`)**：
   - 环形进度指示器：通过动态生成 SVG 圆弧指令 (`M ... A ...`) 调用 `IRenderer::DrawSvgPath` 在 Skia 与 Direct2D 双后端绘制圆弧轨道与进度环，中心支持百分比或自定义文本。
10. **`<EmptyState>` (`EmptyState`)**：
    - 标准空数据占位组件（浅色圆形图标底座 + 矢量图标 + 主标题 + 副标题 + 可选操作按钮）。

### 2.4 容器与图表扩展 (`src/ui_containers.h` & `src/ui_chart.h`)
11. **`<Expander>` (`Expander`) & `<SplitView>` (`SplitView`)**：
    - `<Expander>`：可折叠手风琴容器，点击 Header 栏切换展开/收起状态并触发宿主 `UpdateLayout()` 动态重排。
    - `<SplitView>`：支持左右或上下双面板分栏，鼠标拖拽中间分割条可实时调整 `splitRatio` 并重排两侧子面板。
12. **`<Chart>` 饼图与环形图扩展 (`type="pie|donut"`)**：
    - `ChartSeries::Type` 新增 `Pie` 与 `Donut`，支持 `innerRadiusRatio`、`centerValue`、`centerTitle`、`colors` 自定义色板、扇区悬停外扩高亮及 Tooltip 联动。

---

## 3. 验证结果

- **编译构建**：`.\build.ps1` Debug 构建 0 Error / 0 Warning。
- **CI 自动化回归测试**：`.\scripts\run_ci_local.ps1 -SkipBuild` 7/7 项测试 100% PASS。
- **双后端画廊验证**：新增 `resource/layouts/modern_components_v2.xml` 挂载至 `demo_gallery.xml`，并在 Skia 与 Direct2D 双后端通过 Headless Smoke 测试。

