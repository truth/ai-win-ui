# React (QuickJS-ng) vs XML (C++) 刷新性能与响应速度评估测试报告

- **日期**：2026-11-20
- **分支**：`feature/react-jsx-ui`
- **测试环境**：Windows 10.0.19045 x64 · Intel Core i7-1165G7 @ 2.80GHz · 15.7 GB RAM · NVIDIA GeForce MX450 · 125% DPI
- **构建配置**：**Release** (`.\build.ps1 -Configuration Release`) × 双渲染后端 (**Skia Release CPU Raster** / **Direct2D HWND**)
- **基准脚本**：[`scripts/bench_react_vs_xml.ps1`](../../scripts/bench_react_vs_xml.ps1)
- **原始数据**：[`build/bench/react_vs_xml/summary.json`](../../build/bench/react_vs_xml/summary.json)
- **交互式 HTML 报表**：[`doc/reports/2026-11-20-react-vs-xml-perf-report.html`](2026-11-20-react-vs-xml-perf-report.html)

---

## 1. 测试方法与对标设计

为了客观评估自研轻量 TypeScript React 运行时（QuickJS-ng v0.18.0 + 双线程整型 `MutationBatch` 协议）相对于传统 XML + C++ 原生调用方案的性能表现与工程开销，本测试从两个维度构建完全同构的 **8 组对比矩阵**（2 套负载 × 2 种 UI 模式 × 2 种后端）：

1. **Suite 1 — 真实金融终端对标 (`1280×820` 客户区 / `1298×867` 窗口)**
   - **XML + C++**：`stock_terminal.exe` loading `resource/layouts/stock_quotes.xml`（`24,975 bytes` / `268` 节点）。
   - **React JSX**：`react_demo.exe` loading `resource/react/stock_terminal.bundle.js`（`95,371 bytes` unminified / `~41.5 KB` minified / `262` 节点）。
   - **测试动作序列**：冷启动首帧 → 8s 纯静态 500ms Tick 刷新 → 60Hz 十字光标图面移动（60 帧）→ 日K ↔ 分时周期切换点击（10 次）→ `T` 快捷键双主题切换（6 次）→ 60Hz 边缘连续缩放（40 步）。

2. **Suite 2 — 60Hz 100格高频看板 + 极速连续输入压力 (`1000×700`)**
   - **XML + C++**：`perf_stress_xml.exe` loading `resource/layouts/bench_stress_xml.xml`（`11,439 bytes` / `116` 节点）。
   - **React JSX**：`react_demo.exe` loading `resource/react/bench_stress.bundle.js`（`39,281 bytes` unminified / `116` 节点）。
   - **测试动作序列**：冷启动首帧 → 4s 持续 **16ms (60Hz)** 100 个数值单元格批量文本刷新 → 聚焦 `TextInput` 后每 `25ms` 连击 **40 次字符输入**（`WM_CHAR`），同时采集 **UI 线程即时响应 (`latency_ms`)** 与 **React 异步工作线程 Diff 回推全链路 (`react_e2e_ms`)**。

3. **新增零开销全链路埋点 (`src/frame_profiler.h` + `src/react/`)**：
   - `startup_ms`：从 `UiHost::Initialize()` 入口到**首个真实内容帧**完成呈现的端到端毫秒数；
   - `react_js_ms` / `react_apply_ms`：QuickJS Worker 执行与 UI 线程应用整型 `MutationBatch` 的独立耗时；
   - `react_e2e_ms`：从 UI 线程触发事件（点击 / 热键 / 输入）经 Worker 线程 Reconcile 再返回 UI 线程呈现完成的总延迟。

---

## 2. Suite 1：同花顺金融终端实测数据（Release）

### 2.1 冷启动、内存占用与非事件渲染场景

