# Wave4 架构模块化重构、渲染性能与动效引擎设计文档 (2026-07-26)

本文归档了 `ai-win-ui` 在 Wave4 阶段完成的代码解耦、渲染性能优化、响应式数据绑定与动效 Transition 引擎的系统性设计与落地实现。

---

## 1. 架构模块化解耦 (Phase 1)

### 现状与挑战
随着项目演进，部分源文件膨胀严重：
- `src/layout_parser.cpp` 增长至 191 KB。
- `src/ui_panels.h` (103 KB) 包含了大量内联容器方法。
- `src/ui_host.cpp` (106 KB) 兼具 HWND 消息循环、输入分发与 Overlay 管理。

### 解耦设计
1. **Layout Parser 分解**：
   - 保留 `layout_parser.cpp` 基础 DOM 解析与上下文流转。
   - 抽出 `layout_parser_panels.cpp`（容器节点解析）与 `layout_parser_controls.cpp`（控件节点解析）。
2. **UI 控件与容器分离**：
   - 新增 `src/ui_panels.cpp`，将 Panel, GridPanel, DataTable, SeagullAnimation 的复杂实现函数移入 `.cpp` 文件，只在 `ui_panels.h` 中保留类接口定义。
3. **UiHost 子组件解耦**：
   - 抽取 `ui_host_input.cpp` 处理鼠标、键盘、IME 路由；
   - 抽取 `ui_host_overlay.cpp` 处理 Popup 与 Flyout 的生命周期。

---

## 2. 渲染性能与显存/内存优化 (Phase 2)

### 1. 全局 TextLayout / Paragraph 缓存 (TextLayout Cache)
- 在 `text_measurer.h` & `text_measurer.cpp` 中建立 LRU `CachingTextMeasurer` 装饰器。
- 对 Key `(text, font, fontSize, maxWidth, wrapMode)` 进行哈希，最大缓存 1024 项，避免列表滚动和频繁重排时的 DWrite `IDWriteTextLayout` 或 Skia `SkParagraph` 重建开销。

### 2. 脏区裁切包围盒剔除 (Dirty Rect BBox Culling)
- 在 `IRenderer` 中引入 `SetDirtyRect` / `GetDirtyRect` / `IntersectsDirtyRect`。
- 在 `UIElement::Render` 递归时先进行包围盒相交检测 `!renderer.IntersectsDirtyRect(m_bounds)`，非视区/脏区外的节点直接跳过绘图分支。

### 3. SVG DOM 显存 LRU 淘汰
- 在 `renderer_skia.cpp` 中将 `m_svgCache` 替换为上限 256 项的 `list + map` LRU 结构，确保大量矢量图标渲染时的显存稳定。

---

## 3. 响应式绑定与动效 Transition 引擎 (Phase 3/4)

### 1. `Observable<T>` 响应式属性
- 导出公开头文件 `<ai_win_ui/observable.h>`。
- 支持 `Get()`, `Set(value)`, `Subscribe(listener)` 机制，当属性变量发生修改时，自动通知订阅该属性的 UI 控件触发 `requestLayout` 或 `invalidate`。

### 2. Animation Controller 与缓动算法
- 新增 `src/animation_controller.h`，实现统一插值引擎。
- 支持 `Linear`, `EaseIn`, `EaseOut`, `EaseInOut`, `CubicBezier` 与 `Spring` 弹簧计算。
- 提供 `AnimationController::LerpColor` 与 `LerpFloat`，支持 Hover / Press / Selected 状态切换时的 Color 平滑过渡。

---

## 4. 验证与 CI 防护

所有修改均通过 PowerShell 本地 CI (`scripts/run_ci_local.ps1`) 的 7 项全套自动化测试（包含覆盖率、Skia/D2D Headless Smoke 以及 Core/Yoga/ScrollViewer Golden 测量），确保零回归退化。
