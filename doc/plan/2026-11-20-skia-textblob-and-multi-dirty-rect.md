# Skia 文本 SkTextBlob 缓存与多脏矩形渲染优化方案

- **日期**：2026-11-20
- **分支**：`feature/skia-textblob-dirty-region`（基于 `feature/react-jsx-ui` @ `12fa270`）
- **状态**：已落地并完成 Release 实测对比，Unit Test（`4/4` + `3/3`）与 `run_ci_local.ps1`（`7/7`）全部 **PASS**

---

## 1. 背景与定位

在 Release 基准评测中，虽然消除了 `Debug-x64/skia.lib` 的 ~28× 软光栅惩罚（如 `modern_components_v2.xml` 从 `Debug render 135.93 ms` 降至 `Release render 4.41 ms`），但在高频局部更新、快速连续输入、以及多角离散行刷新场景下，Skia 后端仍存在两类结构性开销：

1. **单帧文本路径存在重复跨编码转换与字形构型开销**：
   - `skia_font::MeasureTextWidthWithFallback` 对每个字符执行 UTF-16 -> UTF-8 转换及逐字符 `unicharToGlyph` / `measureText`；
   - `DrawTextW` 对不含换行且宽度充足的单行短文本仍分配/构建 `SkiaTextLayout`（按值拷贝与非真 LRU 淘汰），绘制阶段每次调用 `drawSimpleText` 重复走字形查询与栅格前准备工作；
   - 行度量（`FontMetrics`）与虚线（`SkDashPathEffect`）在高频重绘中重复构建。
2. **脏区合并不保留离散空隙，局部刷新仍误伤中间整块区域**：
   - `UiHost` 仅维护单个 `m_pendingDirtyRect`，当窗口左上/右下或盘口多角出现离散更新时，单一外接矩形（Bounding Box）会把中间庞大的静态 K 线图与未变化控件一并拉入重绘（导致大面积误重绘）；
   - `UiHost::OnPaint` 仅读取 `PAINTSTRUCT::rcPaint`（单一合并外接框），未在 `BeginPaint` 前提取 Win32 更新区域的离散矩形集合；
   - 大多数控件 `Render()` 缺少 `renderer->IntersectsDirtyRect(...)` 早期剔除；`TextInput` / `RichTextBox` 光标闪烁直接触发整窗 `Invalidate()`；高频行情批更新未包裹在 `BeginBatchUpdate()` / `EndBatchUpdate(false)` 中；F12 DevTools live HUD 每次刷新重绘全屏。

---

## 2. 核心架构设计

### 2.1 多脏矩形抽象与裁剪上屏 (`DirtyRegion` + `SkRegion` + Per-Rect `BitBlt`)

1. **`ai_win_ui::DirtyRegion` (`src/dirty_region.h`)**：
   - 定义可控规模的多脏矩形容器：`kMaxRects = 8`、`kAdjacentMergeWasteArea = 2048.0f`、`kFullRepaintAreaRatio = 0.85f`；
   - 提供 `AddRect`、`NormalizeForViewport`、`Intersects`、`TotalArea`、`CoverageRatio`：
     - 相邻微间距合并（允许废区 `<= 2048 px²` 时合并，减少碎片）；
     - 超过 `kMaxRects = 8` 时按最小面积增量代价贪心归并，确保下游裁剪复杂度 `O(kMaxRects)` 封顶；
     - 总覆盖面积超过视口 `85%` 时自动退化为全帧重绘，避免裁剪与多矩形 `BitBlt` 自身开销反超全帧路径。
2. **`IRenderer` 多脏矩形接口 (`src/renderer.h`)**：
   - 新增 `SetDirtyRects(const std::vector<Rect>&)`、`GetDirtyRects()`；
   - `IntersectsDirtyRect(const Rect&)` 先做总包围盒 `O(1)` 快速拒绝，再遍历脏矩形列表做精确相交检测（支持无脏区/全帧模式直接返回 `true`）。
