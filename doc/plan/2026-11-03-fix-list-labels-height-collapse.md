# 设计归档：修复列表文字高度塌陷与 Panel 内边距安全保护机制

- **日期**：2026-11-03
- **分支**：`feature/fix-list-labels-height-collapse` -> `master`
- **问题**：同花顺金融终端右侧盘口核心指标双列 Grid、个股贡献度列表、短线精灵逐笔推流列表的 Label 文字全部无法显示。

---

## 1. 根本原因深入排查与分析 (Root Cause Analysis)

### 1.1 默认属性与紧凑容器的冲突
- 在 `src/ui_panels.h` 中，`Panel` 的默认属性为 `padding{8, 8, 8, 8}`。
- 在标准 1.25x DPI 缩放下，上下内边距各缩放为 10px，合计占据 20px 垂直高度。
- 右侧 3 处核心列表中的 28 个行容器定义如下：
  - 盘口核心指标 10 行：`<Panel direction="row" height="19" alignItems="center">`
  - 个股贡献度 8 行：`<Panel direction="row" height="18" alignItems="center">`
  - 短线精灵 10 行：`<Panel direction="row" height="18" alignItems="center">`
- 这些行容器在 XML 中均未显式声明 `padding`，从而继承了默认的 20px 垂直内边距。

### 1.2 Yoga 布局约束导致叶子节点下溢塌陷
- 行容器的高为 19px（DPI 缩放后为 23.75px），扣除默认内边距上下各 10px 后：
  $$\text{contentHeight} = 23.75 - 20 = 3.75\text{px}$$
- Yoga 在叶子节点度量阶段传入 `YGMeasureModeAtMost`，要求内部 `Label` 叶子节点的测量高度上限为 3.75px。
- `MeasureLeafNode` 在 `YGMeasureModeAtMost` 下执行 `measured.height = std::min(measured.height, height)`，强制将子元素的高度截断为 `h = 3.0px`。

### 1.3 渲染器文本垂直裁剪
- 11pt 字体渲染所需高度约为 14~16px。
- 当 `m_bounds` 的高度被严重压缩至 3.0px 时，DirectWrite 与 Skia 在 3px 矩形内垂直居中排版文字，字形由于上下各被截掉 6~7px，视觉上呈现完全隐形/消失。

---

## 2. 解决方案与架构实施

### 2.1 布局 XML 显式声明
在 `resource/layouts/stock_quotes.xml` 中：
- 为盘口指标双列 Grid 的 10 个行容器显式添加 `padding="0,0,0,0"`。
- 为个股贡献度列表的 8 个行容器显式添加 `padding="0,0,0,0"`。
- 为短线精灵逐笔推流列表的 10 个行容器显式添加 `padding="0,0,0,0"`。
- 确保行内容可用高度达满高 23px，Label 可以在 18-19px 高度内垂直居中排版。

### 2.2 C++ 解析器安全兜底与 Thickness 扩展
在 `src/layout_parser.cpp` 中：
- **安全兜底机制**：解析 XML/JSON `<Panel>` 时，若未显式指定 `padding` 且设置了固定高度且 `height <= (padding.top + padding.bottom)` 或 `height <= 24.0f` 时，自动将内边距置 0，彻底根除类似单行紧凑容器由于默认 padding 挤压导致内容高度下溢的隐患。
- **多参数解析放宽**：放宽 `margin`、`padding` 的数组解析限制，完整支持 1 个值（四边等距）、2 个值（水平, 垂直）、4 个值（Left, Top, Right, Bottom）语法。

### 2.3 UIElement 属性规范化
在 `src/ui_element.h` 中：
- 补充标准 accessor 方法：
  ```cpp
  float FixedWidth() const { return m_fixedWidth; }
  float FixedHeight() const { return m_fixedHeight; }
  ```

---

## 3. 验证指标与回归测试

1. **几何度量转储对比 (`scripts/run_measure_dump.ps1`)**：
   - 修复前：`grid_weibi` 的 `w = 78, h = 3`，所有列表文本 `h` 仅 3px。
   - 修复后：`grid_weibi` 的 `w = 78, h = 23`，所有列表行内的所有 Label 高度全部恢复为满高 23px。
2. **自动化回归测试**：
   - 执行 `.\scripts\run_ci_local.ps1 -SkipBuild`，7 项 CI 回归测试 **100% PASS**（包含 gallery coverage、headless smoke 双后端、3 套 golden measure 对比）。
3. **真实运行视觉核验**：
   - 启动 `stock_terminal.exe`，盘口指标、个股贡献度、短线精灵全部清晰居中饱满显示，500ms 动态推流跳动正常。
