# 实施计划 - 修复同花顺金融终端右侧列表文字无法显示（高度塌陷）

## 问题现象与根因定位

### 1. 现象描述
在运行 `stock_terminal.exe` 时，截图红框标注的右侧 3 处核心列表文字无法正常显示：
- **红框 1**：盘口核心指标双列 Grid（委比、委差、昨收、今开、涨跌、最高、最低、振幅、总手、金额等 10 行）。
- **红框 2**：个股贡献度列表（贵州茅台、招商银行、兴业银行等 8 支权重股的涨跌、现价、贡献）。
- **红框 3**：短线精灵逐笔推流列表（14:58:33 大笔买入 171手等 10 行异动列表）。

### 2. 根因剖析 (Root Cause)
通过 `scripts/run_measure_dump.ps1` 转储 UI 几何排版数据并分析 `scratch/stock_dump.ndjson` 查明：
1. **默认内边距挤压**：
   - C++ 中 `Panel` 的默认属性为 `padding{8, 8, 8, 8}`（上下各 8px，DPI 1.25 缩放下为 10px + 10px = 20px）。
   - 右侧列表中的 28 个行容器（`<Panel direction="row" height="19" alignItems="center">` 及 `height="18"`）在 XML 中均**未显式声明 `padding`**。
2. **Yoga 内容高度下溢**：
   - 行容器的高为 19px（DPI 1.25 缩放下为 23.75px），扣除默认内边距上下各 10px 后，`contentHeight = 23.75 - 20 = 3.75px`！
   - Yoga 在叶子节点测量阶段传入 `YGMeasureModeAtMost`，要求 `Label` 的最大高度不得超过 3.75px。
   - `MeasureLeafNode` 将子元素的高度硬截断为 `h = 3.0px`。
3. **渲染器文本垂直裁剪**：
   - 11pt 字体渲染需要约 15px 垂直空间。当其包围盒 `m_bounds` 高度被压缩至 3px 时，DirectWrite 与 Skia 无法在 3px 区域内正常居中绘制完整字形，导致文字视觉上完全消失。

---

## 修改范围与技术方案

### 1. XML 布局层（核心修复）
- **文件**：`resource/layouts/stock_quotes.xml`
- **改动**：
  - 为盘口指标双列 Grid 的 10 个行容器显式添加 `padding="0,0,0,0"`。
  - 为个股贡献度列表的 8 个行容器显式添加 `padding="0,0,0,0"`。
  - 为短线精灵逐笔推流列表的 10 个行容器显式添加 `padding="0,0,0,0"`。
  - 确保行内容高度饱满，Label 可以在 18-19px 高度内垂直居中排版。

### 2. C++ 解析与布局引擎层（防御性兜底）
- **文件**：`src/layout_parser.cpp`
- **改动**：
  - 增强 XML/JSON 的 `padding` 属性解析，放宽 `values.size() == 4` 限制，完整支持 1、2、4 个数值（如 `padding="0"`、`padding="6,2"`）。
  - 增加紧凑行安全兜底保护：当 `<Panel>` 设置了固定高度 `height` 且 `height <= 24.0f` 时，若未显式指定 `padding`，自动将上下内边距安全置 0，杜绝单行紧凑容器由于默认 padding 导致内部叶子节点被挤压下溢的结构性隐患。

### 3. 验证与回归测试
- 运行 `scripts/run_measure_dump.ps1` 转储验证：
  - 验证 `grid_weibi` 及列表内所有 Label 的 `h` 从 3.0px 恢复为正常的高度（15~19px）。
- 运行 `build.ps1` 校验编译通过。
- 运行 `scripts/run_ci_local.ps1 -SkipBuild`，确保 7 项 CI 回归测试 **100% PASS**。
- 启动 `stock_terminal.exe`，直观核验 3 个红框区域内的所有列表文字完整、饱满、清晰显示。

---

## 交付与归档计划
1. 完成代码修改并执行本地 CI 全绿校验；
2. 合并 `feature/fix-list-labels-height-collapse` 分支至 `master`；
3. 更新 `DEVELOPMENT_LOG.md` 顶部开发日志；
4. 归档设计文档 `doc/plan/2026-11-03-fix-list-labels-height-collapse.md`，更新 `doc/README.md`。