3. **Skia 渲染后端多矩形裁剪与局部上屏 (`src/renderer_skia.cpp`)**：
   - **`BeginFrame` 裁剪**：当 `m_dirtyRects` 非空时构造 `SkRegion`，通过 `canvas->clipRegion(clipRegion)` 替代单矩形 `clipRect`，精确跳过离散脏块之间的静止像素；
   - **图元级 Early-Out**：在 `FillRect`、`FillRoundedRect`、`FillRRect`、`FillLinearGradient`、`FillSoftShadow`、`DrawRect`、`DrawRoundedRect`、`DrawRRect`、`DrawLine`、`DrawDashedLine`、`DrawTextW` 内接入 `IntersectsDirtyRect` 早期退出；
   - **局部 `BitBlt` 上屏 (`PresentHwnd`)**：Skia 具备离屏持久 DIB Surface，当存在局部脏矩形时逐矩形执行 `BitBlt`，避免全屏像素拷贝；
   - **严格遵守 Direct2D 全帧不变式（Rule #3）**：Direct2D 使用 DXGI 双缓冲交换链，绝不启用局部脏矩形裁剪（保持 `SetDirtyRects({})` 与全帧提交）。
4. **宿主收集、Win32 区域还原与批更新 (`src/ui_host.cpp`, `samples/stock_terminal/main.cpp`)**：
   - `UiHost` 将 `m_pendingDirtyRect` 升级为 `ai_win_ui::DirtyRegion m_pendingDirtyRegion`；
   - `OnPaint()` 在调用 `BeginPaint()`（会销毁并合并 update region）**之前**通过 `GetUpdateRgn` + `GetRegionData` 还原 Win32 离散更新矩形列表；
   - 抽出 `ComputeDevToolsOverlayRect(float pad = 0.0f)`，F12 实时统计 HUD 在 Skia 下仅标脏右上角自身区域，而非整窗全刷；
   - `SetElementBackground`、`SetElementForeground` 及 `*FinancialChart*` 接口对齐 `m_batchUpdateDepth` 保护；
   - `WM_CHAR` / `WM_IME_CHAR` / `WM_UNICHAR` 仅标脏当前聚焦控件 `m_focusedElement->InvalidateSelf(6.0f)`；
   - `samples/stock_terminal/main.cpp` 的 `onDataUpdated`（`AppendFinancialChartPoint` + `UpdateUiFromSnapshot`）统一包裹在 `BeginBatchUpdate()` / `EndBatchUpdate(false)` 中。

### 2.2 Skia UTF-16 直通度量、零拷贝排版 LRU 与 `SkTextBlob` 缓存

1. **UTF-16 零转码字体度量 (`src/skia_font_shared.h`)**：
   - `MeasureTextWidthWithFallback` 改为批量调用 `SkFont::textToGlyphs(..., SkTextEncoding::kUTF16)` 与 `SkFont::getWidths`（128 项栈上缓冲区，超长回退堆分配），未命中缺字时按需通过 `MatchTypefaceForCharacter` 与 UTF-16 `measureText` 回退；
   - 完全消除 `WideCharToMultiByte` UTF-8 临时分配及逐字符 `unicharToGlyph` 循环，同时保持 `Segoe UI` 与 CJK fallback 行为严格一致。
2. **零拷贝真 LRU `SkiaTextLayout` 缓存 (`src/skia_text_layout.h`, `src/skia_text_layout.cpp`)**：
   - 新增 `AcquireSkiaTextLayout(...)` 返回 `std::shared_ptr<const SkiaTextLayout>`，由 4096 项真 LRU 缓存（`std::list` + `std::unordered_map`）托管，并复用 `s_lookupKey` 消除查询期堆分配与整份结构体拷贝；
   - 修复 `TextLayoutCacheKeyHash` 漏混入 `lineHeightFactorQ` 的哈希碰撞缺陷。
