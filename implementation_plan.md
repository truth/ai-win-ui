# Implementation Plan: 同花顺风格金融看盘工作台 (Stock Terminal Prototype)

## 1. 目标 (Goals)
在 `ai-win-ui` 体系下，构建一个高保真还原同花顺风格的上证指数分时工作台原型 (`samples/stock_terminal`)。
系统涵盖高密度分时图表复合控件 (`FinancialChart`)、右侧盘口与高频逐笔异动、键盘精灵检索弹窗、内置模拟行情推流引擎 (`MockMarketEngine`) 以及双主题切换（经典黑底模式 & 现代暗黑模式）。

---

## 2. 总体技术架构与方案

### 2.1 模块分层与实现策略
1. **金融复合图表控件 (`FinancialChart`)**：
   - 继承自 `ai_win_ui::Element`，并在 `LayoutParser` 中注册 `<financial-chart>` 标签。
   - 内部三区域统一划分与绘制：
     - `PricePane`（顶部 60%）：白线（加权点位）、黄线（领先不加权点位）、0.00% 昨收参考中轴虚线、双 Y 轴（左点位、右百分比涨跌幅）、时间 X 轴标尺（09:30、10:30、11:30/13:00、14:00、15:00）。
     - `VolumePane`（中部 20%）：红绿成交量柱状图、成交量 Y 轴刻度。
     - `IndicatorPane`（底部 20%）：大盘涨跌停指标线 / 多空曲线。
   - **交互特性**：
     - 鼠标移动追踪（Crosshair 十字光标），并显示悬浮信息提示（价格、涨跌、成交量、时间）。
     - 支持局部 `InvalidateRect`，高频绘制避免全树重新 Layout。
2. **模拟行情推流引擎 (`MockMarketEngine`)**：
   - 生成符合真实 A 股分时波动逻辑的模拟数据（开盘 09:30 至收盘 15:00 对应 240 个数据点）。
   - 提供定时器 Tick 驱动（默认 500ms 刷新），实时追加点位并通知 UI 重绘。
   - 生成盘口五档/十档明细与逐笔成交异动（大笔买入/大笔卖出），支持红绿闪烁。
3. **右侧盘口与排行榜看板 (`MarketDepthPanel` & `VirtualListBox`)**：
   - 上半部分：指数关键指标概览（昨收、今开、现价、涨跌幅、委比、成交额、涨跌家数比）。
   - 中间部分：个股贡献度排行榜。
   - 下半部分：短线精灵逐笔异动流（带时间、成交量、方向以及高亮背景动画）。
4. **键盘精灵 (`KeyboardGenie`)**：
   - 监听宿主窗口按键事件，用户输入代码或拼音首字母（如 `600`、`GZMT`）即时呼出右下角联想面板，支持键盘上下切换与回车。
5. **双主题支持 (`theme_stock_classic.json` & `theme_stock_modern.json`)**：
   - 经典同花顺黑底主题：高饱和红绿白黄对比，暗红/暗灰网格。
   - 现代暗黑主题：科技深灰底色，低饱和柔和配色。

---

## 3. 具体修改与新建内容清单

### 3.1 核心组件与扩展 (`src/` & `include/`)
- `include/ai_win_ui/financial_chart.h` & `src/financial_chart.cpp`：
  - 定义 `FinancialChart` 控件，实现多 Pane 划分、坐标变换、Direct2D 路径绘制与十字光标。
- `src/mock_market_engine.h` & `src/mock_market_engine.cpp`：
  - 实现行情数据模型、分时走势生成算法与定时 Tick 推流器。
- `src/keyboard_genie.h` & `src/keyboard_genie.cpp`：
  - 键盘精灵逻辑与浮层弹出控制器。
- `src/layout_parser_controls.cpp`：
  - 注册 `<financial-chart>` 标签解析，支持属性如 `gridRows`、`gridCols`、`showCrosshair` 等。

### 3.2 布局与资源文件 (`resource/`)
- `resource/layouts/stock_quotes.xml`：
  - 完整的同花顺分时工作台布局（Top Menu + Left Dock + Center Chart + Right Depth + Bottom Status）。
- `resource/themes/theme_stock_classic.json`：
  - 经典同花顺配色主题 Tokens。
- `resource/themes/theme_stock_modern.json`：
  - 现代扁平暗黑配色主题 Tokens。

### 3.3 示例程序入口与构建配置 (`samples/stock_terminal/` & `CMakeLists.txt`)
- `samples/stock_terminal/main.cpp`：
  - 创建独立金融工作台窗口，加载 `stock_quotes.xml`，绑定 `MockMarketEngine` 与 `KeyboardGenie`，支持按 `T` 键热切换双主题。
- `CMakeLists.txt`：
  - 添加 `ai_win_ui_add_app(stock_terminal samples/stock_terminal/main.cpp)` 编译目标。

---

## 4. 实施阶段与步骤

- [ ] **Step 1: 编写核心图表控件 `FinancialChart`**
  - 实现双 Y 轴自适应价格算法、时间 X 轴换算；
  - 实现分时曲线、成交量柱、指标线 Direct2D 绘制；
  - 实现鼠标十字光标与 Tooltip 浮窗。
- [ ] **Step 2: 实现模拟行情引擎 `MockMarketEngine` 与键盘精灵 `KeyboardGenie`**
  - 构建开盘分时走势生成器、逐笔成交发生器；
  - 注册股票代码与简拼字典，实现快速联想查询。
- [ ] **Step 3: 编写布局与主题资源**
  - 编写 `stock_quotes.xml` 完整工作台；
  - 编写经典黑底与现代暗黑两套主题 Tokens。
- [ ] **Step 4: 注册 `<financial-chart>` 标签并集成 `samples/stock_terminal`**
  - 扩展 `LayoutParser`；
  - 编写 `samples/stock_terminal/main.cpp`，配置编译目标。
- [ ] **Step 5: 编译测试与自动化回归验证**
  - 使用 `.\build.ps1` 校验编译通过；
  - 运行 `.\scripts\run_ci_local.ps1 -SkipBuild` 确保 7 项 CI 回归测试 100% PASS。
- [ ] **Step 6: 合并分支与文档归档**
  - 合并 `feature/stock-terminal` 到 `master`；
  - 更新 `DEVELOPMENT_LOG.md`、在 `doc/plan/` 增加设计归档文档、更新 `doc/README.md`。

---

## 5. 验证指标 (Verification Criteria)
1. **编译构建**：`stock_terminal.exe` 成功生成并可独立启动运行，零编译警告与报错。
2. **视觉保真度**：分时图白黄双曲线、昨收中轴基准、红绿成交量柱、右侧指数指标与逐笔成交布局与截图高度吻合。
3. **动态与交互**：
   - 鼠标悬停分时图区域时，十字光标准确定位并显示对应时间与点位；
   - 模拟行情平滑跳动，右侧逐笔成交流实时滚动；
   - 按下键盘（如代码 `6`）弹出键盘精灵搜索框；
   - 按 `T` 键可平滑在“经典同花顺黑底模式”与“现代暗黑模式”之间切换主题。
4. **CI 质量红线**：7 项本地 CI 回归测试 **100% PASS**（不破坏现有任何控件、XML 解析与 Golden Tests）。
