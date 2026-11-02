# 帧耗时剖析器与交互丝滑度优化 (Frame Profiler & Interaction Smoothness)

- **日期**：2026-11-02
- **分支**：`feature/frame-profiler`
- **状态**：已完成并通过 7/7 CI 回归验证

---

## 1. 目标与背景

针对运行 `.\scripts\run_layout_demo.ps1 -Layout "modern_components_v2" -Renderer "skia"` 时拖拽窗口大小与点击 `<Switch>` 等控件出现的明显迟钝感，先构建高精度帧耗时统计器（Frame Profiler）量化各阶段耗时，再根据实测瓶颈实施靶向优化。

---

## 2. 帧耗时统计器 (`src/frame_profiler.h` & `scripts/bench_frame_stats.ps1`)

1. **`ai_win_ui::FrameProfiler` (`src/frame_profiler.h`)**：
   - 基于 `QueryPerformanceCounter` 采集每帧 6 个核心指标：`resize_ms`、`layout_ms`（含 `layout_count`）、`render_ms`、`present_ms`、`paint_ms` 以及从输入事件进入消息循环到上屏完成的端到端 `latency_ms`。
   - **F12 实时监控面板 (`UiHost::DrawFrameStatsOverlay`)**：展示最近 120 帧各阶段的 `avg / p95 / max`（支持 `F11` 切换持续刷新及环境变量 `AI_WIN_UI_DEVTOOLS=1` 启动即显示）。
   - **CSV 逐帧导出 (`AI_WIN_UI_FRAME_STATS=<path>`)**：将每帧触发源（`resize`、`mousedown`、`mousemove`、`mouseup`、`invalidate`）与各阶段耗时写入 CSV。
2. **自动化基准测试脚本 (`scripts/bench_frame_stats.ps1`)**：
   - 自动驱动 60 步连续窗口缩放、60 步鼠标悬停移动与 20 次 `<Switch>` 点击，输出分阶段统计报表。

---

## 3. 五项靶向优化方案

1. **启动脚本默认 `Release` 构建支持 (`scripts/run_layout_demo.ps1`)**：
   - 新增 `-Configuration Auto|Release|Debug` 参数；交互式启动时默认使用 `Release` 构建，消除 `Debug` 下链接未优化 `Debug-x64/skia.lib` 带来的 28 倍软光栅性能惩罚。
2. **Skia 字体实例缓存与文本排版缓存 (`src/skia_font_shared.h`, `src/skia_text_layout.cpp`, `src/renderer_skia.cpp`)**：
   - 在 `skia_font::CreateDefaultTypeface` 中缓存 `(fontMgr, bold, italic, weight, customFamily) -> sk_sp<SkTypeface>`，避免每次文本测量/绘制重复调用 DirectWrite `matchFamilyStyle` 并保持 Skia 字形缓存命中；
   - 在 `CreateSkiaTextLayout` 中增加 4096 项容量上限的排版结果缓存；
   - 在 `SkiaRenderer::DrawSvgPath` 中增加 `svgPathData -> SkPath` 解析缓存。
3. **按需触发 `UpdateLayout()` (`src/ui_host.cpp`, `src/ui_containers.h`, `src/ui_widgets.h`)**：
   - 移除 `UiHost::OnMouseDown` 中无条件执行的整页 `UpdateLayout()`；
   - 仅在改变布局尺寸的组件交互（`Expander` 展开/折叠、`AlertBanner` 关闭隐藏）中通过 `m_context->requestLayout()` 触发重排，使 `Switch`、`SegmentedControl`、`Button` 等点击实现 **0 ms 布局开销**。
4. **`Switch` 与 `SegmentedControl` 缓动过渡动画 (`src/ui_widgets.h`)**：
   - 为 `Switch` 增加 ~140ms Smoothstep 过渡动画（圆钮位置平滑滑动 + 轨道背景色平滑插值）；
   - 为 `SegmentedControl` 增加活动胶囊滑块平滑滑动过渡动画。
5. **拖拽窗口 DIB 容量步进复用与同步重绘 (`src/renderer_skia.cpp`, `src/ui_host.cpp`)**：
   - `SkiaRenderer::RecreateSurface` 按 64px 步进对齐分配 GDI DIB 位图容量（`m_dibCapWidth × m_dibCapHeight`），窗口拖拽中直接基于已有内存缓冲区的行距（`rowBytes = m_dibCapWidth * 4`）包装新尺寸 `SkSurface`，避免每 1px 调用 `CreateDIBSection`；
   - `UiHost::OnResize` 在可见窗口上使用 `RedrawWindow(..., RDW_INVALIDATE | RDW_UPDATENOW)` 立即同步绘制，消除拖拽边框时的画面滞后。

---

## 4. 优化前后实测对比（`modern_components_v2.xml`）

| 指标 | 优化前 (`skia-Debug` 默认) | 优化后 (`skia-Debug`) | 优化后 (`skia-Release` 默认) | 提升幅度（默认启动体验） |
| :--- | :---: | :---: | :---: | :---: |
| **拖拽 `resize` 画布分配 (avg)** | `1.26 ms` | `0.12 ms` | **`0.04 ms`** | **快 31.5×** |
| **拖拽 `layout` 布局 (avg / p95)** | `55.27 / 82.90 ms` | `35.71 / 44.08 ms` | **`3.60 / 5.66 ms`** | **快 15.4×** |
| **拖拽 `render` 渲染 (avg / p95)** | `135.93 / 183.71 ms` | `85.68 / 121.67 ms` | **`4.41 / 6.38 ms`** | **快 30.8×** |
| **拖拽端到端 `latency` (avg)** | `194.14 ms` (~5 FPS) | `122.64 ms` | **`8.86 ms` (~113 FPS)** | **快 21.9×** |
| **点击 Switch 触发 `layout_ms`** | `27.74 ms` (`x1`) | **`0.00 ms` (`x0`)** | **`0.00 ms` (`x0`)** | **100% 消除** |
| **点击 Switch 单帧 `paint_ms` + 动画** | `119.25 ms`（无动画瞬移） | `68~80 ms` | **`2.83~3.83 ms`（60fps 缓动）** | **快 31× + 丝滑缓动** |

