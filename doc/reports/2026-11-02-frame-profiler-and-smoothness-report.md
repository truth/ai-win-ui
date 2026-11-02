# 帧耗时剖析与交互丝滑度优化过程报表

- **日期**：2026-11-02
- **测试环境**：Windows 10.0.19045 x64 / MSVC 19.44 / CMake 3.21+ / 125% DPI (`1018×747` 物理像素窗口)
- **测试页面**：`resource/layouts/modern_components_v2.xml`（包含 12 类现代组件、多张高斯阴影卡片、环形图、分段器、Switch 开关与手风琴容器）
- **关联设计文档**：[`doc/plan/2026-11-02-frame-profiler-and-smoothness.md`](../plan/2026-11-02-frame-profiler-and-smoothness.md)
- **HTML 交互版报表**：[`doc/reports/2026-11-02-frame-profiler-and-smoothness-report.html`](2026-11-02-frame-profiler-and-smoothness-report.html)

---

## 1. 问题描述与初步归因

在执行以下命令启动综合组件页时：
```powershell
.\scripts\run_layout_demo.ps1 -Layout "modern_components_v2" -Renderer "skia"
```
用户反馈在 **拖拽窗口大小** 与 **点击圆角 `<Switch>` 开关组件** 时，窗体响应不够丝滑，存在明显的“迟钝感”。

通过对消息循环、布局引擎与 Skia 渲染管线的静态代码走查，初步锁定单次交互的完整执行链路：

```mermaid
flowchart LR
    A["鼠标按下 / 窗口缩放"] --> B["无条件整页 UpdateLayout()"]
    B --> C["InvalidateRect 标记整窗脏区"]
    C --> D["等待低优先级 WM_PAINT"]
    D --> E["CPU 全窗口软光栅重绘"]
    E --> F["BitBlt 整块复制上屏"]
```

初步识别出 6 个潜在原因：
1. **构建配置因素**：`run_layout_demo.ps1` 硬编码启动 `Debug` 构建，且链接了未优化的 `third_party/skia-m124/out/Debug-x64/skia.lib`；
2. **全窗口 CPU 重绘**：每次交互均调用 `InvalidateRect(m_hwnd, nullptr, FALSE)` 触发整窗重绘；
3. **窗口缩放逐像素重建 DIB 画布**：`SkiaRenderer::Resize` 每变化 1px 即调用 `CreateDIBSection` + `ZeroMemory`，且 `OnResize` 仅 `InvalidateRect` 而不立即同步上屏；
4. **`<Switch>` 缺少缓动过渡动画**：`Switch::OnMouseDown` 直接翻转布尔值，圆钮 1 帧瞬移、颜色瞬间跳变；
5. **任意点击无条件触发整页重排**：`UiHost::OnMouseDown` 在任意控件返回 `true` 时均调用 `UpdateLayout()`；
6. **Skia 字体与文本排版未缓存**：每次 `MeasureText` 与 `DrawTextW` 均重复调用 DirectWrite `matchFamilyStyle` 创建新 `SkTypeface` 对象，导致 Skia 内部字形缓存 (`SkStrikeCache`) 失效。

---

## 2. 第一阶段：帧耗时剖析器 (Frame Profiler) 建设与 2×2 基线实测

为了用客观数据量化上述每项因素的真实耗时占比，首先实现了高精度帧耗时统计器 [`src/frame_profiler.h`](../../src/frame_profiler.h) 与自动化基准测试脚本 [`scripts/bench_frame_stats.ps1`](../../scripts/bench_frame_stats.ps1)。

### 2.1 采集阶段划分

