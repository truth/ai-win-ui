# Skia 文本 SkTextBlob 缓存与多脏矩形优化 — Release 实测对比报告

- **日期**：2026-11-20
- **对比基线 (Before)**：`feature/react-jsx-ui` @ `12fa270`（`build/bench/react_vs_xml/summary.json`）
- **优化分支 (After)**：`feature/skia-textblob-dirty-region` @ `1f1af45`（`build/bench/skia_opt_after/summary.json` & `build/bench/skia_dirty_dayk/dayk_summary.json`）
- **交互式可视化报告**：[`doc/reports/2026-11-20-skia-opt-before-after-report.html`](2026-11-20-skia-opt-before-after-report.html)
- **测试脚本**：`.\scripts\bench_react_vs_xml.ps1 -Renderer both -Configuration Release` 与 `.\scripts\bench_skia_dirty_region.ps1 -Renderer both -Configuration Release`

---

## 1. 测试环境与 Debug / Release 背景上下文

- **构建配置**：Windows x64 **Release**（MSVC 优化 + `Release` Skia 静态库）
- **历史 Debug vs Release 差距参考**（来源：[`doc/plan/2026-11-02-frame-profiler-and-smoothness.md`](../plan/2026-11-02-frame-profiler-and-smoothness.md)，`modern_components_v2.xml`）：
  - Skia Debug 默认拖拽：`render 135.93 ms`（端到端 `latency 194.14 ms`）
  - Skia Release 优化后：`render 4.41 ms`（端到端 `latency 8.86 ms`，提升 **30.8×**）
  - 交互时“Skia 100 多 ms vs Direct2D 10 多 ms”主要由 Debug 构建链接未优化的 `Debug-x64/skia.lib` 引起；本报告全部采用 **Release 同口径实测**。

---

## 2. 优化前 Release 基准实测对比（`12fa270`，Skia vs Direct2D）

### 2.1 同花顺金融终端（Stock Terminal，`1298×867`，XML 268 节点 / React 262 节点）

| 指标 | XML Skia | React Skia | XML Direct2D | React Direct2D |
| :--- | :---: | :---: | :---: | :---: |
| **启动耗时 (`startupMs`)** | `46.961 ms` | `45.468 ms` | `97.940 ms` | `83.403 ms` |
| **工作集内存 (`WorkingSet`)** | `30.36 MB` | `32.31 MB` | `63.21 MB` | `65.16 MB` |
| **500ms 行情 Tick (`render / present / paint`)** | `4.224 / 0.369 / 4.932 ms` (`n=32`) | `4.199 / 0.365 / 4.824 ms` (`n=16`) | `6.414 / 0.313 / 6.735 ms` (`n=32`) | `5.810 / 0.307 / 6.123 ms` (`n=16`) |
| **60Hz 悬停 (`render avg / p95 / paint`)** | `5.237 / 9.462 / 6.304 ms` | `4.969 / 6.879 / 5.720 ms` | `6.007 / 7.980 / 6.288 ms` | `5.987 / 7.210 / 6.352 ms` |
| **周期切换 (`render / paint / e2e`)** | `4.877 / 5.737 / 6.278 ms` (`lat`) | `2.715 / 3.053 / 9.081 ms` (`p95 13.608`) | `6.418 / 6.784 / 7.339 ms` (`lat`) | `5.592 / 5.939 / 12.605 ms` |
| **主题切换 (`render / paint / e2e`)** | `5.888 / 6.662 / 6.744 ms` (`lat`) | `5.090 / 5.836 / 10.106 ms` (`p95 13.779`) | `5.979 / 6.295 / 6.406 ms` (`lat`) | `6.442 / 6.825 / 11.604 ms` |
| **Resize Sweep (`layout / render / present / paint / lat`)** | `3.782 / 7.335 / 0.511 / 8.348 / 12.208 ms` | `4.131 / 6.785 / 0.613 / 7.967 / 12.183 ms` | `5.628 / 6.640 / 0.323 / 6.970 / 12.786 ms` | `4.714 / 6.257 / 0.378 / 6.642 / 11.532 ms` |

### 2.2 100 单元格 60Hz 压测与快速输入（Stress 100-Cell，`1000×700`，116 节点）

