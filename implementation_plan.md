# 实施计划 - 同花顺金融终端 UI 对齐、文字截断修复与 K 线图功能实现

## 1. 问题复盘与需求分析

在用户提供的最新运行截图（`media_1793629935816_1b8fd61b.png`）中，发现以下关键缺陷需要彻底解决：
1. **上方菜单与字体截断/消失**：
   - 顶部菜单行高仅 `22px`，在 DPI 缩放（125%/150%）下文字被严重压扁、重叠与裁剪。
   - 右侧盘口栏与列表大量行高被写死为 `height="14"` 或 `height="15"`，导致 DirectWrite / Direct2D 测出的文本边界超过容器高度，整行被父容器 clip 或完全不绘制，导致右侧出现大片黑屏留白。
   - 顶部工具条密度与参考原图（`media_1793625014431_f9d54b2e.png`）风格不匹配。
2. **新增 K 线图支持**：
   - 核心图表控件当前仅支持分时图（Time-Sharing），需扩展支持 **K 线图（Candlestick）** 模式。
   - K 线图需支持：红绿蜡烛实体柱（阳线红实心、阴线绿实心）、上下影线、MA5（白）、MA10（黄）、MA20（紫）三条均线、成交量副图、完整十字光标与悬停 OHLCV 信息窗。
   - 支持通过顶部周期按钮（“分时” vs “日K/周K/月K”）或左侧“K线”按钮进行**分时/K线模式一键无缝切换**。

---

## 2. 总体设计方案与技术架构

### 2.1 UI 布局与字体渲染重构 (`stock_quotes.xml`)
- **顶部菜单条（Menu Bar）**：
  - 高度提升至 `28px`，内边距 `0, 10px`，字号 `12px`，背景为同花顺经典深灰 `#141414`，菜单项增加间距 `10px`，彻底杜绝文字截断。
- **第二层工具栏（Toolbar）**：
  - 高度设为 `30px`，按钮高度 `22px`，增加边框与同花顺红绿高对比色彩。
- **第三层周期切换栏（Period Bar）**：
  - 增加清晰的按钮点击 action 回调：`btn_period_timeshare`（分时）、`btn_period_day_k`（日K）、`btn_period_week_k`（周K）、`btn_period_month_k`（月K）。
- **右侧盘口看板与列表**：
  - 彻底移除 `height="14/15"` 的危险硬编码，统一行高为 `19px`（行间距 `1px`），文字 `11px`，垂直居中 `verticalAlign="center"`。
  - 右侧盘口重新梳理为标准两列双值（委比、最新、涨跌、涨幅、振幅、最高、最低、现手、量比、总手、金额、两市额、上涨家数、下跌家数），字字清晰，杜绝空白。
  - “个股贡献度”与“短线精灵”列表行高调整为 `18px`，增加文本水平对齐和颜色高亮。

### 2.2 FinancialChart 升级支持 K 线模式 (`src/ui_financial_chart.h`)
- 引入枚举 `FinancialChartMode { TimeSharing, Candlestick }`。
- 引入 `CandleBar` 结构：
  ```cpp
  struct CandleBar {
      std::string date;      // "2026-03-12"
      float open = 0.0f;
      float high = 0.0f;
      float low = 0.0f;
      float close = 0.0f;
      float volume = 0.0f;
      float ma5 = 0.0f;
      float ma10 = 0.0f;
      float ma20 = 0.0f;
  };
  ```
- 绘制逻辑划分：
  - `Render()` 根据 `m_mode` 分流调用 `DrawTimeSharingPane()` 或 `DrawCandlestickPane()`。
  - **K线主图**：绘制蜡烛实体（收盘 >= 开盘 为红，收盘 < 开盘 为绿）、上下细影线（DrawLine）、MA5/MA10/MA20 多段折线。
  - **副图**：根据当日涨跌显示红/绿量柱。
  - **K线十字光标**：十字交叉定位到当前选中蜡烛，顶部 Header 显示：`日期 2026-xx-xx  开: 3512.10  高: 3540.20  低: 3505.30  收: 3534.32  涨跌: +22.22 (+0.63%)  MA5: 3520.12  MA10: 3515.80  MA20: 3498.40`。

### 2.3 宿主与模拟引擎扩展 (`src/ui_host.cpp`, `src/mock_market_engine.h/cpp`)
- `MockMarketEngine` 增加 120 根历史日 K 线数据生成器（自适应计算真实 MA5、MA10、MA20 及波动走势）。
- `Host` 增加 `SetFinancialChartMode`、`SetFinancialChartCandles` 接口。
- `stock_terminal/main.cpp` 支持周期按钮事件响应，点击“日/周/月/K线”即刻切换并加载 K 线数据，点击“分时”无缝切回动态跳动的分时数据。

---

## 3. 实施步骤与分工

| 步骤 | 内容 | 文件 |
| :--- | :--- | :--- |
| **Step 1** | 在 `src/ui_financial_chart.h` 中实现 K 线图数据结构、绘制引擎（蜡烛图、均线、量柱、K线十字光标）与模式切换 | `src/ui_financial_chart.h` |
| **Step 2** | 在 `src/mock_market_engine.h/cpp` 中实现日 K 线生成与均线计算逻辑 | `src/mock_market_engine.h`, `src/mock_market_engine.cpp` |
| **Step 3** | 在 `src/ui_host.cpp` 与 `include/ai_win_ui/host.h` 增加 K 线数据灌入与模式切换接口 | `src/ui_host.cpp`, `include/ai_win_ui/host.h` |
| **Step 4** | 重构 `resource/layouts/stock_quotes.xml`，调整行高与间距，消除文本截断，对齐参考图配色与紧凑度，绑定分时与K线按钮 action | `resource/layouts/stock_quotes.xml` |
| **Step 5** | 在 `samples/stock_terminal/main.cpp` 中实现分时与 K 线模式切换响应逻辑 | `samples/stock_terminal/main.cpp` |
| **Step 6** | 执行本地构建 `.\build.ps1` 校验编译通过 | - |
| **Step 7** | 运行自动化回归测试 `.\scripts\run_ci_local.ps1 -SkipBuild`，确保 7 项测试 100% PASS | - |
| **Step 8** | 启动 `stock_terminal.exe` 验证渲染效果与周期切换，确认字体无截断，K线图流畅展示 | - |

---

## 4. 验证指标与验收标准

1. **界面文字完整性**：
   - 顶部菜单项（“同花顺”、“系统”、“报价”、“行情”等）清晰完整，无挤压重叠。
   - 右侧 16 行核心指标盘口（委比、涨跌、昨收、今开、最高、最低、总手、两市额等）全部清晰显示，数值高亮。
   - 个股贡献度与短线精灵列表行文字不被下边缘裁剪。
2. **K 线图功能**：
   - 点击“日”、“周”、“月”或左侧“K线”按钮，图表立即切换为红绿 K 线蜡烛图及 MA5/MA10/MA20 均线图。
   - 鼠标悬停显示精确的 K 线十字光标与 OHLCV 信息框。
   - 点击“分时”按钮，瞬间切回分时走势图并继续接受 500ms 行情 Tick 跳动。
3. **CI 回归**：
   - 本地 CI 7 项测试保持 **100% PASS**。