3. **`SkTextBlob` LRU 缓存与单行快路径 (`src/renderer_skia.cpp`)**：
   - 移除 `DrawSimpleTextWithFallback`，在 `SkiaRenderer` 内新增 4096 项容量的 `SkTextBlob` 真 LRU 缓存（使用 `SkTextBlobBuilder::allocRunPosH` 批量构建 UTF-16 单 run 及 CJK fallback 多 run 字形定位）；
   - **成员生命周期安全**：`m_textBlobLruMap` / `m_textBlobLruList` / `m_fontMetricsCache` / `m_dashEffectCache` 声明在 `m_fontMgr` 与 `m_defaultTypeface` 之后，且在 `~SkiaRenderer()` 中显式清理；
   - **单行无换行快路径**：`DrawTextW` 当 `!hasNewline` 且实测宽度未超出布局盒约束（无需省略号与折行）时，直接跳过 `SkiaTextLayout` 解析，命中 `SkTextBlob` 缓存直达 `canvas->drawTextBlob(...)`；文本裁切改用非抗锯齿 `clipRect(..., false)`；
   - 同步缓存 `SkFontMetrics` 与 `SkDashPathEffect` 实例。

### 2.3 全控件脏矩形剔除与精确光标闪烁

- 为核心控件库补齐 `renderer->IntersectsDirtyRect(...)` 早期跳过逻辑：
  - `src/ui_financial_chart.h`（`FinancialChart::Render`）
  - `src/ui_inputs.h`（`Button`、`TextInput`、`CheckBox`、`RadioButton`、`Slider`）
  - `src/ui_text.h`、`src/ui_panels.h`（`GridPanel`、`StatCard`、`LineChart`、`ShapePanel`、`Table`、`SeagullAnimation`）
  - `src/ui_lists.h`、`src/ui_media.h`、`src/ui_menus.h`、`src/ui_nav.h`、`src/ui_pickers.h`、`src/ui_views.h`
  - `src/ui_overlays.h`（仅在 `Popup::Render` 剔除，刻意保留 `RenderOverlay` 不误裁浮层弹出窗口）
- 修复光标定时器整窗重绘问题：`TextInput::OnTimer` 与 `RichTextBox::OnTimer`（`src/ui_inputs.h`, `src/ui_text.h`）由全窗标脏改为 `InvalidateSelf(4.0f)`。

### 2.4 性能剖析指标与自动化回归体系

1. **`src/frame_profiler.h`**：在 CSV 日志末尾新增 `dirty_rects,dirty_area_pct` 两列与 `SetDirtyInfo(int, double)` 接口，量化每帧裁剪块数与重绘面积占比。
2. **`samples/skia_unit_test/main.cpp` + `CMakeLists.txt`**：新增 4 项 Skia 专项单元测试：
   - `TestDirtyRegionDisjointCornersAndCulling`
   - `TestDirtyRegionGridCoalescingAndCap`
   - `TestRendererDirtyRectsInterface`
   - `TestSkiaTextUtf16ParityAndLruCache`
3. **`scripts/bench_skia_dirty_region.ps1`**：新增 Day-K（日 K 线）500ms 分散四角更新 Release 自动化压测脚本。

---

## 3. 验证状态

- `.\build.ps1 -Configuration Release` & `.\build.ps1 -Configuration Debug`：0 Error / 0 Warning。
- `.\build\Release\skia_unit_test.exe` & `.\build\Debug\skia_unit_test.exe`：**4/4 PASS**。
- `.\build\Release\react_unit_test.exe` & `.\build\Debug\react_unit_test.exe`：**3/3 PASS**。
- `.\scripts\run_ci_local.ps1 -SkipBuild`：**7/7 PASS**（48.0s，含 Skia 与 Direct2D 双后端 `core-validation` / `yoga-measure` / `scroll-viewer` Measure Goldens）。
- Release 优化前后对比及 Day-K 多脏矩形专项实测见 [`doc/reports/2026-11-20-skia-opt-before-after-report.md`](../reports/2026-11-20-skia-opt-before-after-report.md)。

---

## 4. Phase 2 后续演进方向

1. **鼠标悬停 (`mousemove`) 局部脏区精确标脏**：目前 K 线十字光标移动与 hover 状态仍触发全窗重绘，后续可将 K 线十字光标与状态栏高亮拆分为独立脏矩形，进一步压低 `hover60Hz` 与 `periodClick` 的重绘面积。
2. **React 日 K 线 500ms Tick 局部更新粒度收窄**：在 Day-K 模式下跳过无关贡献度/提示条与冗余 chart props 变更，将 React Day-K 的脏面积占比从 `~56.7%` 进一步收敛至与 XML 对齐的 `<5%`。
