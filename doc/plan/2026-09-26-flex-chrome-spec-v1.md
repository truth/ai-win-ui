# Wave5 Flex + Chrome 规范与画廊验证 (2026-09-26)

本文档归档了 Wave5 执行计划中，关于 Flex 布局规范化、Cursor AI 开发规则、以及自定义层叠窗口（Layered Chrome）的测试画廊设计。

---

## 1. Cursor AI 布局规则 (.cursor/rules)

为了确保 AI 生成代码的质量和与本项目专有的 XML/JSON DSL 对齐，我们添加了 `.cursor/rules/ai_win_ui_layout.mdc`。

### 规则核心要点
1. **Flexbox 作为唯一布局模型**：禁止尝试使用绝对定位（Absolute Positioning）。所有的排版必须通过 `Panel` 的 `direction`, `wrap`, `justifyContent`, `alignItems`，以及子元素的 `flexGrow` 和 `alignSelf` 实现。
2. **Layered Chrome 的要求**：当根节点设置了 `chrome="layered"` 时，必须使用 `background="transparent"` 以确保系统的 DWM 可以透出底层桌面。不规则窗口的点击实体必须是一个带有可见背景的内部 `Panel` 或 `ShapePanel`，并通过 `hitTest="caption"` 提供拖拽功能。
3. **Style 与 Theme 的区分**：确保生成的代码能正确使用 `$style.xxx`（引用外部 JSON 样式）和 `$color.xxx`（引用 Theme Token），不混用这两种机制。

---

## 2. Flex Chrome Validation 画廊

为方便人工与自动化进行回归测试，在 `resource/layouts/flex_chrome_validation.xml` 中综合了最容易出问题的几个特性：

### 测试点
- **透明的 Layered Window**：窗口无系统边框，背景完全透明。
- **自定义拖拽区**：内部主 `Panel` 包含圆角与阴影（`shadowBlur="32"`），并通过 `hitTest="caption"` 设置为可拖拽。
- **点击透传**：在 caption 区域中嵌入 `hitTest="client"` 的关闭按钮，确保点击事件能正确路由。
- **Flex Wrap**：包含多个按钮子项的子 `Panel` 开启了 `wrap="wrap"`，用于验证容器在窗口大小调整或固定尺寸下是否能正确进行多行折叠计算。

通过将此画廊集成在 `demo_gallery.xml` 的 `Window Chrome` 网格中，开发者可以直观地确认 Flex 计算与窗体事件引擎的兼容性。