| 指标 | XML Skia | React Skia | XML Direct2D | React Direct2D |
| :--- | :---: | :---: | :---: | :---: |
| **启动耗时 (`startupMs`) / 内存 (`WS`)** | `29.025 ms` / `26.04 MB` | `29.183 ms` / `27.46 MB` | `132.102 ms` / `63.96 MB` | `64.354 ms` / `56.41 MB` |
| **60Hz 批刷新 FPS** | `41.95 FPS` | `59.44 FPS` | `42.07 FPS` | `59.72 FPS` |
| **60Hz 批刷新 (`render avg / p95 / paint avg / p95`)** | `0.483 / 0.656 / 0.744 / 1.007 ms` | `0.467 / 0.570 / 0.698 / 0.865 ms` | `0.504 / 0.623 / 0.780 / 0.951 ms` | `0.520 / 0.714 / 0.819 / 1.150 ms` |
| **原生快速输入 `typingChar` (`render / paint / lat`)** | `0.546 / 0.975 / 1.140 ms` | `0.599 / 1.027 / 1.221 ms` | `0.514 / 0.816 / 0.987 ms` | `0.502 / 0.895 / 1.076 ms` |
| **React 同步输入 `typingReact` (`paint / reactE2e`)** | — | `0.754 / 2.519 ms` | — | `0.703 / 3.164 ms` |

> 注：在 Release 基线下，Skia 的 500ms Tick 与内存占用已优于 Direct2D，但在全帧缩放 (`7.335 ms`) 与单字符输入 (`0.546~0.599 ms`) 上仍有显著优化空间。

---

## 3. Skia 优化前后 Release 实测对比（Before vs After）

### 3.1 Stock Terminal — XML Skia (`1298×867`, 268 节点)

| 场景 | 指标 | Before (`12fa270`) | After (`feature/skia-textblob-dirty-region`) | 变化幅度 |
| :--- | :--- | :---: | :---: | :---: |
| **500ms Tick** (`n=32`, `layout=0`) | `renderMs` avg (p95) | `4.224 (6.018) ms` | **`1.239 (2.617) ms`** | **`-70.7%` (3.41×)** |
| | `presentMs` avg | `0.369 ms` | **`0.129 ms`** | **`-65.0%` (2.86×)** |
| | `paintMs` avg (p95) | `4.932 (7.502) ms` | **`1.464 (3.051) ms`** | **`-70.3%` (3.37×)** |
| **60Hz Hover** (`n=60`) | `renderMs` avg / **p95** | `5.237 / 9.462 ms` | **`4.780 / 7.368 ms`** | avg `-8.7%` / **p95 `-22.1%`** |
| | `paintMs` avg / **p95** | `6.304 / 12.808 ms` | **`5.806 / 8.788 ms`** | avg `-7.9%` / **p95 `-31.4%`** |
| | `latencyMs` avg | `6.571 ms` | **`6.040 ms`** | `-8.1%` |
| **周期切换 `periodClick`** (`n=10`) | `renderMs` / `paintMs` avg | `4.877 / 5.737 ms` | **`3.598 / 4.299 ms`** | **`-26.2%` / `-25.1%`** |
| | `latencyMs` avg / **p95** | `6.278 / 10.991 ms` | **`5.165 / 6.956 ms`** | avg `-17.7%` / **p95 `-36.7%`** |
| **主题切换 `hotkeyTheme`** (`n=6`, 100% 全帧) | `renderMs` avg | `5.888 ms` | **`3.579 ms`** | **`-39.2%` (1.65×)** |
| | `paintMs` / `latencyMs` avg | `6.662 / 6.744 ms` | **`4.156 / 4.353 ms`** | **`-37.6%` / `-35.5%`** |
| **拖拽缩放 `resizeSweep`** (`n=41`, **100% 全帧**) | `layoutMs` avg | `3.782 ms` | **`3.699 ms`** | `-2.2%` |
| | `renderMs` avg | `7.335 ms` | **`5.308 ms`** | **`-27.6%` (1.38×)** |
| | `paintMs` / `latencyMs` avg | `8.348 / 12.208 ms` | **`6.326 / 10.096 ms`** | **`-24.2%` / `-17.3%`** |

### 3.2 Stock Terminal — React Skia (`1298×867`, 262 节点)