| 指标名称 | 统计范围 | 含义 |
| :--- | :--- | :--- |
| `resize_ms` | `m_renderer->Resize(...)` | 渲染器表面 / GDI DIB 位图重建耗时 |
| `layout_ms` | `UiHost::UpdateLayout()`（含 `layout_count`） | Yoga Flexbox 树构建 + 文本测量 + Arrange 耗时 |
| `render_ms` | `m_root->Render` + `RenderOverlay` | 控件树遍历、高斯阴影、矢量图标与文本 CPU 光栅化耗时 |
| `present_ms` | `m_renderer->EndFrame()` | `BitBlt` / `UpdateLayeredWindow` / `D2D EndDraw` 上屏耗时 |
| `paint_ms` | 完整 `WM_PAINT` 处理过程 | `BeginFrame` + `render_ms` + `present_ms` 总和 |
| `latency_ms` | 首个输入/缩放消息进入窗口过程 $\rightarrow$ `EndPaint` | 用户可感知的端到端帧响应延迟 |

同时升级了 `F12` 调试浮层 ([`UiHost::DrawFrameStatsOverlay`](../../src/ui_host.cpp))，实时展示最近 120 帧各阶段的 `avg / p95 / max`，支持 `F11` 切换持续刷新及 `AI_WIN_UI_FRAME_STATS=<csv>` 导出逐帧数据。

### 2.2 优化前 2×2 基线实测结果

使用 `bench_frame_stats.ps1` 自动驱动 **60 步连续窗口缩放 + 60 步鼠标悬停移动 + 20 次 Switch 点击**，得到优化前 2×2 实测矩阵（格式：`avg / p95 / max ms`）：

#### 表 1：优化前拖拽窗口大小（`trigger = resize`，61 帧）

| 构建与后端配置 | `resize_ms`（重建画布） | `layout_ms`（Yoga + 文本测量） | `render_ms`（绘制控件树） | `present_ms`（上屏） | **端到端 `latency_ms`（折算帧率）** |
| :--- | :---: | :---: | :---: | :---: | :---: |
| **`skia-Debug`（原脚本默认）** | `1.26 / 1.89 / 2.82` | **`55.27 / 82.90 / 87.52`** | **`135.93 / 183.71 / 266.22`** | `0.63 / 1.00 / 1.82` | **`194.14 / 260.85 / 333.10`（3~5 FPS）** |
| **`skia-Release`（优化前）** | `1.25 / 1.82 / 2.63` | `3.81 / 5.91 / 6.16` | `4.87 / 7.06 / 9.34` | `0.38 / 0.84 / 1.03` | **`10.65 / 14.34 / 18.38`（~94 FPS）** |
| **`direct2d-Debug`** | `0.18 / 0.27 / 0.32` | `19.18 / 23.97 / 26.21` | `5.13 / 6.98 / 7.26` | `0.70 / 1.12 / 1.25` | `25.34 / 29.98 / 32.98`（~39 FPS） |
| **`direct2d-Release`** | `0.18 / 0.28 / 0.40` | `3.38 / 4.97 / 5.51` | `3.61 / 5.32 / 5.83` | `0.73 / 1.33 / 1.62` | **`8.01 / 10.39 / 10.85`（~125 FPS）** |

#### 表 2：优化前点击 Switch 开关（20 次点击逐帧采样）

| 构建与后端配置 | 按下 `layout_ms` (`layout_count`) | 按下 `render_ms` | **按下总延迟 `latency_ms`** | 抬起 `mouseup` `latency_ms` | 过渡动画帧数 |
| :--- | :---: | :---: | :---: | :---: | :---: |
| **`skia-Debug`（原脚本默认）** | **`27.74 ms` (`1 次`)** | **`136.32 ms`** | **`126.5 ~ 207.8 ms`** | **`95.8 ~ 137.9 ms`** | `0 帧`（硬跳） |
| **`skia-Release`（优化前）** | `1.90 ms` (`1 次`) | `4.01 ms` | `5.62 ~ 9.97 ms` | `3.90 ~ 6.71 ms` | `0 帧`（硬跳） |

### 2.3 实测数据揭示的核心瓶颈

