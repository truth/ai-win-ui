# 同花顺金融终端 UI 布局高精对齐、文字截断修复与 K 线图（Candlestick）功能扩展

**日期**: 2026-11-02  
**分支**: `feature/kline-and-layout-align`  
**作者**: Antigravity  

---

## 1. 背景与目标

在初版同花顺金融终端原型中，根据用户真机运行反馈与对比截图，存在以下问题：
1. **上方菜单栏与字体截断/消失**：
   - 顶部菜单行高仅 `22px`，在 Windows DPI 缩放（125%/150%）下文字被严重压扁、重叠与裁剪；
   - 右侧盘口栏与列表行高硬编码为 `height="14"` 或 `height="15"`，DirectWrite 测出的行高边界超出容器高度，导致整行被父容器裁剪或完全不绘制，从而产生大面积黑屏留白；
2. **K 线图需求**：
   - 核心图表控件仅支持分时走势图（Time-Sharing），需扩展支持 **K 线图（Candlestick）** 模式；
   - 需支持：红绿蜡烛实体柱、上下影线、MA5（白）、MA10（黄）、MA20（紫）三条均线、成交量副图、完整十字光标与悬停 OHLCV 信息窗；
3. **数据丰富度**：
   - 用户反馈很多数据区域没有数据显示，需提供丰富的全套真实 Mock 数据，填满盘口深度网格、个股贡献度榜单与短线精灵逐笔流。

---

## 2. 核心架构与技术实现

### 2.1 FinancialChart 升级支持 K 线模式 (`src/ui_financial_chart.h`)
- **双模架构**：引入 `FinancialChartMode::TimeSharing` 与 `FinancialChartMode::Candlestick`；
- **蜡烛图绘制引擎**：
  - 计算全局最高最低价 `candleMinPrice` 与 `candleMaxPrice`，留出 6% 垂直呼吸空间；
  - 绘制细线影线（贯穿 High-Low）与实体蜡烛（Open-Close），阳线实心红，阴线实心绿；
  - 计算并绘制 MA5（纯白 1.2px）、MA10（亮黄 1.2px）、MA20（紫兰 1.2px）多段平滑折线；
  - 副图自适应渲染红绿量柱，底部显示采样日期（如 `01/05`, `02/10`, `03/15`, `04/20` 等）；
  - K 线十字光标准星定位，浮动显示收盘价与日期徽章，并在顶部 Header 输出完整 OHLCV 详情。

### 2.2 模拟行情引擎与 Mock 数据扩展 (`src/mock_market_engine.h/cpp`)
- `GenerateCandles(int count = 90)`：基于几何布朗运动与均值回归算法生成 90 根日 K 线，精确计算真实 MA5、MA10、MA20，并将最新一根 K 线对齐至大盘实时最新价；
- `MarketSnapshot` 扩充 16 项大盘盘口指标：委比、委差、委买量、委卖量、外盘、内盘、市盈(动)、市净率、换手率、两市成交额、上涨/平盘/下跌家数等；
- 贡献度排行榜增加至 16 支权重股，短线精灵增加至 16+ 条逐笔交易推流。

### 2.3 布局与字体渲染重构 (`resource/layouts/stock_quotes.xml`)
- **顶部菜单条（Menu Bar）**：高度提升至 `28px`，内边距 `0, 10px`，间距 `10px`，字号 `12px`，消除所有文字截断；
- **第二层工具栏**：高度 `30px`，按钮 `22px`，同花顺红绿高对比度；
- **第三层周期切换栏**：绑定 `btn_period_timeshare`（分时）、`btn_period_day_k`（日K）、`btn_period_week_k`（周K）、`btn_period_month_k`（月K），支持一键切换模式；
- **右侧盘口看板**：行高安全调整为 `19px`（行距 `1px`，文字 `11px`，`verticalAlign="center"`），10 行双列指标全部清晰填满；
- **个股贡献度与短线精灵**：行高给足 `18px`，字字清晰，彻底杜绝空白与黑屏。

### 2.4 独立终端应用交互 (`samples/stock_terminal/main.cpp`)
- 响应 `btn_period_timeshare`、`btn_period_day_k`、`btn_period_week_k`、`btn_period_month_k` 按钮事件；
- 分时图与 K 线图之间无缝热切换，分时模式下继续接受 500ms 定时器行情 Tick 动态跳动。

---

## 3. 验证结果

- **本地编译**：`build.ps1` 编译通过，0 警告，0 错误；
- **自动化测试**：`run_ci_local.ps1 -SkipBuild` **7/7 100% PASS**（包含 Skia + D2D Headless Smoke 及 3 项 Golden 回归测试）；
- **真机运行**：`stock_terminal.exe` 成功启动，菜单清晰无截断，右侧盘口数据饱满，K线图流畅展示并支持周期一键切换。