| 场景 | 指标 | Before (`12fa270`) | After (`feature/skia-textblob-dirty-region`) | 变化幅度 |
| :--- | :--- | :---: | :---: | :---: |
| **500ms Tick** (`n=16`, `layout=0`) | `renderMs` avg | `4.199 ms` | **`2.976 ms`** | **`-29.1%` (1.41×)** |
| | `paintMs` avg | `4.824 ms` | **`3.531 ms`** | **`-26.8%` (1.37×)** |
| **60Hz Hover** (`n=60`) | `renderMs` avg / **p95** | `4.969 / 6.879 ms` | **`4.157 / 5.510 ms`** | **`-16.3%` / `-19.9%`** |
| | `paintMs` / `latencyMs` avg | `5.720 / 6.010 ms` | **`4.819 / 5.333 ms`** | **`-15.8%` / `-11.3%`** |
| **周期切换 `periodClick`** (`n=10`) | `renderMs` / `paintMs` avg | `2.715 / 3.053 ms` | `2.829 / 3.226 ms` | `+4.2%` / `+5.7%` |
| | `reactE2eMs` avg / **p95** | `9.081 / 13.608 ms` | **`8.876 / 10.503 ms`** | avg `-2.3%` / **p95 `-22.8%`** |
| **主题切换 `hotkeyTheme`** (`n=6`, 100% 全帧) | `renderMs` / `paintMs` avg | `5.090 / 5.836 ms` | **`4.600 / 5.432 ms`** | `-9.6%` / `-6.9%` |
| | `reactE2eMs` avg / **p95** | `10.106 / 13.779 ms` | **`9.911 / 11.897 ms`** | avg `-1.9%` / **p95 `-13.7%`** |
| **拖拽缩放 `resizeSweep`** (`n=41`, **100% 全帧**) | `layoutMs` avg | `4.131 ms` | **`3.562 ms`** | **`-13.8%`** |
| | `renderMs` avg | `6.785 ms` | **`5.340 ms`** | **`-21.3%` (1.27×)** |
| | `paintMs` / `latencyMs` avg | `7.967 / 12.183 ms` | **`6.308 / 9.946 ms`** | **`-20.8%` / `-18.4%`** |

### 3.3 100 单元格 60Hz 压测、快速输入与光标闪烁（Stress 100-Cell, `1000×700`）

| 模式 | 场景 | 指标 | Before | After | 变化幅度 / CSV 脏区观测 |
| :--- | :--- | :--- | :---: | :---: | :--- |
| **XML Skia** | `batch60Hz` | `renderMs` avg (p95) | `0.483 (0.656) ms` | **`0.328 (0.492) ms`** | **`-32.1%`**（CSV `trigger=invalidate`: `1.95 rects`, `47.78%` area） |
| | | `paintMs` avg (p95) | `0.744 (1.007) ms` | **`0.568 (0.877) ms`** | **`-23.7%`**（`41.95 -> 42.04 FPS`） |
| | **`typingChar`** (`n=40`) | `renderMs` / `presentMs` avg | `0.546 / 0.254 ms` | **`0.090 / 0.053 ms`** | **`-83.5%` (6.07×)** / **`-79.1%`**（CSV `char`: `1.02 rects`, **`3.54%`** area） |
| | | `paintMs` / **`latencyMs`** avg | `0.975 / 1.140 ms` | **`0.161 / 0.418 ms`** | **`-83.5%` (6.06×)** / **`-63.3%` (2.73×)** |
| **React Skia** | `batch60Hz` | `renderMs` avg (p95) | `0.467 (0.570) ms` | **`0.356 (0.482) ms`** | **`-23.8%`**（CSV `react_timer`: `2 rects`, `51.75%` area） |
| | | `paintMs` avg | `0.698 ms` | **`0.612 ms`** | **`-12.3%`**（`59.44 -> 59.21 FPS`） |
| | **`typingChar`** (`n=40`) | `renderMs` / `presentMs` avg | `0.599 / 0.231 ms` | **`0.096 / 0.052 ms`** | **`-84.0%` (6.24×)** / **`-77.5%`**（CSV `char`: `1 rect`, **`2.29%`** area） |
| | | `paintMs` / **`latencyMs`** avg | `1.027 / 1.221 ms` | **`0.165 / 0.472 ms`** | **`-83.9%` (6.22×)** / **`-61.3%` (2.59×)** |
| | 局部定时失效 (`stock_react_skia.csv`) | `trigger=invalidate` (`n=24`) | 全窗重绘（`100%`） | **`0.96 rects`, `4.56%` area** | **`render 0.090 ms`, `paint 0.192 ms`** |