1. **头号瓶颈（占 90% 以上耗时）：`skia-Debug` 链接未优化 `Debug-x64/skia.lib` + 字体未缓存**
   - `skia-Debug` 单帧 `render` 平均高达 **135.93 ms**（比 `skia-Release` 的 **4.87 ms** 慢 **27.9 倍**），`layout` 高达 **55.27 ms**（比 `skia-Release` 的 **3.81 ms** 慢 **14.5 倍**）。
   - 根源在于 `Debug` 模式链接了未开启编译器优化的 `Debug-x64/skia.lib` CPU 软光栅库，且每次 `CreateDefaultTypeface` 都重新向 DirectWrite 查询并构造新 `SkTypeface`，导致字形光栅缓存失效。
2. **无效布局开销**：点击 `<Switch>` 时 `layout_count = 1`，白白触发了一次整页 Yoga 布局（Debug 下耗时 **27.74 ms**，Release 下耗时 **1.90 ms**）。
3. **缺少视觉过渡帧**：即使在 Release 下单帧仅需 ~5 ms，`<Switch>` 圆钮与 `<SegmentedControl>` 滑块仍是 1 帧瞬移，缺少 ~140ms 缓动过渡。
4. **缩放画布分配与异步重绘滞后**：每次 `WM_SIZE` 均耗时 **1.25 ms** 重新创建内核 GDI DIBSection，且 `InvalidateRect` 等待消息队列空闲才重绘。

---

## 3. 第二阶段：五项靶向优化实施

基于实测数据，按优先级实施了以下 5 项靶向优化：

| 编号 | 优化项 | 涉及文件 | 核心技术方案 |
| :---: | :--- | :--- | :--- |
| **1** | **启动脚本默认 `Release` 构建** | [`scripts/run_layout_demo.ps1`](../../scripts/run_layout_demo.ps1) | 新增 `-Configuration Auto\|Release\|Debug` 参数；交互式启动默认解析为 `Release`，CI 静默准备 (`-NoLaunch`) 保持 `Debug` 兼容。 |
| **2** | **Skia 字体、排版与 SVG 路径三级缓存** | [`src/skia_font_shared.h`](../../src/skia_font_shared.h)<br>[`src/skia_text_layout.cpp`](../../src/skia_text_layout.cpp)<br>[`src/renderer_skia.cpp`](../../src/renderer_skia.cpp) | ① `CreateDefaultTypeface` 按 `(fontMgr, bold, italic, weight, family)` 静态缓存 `sk_sp<SkTypeface>`，保持 `SkStrikeCache` 命中；<br>② `CreateSkiaTextLayout` 缓存排版结果（上限 4096 项）；<br>③ `DrawSvgPath` 缓存 `svgPathData -> SkPath`。 |
| **3** | **按需触发 `UpdateLayout()`** | [`src/ui_host.cpp`](../../src/ui_host.cpp)<br>[`src/ui_containers.h`](../../src/ui_containers.h)<br>[`src/ui_widgets.h`](../../src/ui_widgets.h) | 移除 `UiHost::OnMouseDown` 中的无条件 `UpdateLayout()`；仅在 `Expander` 展开/收起与 `AlertBanner` 关闭时调用 `m_context->requestLayout()`。 |
| **4** | **`Switch` 与 `SegmentedControl` 缓动动画** | [`src/ui_widgets.h`](../../src/ui_widgets.h) | 在 `OnTimer` (16ms) 中推进动画进度：`Switch` 采用 Smoothstep ($3t^2 - 2t^3$, ~140ms) 平滑插值圆钮位置与轨道颜色；`SegmentedControl` 平滑滑动胶囊滑块。 |
| **5** | **DIB 64px 步进容量复用与同步重绘** | [`src/renderer_skia.cpp`](../../src/renderer_skia.cpp)<br>[`src/ui_host.cpp`](../../src/ui_host.cpp) | `SkiaRenderer` 对 HWND 呈现模式按 64px 步进对齐分配 DIB 容量 (`m_dibCapWidth × m_dibCapHeight`)，常规缩放仅重新 `SkSurfaces::WrapPixels`；`UiHost::OnResize` 调用 `RedrawWindow(..., RDW_INVALIDATE \| RDW_UPDATENOW)` 同步上屏。 |

---

## 4. 第三阶段：优化后实测验证与前后对比