| 指标 | XML (`Skia`) | React (`Skia`) | 差异 | XML (`Direct2D`) | React (`Direct2D`) | 差异 |
| :--- | :---: | :---: | :---: | :---: | :---: | :---: |
| **首内容帧冷启动 (`startup_ms`)** | `46.961 ms` | `45.468 ms` | **`-1.49 ms` (持平)** | `97.940 ms` | `83.403 ms` | **`-14.54 ms`** |
| ↳ *React 初始化 JS / Apply 拆分* | `—` | `23.968 / 8.342 ms` | — | `—` | `25.232 / 10.782 ms` | — |
| **进程内存 WorkingSet / Private** | `30.36 / 12.36 MB` | `32.31 / 14.14 MB` | **`+1.95 / +1.78 MB`** | `63.21 / 53.67 MB` | `65.16 / 52.66 MB` | **`+1.95 / -1.01 MB`** |
| **500ms Tick 刷新 (`layout_count`)** | `0 次` (`n=32`) | `0 次` (`n=16`) | **同为 0 重排** | `0 次` (`n=32`) | `0 次` (`n=16`) | **同为 0 重排** |
| **500ms Tick JS / Apply / Paint** | `0 / 0 / 4.932 ms` | `3.744 / 0.033 / 4.824 ms` | UI 帧耗时一致 | `0 / 0 / 6.735 ms` | `3.538 / 0.032 / 6.123 ms` | UI 帧耗时一致 |
| **60Hz 十字光标 Hover (`n=60` avg/p95)** | `6.571 / 12.885 ms` | **`6.010 / 8.201 ms`** | **0.00ms JS 开销** | `6.604 / 8.375 ms` | `6.624 / 8.138 ms` | **完全一致 (~6.6ms)** |
| **60Hz 边缘连续 Resize (`n=41` avg/p95)** | `12.208 / 14.007 ms` | `12.183 / 15.928 ms` | **基本持平 (~12.2ms)** | `12.786 / 16.864 ms` | **`11.532 / 13.419 ms`** | **完全走 C++ Yoga** |

> **关键发现 1（纯 Native 高频交互 ZERO JS 税）**：窗口缩放（Resize）与 60Hz 图表十字光标移动全部在 C++ UI 线程原生完成，React 运行时在这些高频手势中 **`js_ms = 0 ms`**，性能与纯 C++ XML 方案处于同一水平（均稳跑 80~160+ FPS）。

### 2.2 交互事件响应速度（点击切周期 & 热键切主题）

| 场景 | 后端 | 模式 | 样本数 | `react_js_ms` (avg/p95) | `react_apply_ms` | `paint_ms` (avg/p95) | **端到端响应延迟 avg / p95 / max** | React 额外延迟 |
| :--- | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: |
| **日K↔分时 点击 (`10x`)** | Skia | XML | 10 | `0.000` | `0.000` | `5.737 / 10.061` | **`6.278 / 10.991 / 10.991 ms`** | 基准 |
| **日K↔分时 点击 (`10x`)** | Skia | React | 10 | `4.755 / 7.568` | `0.018` | `3.053 / 4.193` | **`9.081 / 13.608 / 13.608 ms`** | **`+2.80 ms` (avg)** |
| **日K↔分时 点击 (`10x`)** | Direct2D | XML | 10 | `0.000` | `0.000` | `6.784 / 10.220` | **`7.339 / 10.860 / 10.860 ms`** | 基准 |
| **日K↔分时 点击 (`10x`)** | Direct2D | React | 10 | `4.162 / 6.125` | `0.014` | `5.939 / 9.695` | **`12.605 / 19.606 / 19.606 ms`** | **`+5.27 ms` (avg)** |
| **'T' 快捷键主题切换 (`6x`)** | Skia | XML | 6 | `0.000` | `0.000` | `6.662 / 10.046` | **`6.744 / 10.087 / 10.087 ms`** | 基准 |
| **'T' 快捷键主题切换 (`6x`)** | Skia | React | 6 | `8.223 / 8.965` | `0.069` | `5.836 / 9.133` | **`10.106 / 13.779 / 13.779 ms`** | **`+3.36 ms` (avg)** |
| **'T' 快捷键主题切换 (`6x`)** | Direct2D | XML | 6 | `0.000` | `0.000` | `6.295 / 7.055` | **`6.406 / 7.090 / 7.090 ms`** | 基准 |
| **'T' 快捷键主题切换 (`6x`)** | Direct2D | React | 6 | `9.073 / 12.243` | `0.065` | `6.825 / 7.522` | **`11.604 / 12.973 / 12.973 ms`** | **`+5.20 ms` (avg)** |

