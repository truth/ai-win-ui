# Flush Stock Terminal & FinancialChart Component (v1)

**日期**：2026-11-02  
**状态**：已落地 (Merged)  
**分支**：`feature/stock-terminal` -> `master`  
**关联目标**：`samples/stock_terminal` (`build/Debug/stock_terminal.exe`)

---

## 1. 背景与目标

为了验证 `ai-win-ui` 在复杂高密度、高频刷新场景下的工程承载能力，以经典的同花顺（Flush）股票看盘软件上证指数分时工作台为参考系，构建了一个高性能金融看盘终端原型。

主要目标：
1. **金融复合图表控件 (`FinancialChart`)**：支持上证分时白线（加权点位）、黄线（领先指数）、昨收中轴基准、双 Y 轴自适应价格算法、红绿分时成交量柱以及大盘指标曲线。
2. **高频模拟行情引擎 (`MockMarketEngine`)**：支持高精度定时 Tick 发生、盘口快照、动态异动推流与红绿变色动画。
3. **键盘精灵 (`KeyboardGenie`)**：支持拼音简拼/证券代码即时过滤匹配与联想弹窗。
4. **双主题支持**：提供经典同花顺黑底主题 (`theme_stock_classic.json`) 与现代质感暗黑主题 (`theme_stock_modern.json`)，支持按 `T` 键热切换。
5. **XML 声明式布局**：基于 `resource/layouts/stock_quotes.xml`，完整还原顶部双层工具栏、左侧竖向 Dock、主图、资讯列表、右侧指标与逐笔面板、底部状态栏。

---

## 2. 核心架构与技术实现

### 2.1 FinancialChart 控件设计 (`src/ui_financial_chart.h`)
- 继承自 `ai_win_ui::UIElement`，在 XML 解析器中注册为 `<FinancialChart>`。
- 内部划分三大区域：
  - `PricePane`（占 58% 高度）：分时走势主图，包含白黄双曲线、昨收参考虚线及网格分割线。
  - `VolumePane`（占 20% 高度）：红绿成交量柱状图。
  - `IndicatorPane`（占 20% 高度）：大盘涨跌停指标线 / 多空指标。
- 动态自适应对称振幅算法：以 `preClose` 为基准中心，计算最大绝对偏差并预留 5% 缓冲区，确保中轴始终对齐 0.00% 涨跌幅。
- 鼠标十字准星 (`Crosshair`)：重写 `OnMouseMove` 与 `OnMouseLeave`，高频追踪鼠标位置并在 PricePane/VolumePane/IndicatorPane 贯穿绘制虚线，附带左轴价格、右轴涨跌幅、底轴时间浮动标签以及顶部信息概览条。

### 2.2 模拟行情引擎 (`src/mock_market_engine.h/cpp`)
- 模拟生成 09:30 至 15:00 共 241 个分时交易点，并在 500ms 定时器驱动下实时跳动最新 Tick。
- 产生逐笔成交异动（大笔买入、大笔卖出、急速拉升），实时滚动推送到看板。

### 2.3 宿主 API 扩展 (`include/ai_win_ui/host.h` & `src/ui_host.cpp`)
新增对金融图表控件的专用控制接口：
```cpp
bool ResetFinancialChart(const char* name, float preClose);
bool AppendFinancialChartPoint(const char* name, float price, float leadPrice, float volume, bool isUp, const char* timeStr);
bool SetFinancialChartTheme(const char* name, bool modern);
```

---

## 3. 验证与回归测试

1. **编译构建**：通过 `.\build.ps1`，成功编译出 `stock_terminal.exe`。
2. **自动化测试**：运行 `.\scripts\run_ci_local.ps1 -SkipBuild`，7 项自动化回归测试全部保持 **100% PASS**（包含 Skia / Direct2D 双后端 Headless Smoke 及三项 Golden 测量）。
