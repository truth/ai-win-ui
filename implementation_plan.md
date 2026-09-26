# 实施计划 (Implementation Plan)

## 目标 (Objective)
执行 Wave5 执行计划中的步骤 2：
- 编写 `.cursor/rules`，指导 AI 生成正确的 Flex 属性和 Chrome 自定义层叠布局。
- 增加 `flex_chrome_validation` 画廊页面，用于综合测试 Flex 对齐/包裹以及 Layered Chrome 拖拽交互。
- 确保所有的修改对齐并满足最新的布局规范，并保持 CI 回归测试通过。

## 修改内容与技术方案 (Changes & Technical Solutions)

1. **建立 Cursor AI 规则文件**
   - **文件路径**: `.cursor/rules/ai_win_ui_layout.mdc`
   - **内容要点**:
     - 强制采用 `Panel` 的 `direction`、`wrap`、`alignItems` 和 `justifyContent` 等 Flexbox 语法。
     - 指定 `chrome="layered"` 时，必须使用 `background="transparent"` 以及提供一个带 `hitTest="caption"` 的 `ShapePanel` 或 `Panel` 以支持拖拽。
     - 规范 `$style.xxx` 与 `theme` 令牌的引用，并指明 XML 与 JSON 在样式嵌套上的区别。

2. **新增综合测试布局画廊**
   - **文件路径**: `resource/layouts/flex_chrome_validation.xml`
   - **内容要点**:
     - 根节点使用 `chrome="layered"` 和透明背景。
     - 内部使用圆角 `Panel` 或者 `ShapePanel` 作为主体。
     - 测试 Flex 的 `wrap="wrap"` 多行包裹，以及 `justifyContent`（如 `space-between`）和 `alignItems`。
     - 增加关闭按钮（`onClick="windowClose"`）和自定义标题拖拽区域（`hitTest="caption"`）。

3. **集成至主画廊**
   - **文件路径**: `resource/layouts/demo_gallery.xml`
   - **修改内容**:
     - 在 "Window Chrome" 网格区域，新增一个 `flex_chrome_validation` 的入口。
     - 配置 `onClick="openDemo:layouts/flex_chrome_validation.xml|layered|1000x700"`。

## 验证指标 (Verification Metrics)
- `flex_chrome_validation.xml` 在加载时展现正确的层叠不规则窗口（Layered Chrome）。
- 在标题栏区域可以正常拖拽窗口。
- 窗口内多行 Flex 元素正确包裹与对齐。
- `.\scripts\run_ci_local.ps1 -SkipBuild` 的 7 项 CI 回归测试 **100% PASS**。