---

## 3. Suite 2：60Hz 100格高频批量更新 & 极速打字压测（Release）

| 测试项 | XML (`Skia`) | React (`Skia`) | XML (`Direct2D`) | React (`Direct2D`) |
| :--- | :---: | :---: | :---: | :---: |
| **冷启动 (`startup_ms`)** | `29.025 ms` | `29.183 ms` | `132.102 ms` * | `64.354 ms` |
| **内存 WorkingSet / Private** | `26.04 / 8.59 MB` | `27.46 / 9.47 MB` | `63.96 / 47.44 MB` | `56.41 / 45.36 MB` |
| **4 秒 60Hz 实测帧率 (`achievedFps`)** | `41.95 FPS` (`168 帧`) | **`59.44 FPS` (`238 帧`)** | `42.07 FPS` (`168 帧`) | **`59.72 FPS` (`239 帧`)** |
| **60Hz 单帧 JS Worker (`js_ms` avg / p95)** | `—` | `1.518 / 2.106 ms` | `—` | `2.227 / 3.357 ms` |
| **UI 线程 Apply 耗时 (`apply_ms`)** | `0 ms` | **`0.040 ms`** | `0 ms` | **`0.038 ms`** |
| **Yoga 重排触发次数 (`layoutTotal`)** | **`0 次`** | **`0 次`** | **`0 次`** | **`0 次`** |
| **UI 线程 Render (`render_ms` avg / p95)** | `0.483 / 0.656 ms` | `0.467 / 0.570 ms` | `0.504 / 0.623 ms` | `0.520 / 0.714 ms` |
| **UI 线程 Paint (`paint_ms` avg / p95 / max)** | `0.744 / 1.007 / 1.280 ms` | `0.698 / 0.865 / 1.540 ms` | `0.780 / 0.951 / 1.195 ms` | `0.819 / 1.150 / 1.494 ms` |
| **极速打字 UI 首响应 (`char` avg / p95 / max)** | `1.140 / 1.525 / 1.724 ms` | **`1.221 / 1.894 / 2.002 ms`** | `0.987 / 1.447 / 1.577 ms` | **`1.076 / 1.345 / 1.480 ms`** |
| **极速打字异步 JS 全链路 (`react_e2e_ms`)** | `—` | `2.519 / 3.611 / 3.996 ms` | `—` | `3.164 / 4.300 / 4.619 ms` |

*\* 注：`perf_stress_xml.exe` 在 Direct2D 的 `startup_ms = 132.102 ms` 包含首帧 `EndDraw` 阶段 `79.65 ms` 的 DXGI SwapChain 冷初始化开销；React 版在第 1~2 帧空宿主阶段完成了设备预热，故内容首帧在 64ms 落地。*

---

## 4. 深度归因与架构洞察