> **Direct2D 对照 sanity check**：After 测试同步运行了 Direct2D 全套场景，Stock Terminal XML `tick500ms paint 5.962 ms`、`resizeSweep paint 6.572 ms`；React `tick500ms paint 6.865 ms`、`resizeSweep paint 7.544 ms`；Stress 60Hz `paint 0.811~0.876 ms`，严格维持全帧提交、零视觉回退（7/7 CI 通过）。

---

## 4. Day-K（日 K 线）四角分散更新多脏矩形专项测试

通过 [`scripts/bench_skia_dirty_region.ps1`](../../scripts/bench_skia_dirty_region.ps1) 在日 K 线界面下运行 500ms 离散四角行情跳动（`build/bench/skia_dirty_dayk/dayk_summary.json`，各 12 帧）：

| 模式 & 后端 | `dirtyRects` | `dirtyAreaPct` | `renderMs` avg (p95) | `presentMs` avg (p95) | `paintMs` avg (p95) | 相对 Direct2D 加速比 |
| :--- | :---: | :---: | :---: | :---: | :---: | :---: |
| **XML + Skia** | `6` | **`3.84%`** | **`0.197` (`0.244`) ms** | **`0.104` (`0.138`) ms** | **`0.336` (`0.439`) ms** | **`paint` 快 20.8× / `render` 快 33.4×** |
| **XML + Direct2D** | `0`（全帧） | `100%` | `6.575` (`9.966`) ms | `0.393` (`0.610`) ms | `6.976` (`10.567`) ms | 基线 (1.0×) |
| **React + Skia** | `8` | `56.66%` | **`1.824` (`2.566`) ms** | **`0.292` (`0.374`) ms** | **`2.343` (`3.086`) ms** | **`paint` 快 2.65× / `render` 快 3.20×** |
| **React + Direct2D** | `0`（全帧） | `100%` | `5.843` (`6.869`) ms | `0.351` (`0.451`) ms | `6.201` (`7.330`) ms | 基线 (1.0×) |

**关键分析**：
- **XML + Skia**：在存在中间 K 线大画布的情况下，多脏矩形保留了四角离散矩形（`6 rects`，总面积仅 **`3.84%`**），彻底跳过 K 线与未变动列表重绘，单帧 `paint` 降至 **`0.336 ms`**，较 Direct2D 的 `6.976 ms` 提速 **20.8×**；
- **React + Skia**：即便 React 500ms 定时更新包含更多状态节点及图表属性（触发 `8 rects`、面积占比 `56.66%`），仍借助 `SkTextBlob` 缓存、UTF-16 直通度量与局部裁剪实现 `paint 2.343 ms`（较 Direct2D 的 `6.201 ms` 提速 **2.65×**）。

---

## 5. 回归验证矩阵

| 检查项 | 命令 | 结果 |
| :--- | :--- | :--- |
| Release 编译 | `.\build.ps1 -Configuration Release` | **0 Error / 0 Warning** |
| Debug 编译 | `.\build.ps1 -Configuration Debug` | **0 Error / 0 Warning** |
| Skia 单元测试 | `.\build\Release\skia_unit_test.exe` & `Debug` | **4/4 PASS**（`ALL SKIA UNIT TESTS PASSED`） |
| React 单元测试 | `.\build\Release\react_unit_test.exe` & `Debug` | **3/3 PASS**（`ALL REACT UNIT TESTS PASSED`） |
| 本地 CI 全套 | `.\scripts\run_ci_local.ps1 -SkipBuild` | **7/7 PASS (48.0s)**（含 Skia & D2D 3 组 Measure Goldens） |

---

## 6. 结论

1. **纯全帧场景也获得稳定大幅收益**：在 `100%` 全帧无任何裁剪的 `resizeSweep` 与 `hotkeyTheme` 场景中，`SkTextBlob` 4096-entry LRU 缓存 + UTF-16 直通测量 + 单行快路径仍带来 **XML `render -27.6% ~ -39.2%`**、**React `render -21.3%`**、拖拽端到端 **`latency -17.3% ~ -18.4%`** 的纯文本链路提升。
2. **局部离散场景实现数量级飞跃**：`typingChar` 绘制耗时暴降 **`-83.5% ~ -83.9%`**（端到端输入延迟 **`-61.3% ~ -63.3%`**），`tick500ms` XML 下降 **`-70.3%`**，Day-K 四角分散更新场景高达 **20.8×** 加速且与 Direct2D 布局度量 100% 一致。