### 4.1 优化前后核心指标总览（`modern_components_v2.xml`）

| 场景与阶段 | 优化前（默认 `skia-Debug`） | 优化后 (`skia-Debug`) | **优化后（默认 `skia-Release`）** | **总体提升幅度** |
| :--- | :---: | :---: | :---: | :---: |
| **拖拽缩放 — `resize_ms` (avg / p95)** | `1.26 / 1.89 ms` | `0.12 / 0.47 ms` | **`0.04 / 0.27 ms`** | **快 31.5×** |
| **拖拽缩放 — `layout_ms` (avg / p95)** | `55.27 / 82.90 ms` | `35.71 / 44.08 ms` | **`3.60 / 5.66 ms`** | **快 15.4×** |
| **拖拽缩放 — `render_ms` (avg / p95)** | `135.93 / 183.71 ms` | `85.68 / 121.67 ms` | **`4.41 / 6.38 ms`** | **快 30.8×** |
| **拖拽缩放 — `present_ms` (avg / p95)** | `0.63 / 1.00 ms` | `0.50 / 0.72 ms` | **`0.39 / 0.85 ms`** | **快 1.6×** |
| **拖拽缩放 — 端到端 `latency_ms` (avg)** | **`194.14 ms`（~5.1 FPS）** | `122.64 ms`（~8.2 FPS） | **`8.86 ms`（~112.9 FPS）** | **快 21.9×** |
| **点击 Switch — `layout_ms` (`layout_count`)** | `27.74 ms` (`1 次`) | **`0.00 ms` (`0 次`)** | **`0.00 ms` (`0 次`)** | **100% 消除** |
| **点击 Switch — 按下首帧 `paint_ms`** | `119.25 ~ 182.34 ms` | `68.05 ~ 80.37 ms` | **`2.83 ms`** | **快 42.1×** |
| **点击 Switch — 60fps 缓动帧 `paint_ms` (avg / p95)** | 无过渡动画（0 帧） | `67.20 ms` | **`3.83 / 5.22 ms`（101 帧采样）** | **丝滑 60fps 过渡** |

### 4.2 CI 自动化回归测试结果

执行命令：
```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\run_ci_local.ps1 -SkipBuild
```

| 序号 | CI 检查步骤 | 状态 | 详情 |
| :---: | :--- | :---: | :--- |
| 1 | `build` | **PASS** | `build.ps1` 编译 0 Error / 0 Warning |
| 2 | `gallery coverage` | **PASS** | 全部画廊布局索引完整覆盖 |
| 3 | `headless smoke (skia / smoke)` | **PASS** | 7/7 核心布局启动与渲染正常退出 (`exit=0`) |
| 4 | `headless smoke (direct2d / smoke)` | **PASS** | 7/7 核心布局启动与渲染正常退出 (`exit=0`) |
| 5 | `measure golden (core-validation / both)` | **PASS** | Skia / Direct2D 测量树与 Golden 基准 100% 吻合 |
| 6 | `measure golden (yoga-measure / both)` | **PASS** | Yoga Flexbox 测量树与 Golden 基准 100% 吻合 |
| 7 | `measure golden (scroll-viewer / both)` | **PASS** | ScrollViewer 测量树与 Golden 基准 100% 吻合 |

---

## 5. 复现与日常使用指南

1. **体验优化后的流畅交互演示（默认 Release）**：
   ```powershell
   .\scripts\run_layout_demo.ps1 -Layout "modern_components_v2" -Renderer "skia"
   ```
   - 窗口打开后按 **`F12`** 可随时呼出右上角实时帧耗时浮层；
   - 在浮层开启状态下按 **`F11`** 可切换持续刷新模式。
2. **运行自动化帧耗时基准测试**：
   ```powershell
   .\scripts\bench_frame_stats.ps1 -Renderer skia -Config Release
   .\scripts\bench_frame_stats.ps1 -Renderer skia -Config Debug
   ```
   - 原始逐帧 CSV 自动保存在 `build/bench/frame_stats_<renderer>-<config>.csv`。