### 4.1 为什么 React 的体验几乎“零妥协”？
1. **极轻 C++ Apply 开销 (`0.01~0.07 ms`)**：采用整型 `MutationOp`（无 JSON 解析）后，即使单帧同步 100 个格子的文本变化，UI 线程 `ApplyBatch` 平均也仅耗 **`0.038~0.040 ms`**，且严格命中视觉快路径，**Yoga 重排次数全过程为 0**，UI 线程 `paint_ms` 与纯 C++ 版几乎没有差异（`0.70–0.82 ms`）。
2. **乐观 UI 击键延迟仅 ~1.1 ms**：极速连打 `WM_CHAR` 时，React 与 XML 的即时屏幕回显延迟完全一致（Skia `1.22 ms vs 1.14 ms`，Direct2D `1.08 ms vs 0.99 ms`），异步状态调和则在 **2.5~3.2 ms** 内在后台完成校验且经 `eventSeq` 丢弃冗余回显。
3. **仅 +1.8~1.9 MB 内存代价，启动持平**：内嵌 QuickJS-ng v0.18.0 + 轻量 TS 运行时只带来不到 **2 MB** 常驻内存增量（`Skia` 下 `30.36 → 32.31 MB`），且由于双线程并行初始化，冷启动总耗时与 XML 相当甚至更稳定（`45.5 ms vs 47.0 ms`）。
4. **独立定时线程让高频看板跑满 60 FPS**：XML 版采用 Win32 UI 线程 `SetTimer(..., 16)` 受 Windows 默认 15.6ms 时钟量化与消息泵复用影响，经常跌落至隔帧触发（`~23.8ms` 步长，实测 **`42.0 FPS`**）；React Worker 线程使用 `std::condition_variable::wait_for` 配合 `PostMessageW` 唤醒，稳定跑出 **`59.44~59.72 FPS`**。

### 4.2 React 与 XML 的真实差距在哪里？（客观开销量化）
1. **纯 JS Diff 税（约 `+2.8 ~ +5.3 ms` 端到端交互延迟）**：
   - 在 `Stock Terminal`（`262` 个节点、无 `React.memo` 隔离）下，切周期/切主题会重新执行 `StockTerminalApp()` 函数组件并对比全部 262 个 VNode：QuickJS 耗时约 **`4.2~9.1 ms`**（比纯 C++ XML 多 **`2.8~5.3 ms`**），整体端到端响应仍稳居 **9~13 ms（< 1 帧 @ 60Hz）** 内；
   - 注意一个极有意思的现象：在主题切换 `'T'` 场景下，虽然 JS 线程跑了 `8.2~9.1 ms`，但异步与 `Paint` 部分重叠，或单核 CPU 在 QuickJS 解释执行全树 VNode 遍历时拉高了 JS 时间，可通过将静态菜单/盘口提取为受控 `React.memo` 组件或跳过未变分支把 JS 耗时进一步压缩至 **< 1.5 ms**。
2. **Skia Release vs Direct2D Release 总结**：
   - 在 Release 模式下，Skia CPU Raster 已从 Debug 的 `100+ ms` 降至 **`0.7~6.8 ms`**，与 Direct2D 处于同一梯队；对于金融终端高频更新 100 格这种中等面积局部重绘场景，Skia Release 的 `DirtyRect` 局部 `BitBlt` 甚至比 Direct2D 全帧 `EndDraw` 略快（`0.70 ms vs 0.82 ms`）。

---

## 5. 优化建议（下一阶段路线）

1. **TS 运行时新增组件级 `memo()` 剪枝**：当前全量组件重渲染时对未变化的子组件（如顶部菜单、新闻列表、右侧静态盘口项）仍会执行 JS render。引入浅比较 `memo` 后，切主题 / 500ms Tick 下的 `react_js_ms` 有望从 `3.7~9.1 ms` 降至 **`< 1.5 ms`**。
2. **股票终端 500ms Tick 单帧合并优化**：观察到 XML 与 React 版在 `FinancialChart` 更新时均会伴随一次约 4~16ms 后的次级 `invalidate` 帧（使 8s 出现 `16 + 16` 帧）；检查并收敛 `AppendPoint` 与批提交间的双重 `InvalidateSelf`，可再省下约 **50%** 空转绘制。
3. **可选将 Win32 动画定时器切到高精度 multimedia / waitable timer**：帮助纯 C++ XML 高频样本从 42 FPS 提升至满帧 60 FPS。

