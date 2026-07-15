# Plan: DirectUI P0 Hardening（引擎可信）

## Context

`ai-win-ui` 已具备 retained UI 树、Yoga、双后端、样式 catalog、大量控件与 layered 异形窗。
相对「可产品化的 DirectUI 风格引擎」，P0 不追求控件数量，而追求：

1. **文本测量与绘制可信**（Skia ↔ DirectWrite）
2. **可复用的滚动容器**（离开 App 级硬编码竖滚）
3. **进程内多根窗口**（不再依赖 CreateProcess 才能「多窗」）
4. **最小自动化回归**（load + measure 黄金文件）

本计划与 `doc/mvp-execution-plan.md` Phase 1（Skia 文本）对齐，并扩展滚动 / 多窗 / 测试。

**非目标（P0 不做）**：完整数据绑定、UIA、渐变阴影、CSS 级联、虚拟化最终形态、控件大拆分完成。

---

## Track A — Skia / DirectWrite 文本 parity

### 为何优先

布局高度依赖 `ITextMeasurer`；Skia 画、DirectWrite 量或规则不一致会导致裁剪、基线、焦点框漂移。

### 主文件

| 文件 | 职责 |
|------|------|
| `src/skia_text_layout.cpp` / `.h` | Skia 分行 / 行高 / 对齐 |
| `src/skia_font_shared.h` | typeface / glyph 回退 |
| `src/text_measurer_skia.cpp` | Skia 测量实现 |
| `src/text_measurer_dwrite.cpp` | DirectWrite 测量（对照基准） |
| `src/text_measurer.h` | 统一选项：wrap / bold / fontSize |
| `src/renderer_skia.cpp` | `DrawTextW` 必须与 measurer 同规则 |
| `src/renderer.cpp` | D2D 绘制对照 |
| `src/ui_text_metrics.*` | 控件共用度量 helper |
| `resource/layouts/cjk_render_test.xml` | CJK / 多行对照页 |
| `resource/layouts/core_validation.xml` | 综合验收 |

### 任务拆解

| ID | 任务 | 完成判据 |
|----|------|----------|
| A1 | 列出测量与绘制共用的「单一规则表」：NoWrap/Wrap、lineHeight、ellipsis 有无、垂直对齐 | 文档表 + 代码注释锚定同一结构体字段 |
| A2 | Skia measure 与 Skia draw 走同一 `skia_text_layout` API（禁止 draw 内自算分行） | `renderer_skia` 无独立 wrap 逻辑 |
| A3 | Label / Button / TextInput 单行：Skia vs D2D 同布局页，行高误差 ≤ 2px（96 DPI） | `cjk_render_test` + `core_validation` 双后端肉眼 + 可选 dump |
| A4 | 多行 Label wrap：断行点与 measured height 一致（无「量高 3 行画 2 行」） | 固定字符串 golden height |
| A5 | TextInput caret / selection 矩形用同一 metrics | 选区不偏移 |
| A6 | （可选）导出 `AI_WIN_UI_TEXT_DUMP=1` 打印 measure 矩形，便于 CI 后续接 | Debug 输出稳定格式 |

### 建议顺序

A1 → A2 → A3 → A4 → A5 → A6

### 风险

- CJK 字体回退两边不同 → 先固定 face 名（如 Segoe UI / Microsoft YaHei UI）再比布局。
- 不要在 A 阶段改 Yoga 规则，避免变量混淆。

---

## Track B — 通用 `ScrollViewer`

### 为何

今日竖滚在 `main.cpp`（`m_scrollOffset` + 系统 SB_VERT + 根 Arrange 平移）。ListView/TreeView 自有 pill 条。嵌套滚动与「页面一块可滚区域」无法表达。

### 主文件

| 文件 | 职责 |
|------|------|
| `src/ui.h`（或新 `src/scroll_viewer.h` 若拆分） | 新 `ScrollViewer` 控件 |
| `src/layout_parser.cpp` | 解析 `ScrollViewer` |
| `src/main.cpp` | 滚轮：优先命中控件 `OnMouseWheel`；**无命中再**走窗口级滚（兼容旧布局） |
| `resource/layouts/scroll_viewer_cases.xml` | 嵌套 / 超高内容验收 |
| `doc/layout-spec.md` | 属性文档 |

### 控件最小 API

```
ScrollViewer
  props: scrollY (default true), scrollX (default false),
         showScrollbars (default true), barThickness
  children: 恰好一个内容根（或多个时包一层 Panel）
  behavior:
    - Measure: 视口 = 自身约束；内容 unrestricted 或 max 轴
    - Arrange: 内容按 offset 平移
    - OnMouseWheel / 拖条
    - PaintPillScrollbars 复用现有路径
```

### 任务拆解

| ID | 任务 | 完成判据 |
|----|------|----------|
| B1 | `ScrollViewer` Measure/Arrange + 单轴 Y | 超高 Panel 可滚，clip 正确 |
| B2 | 滚轮命中与冒泡：子控件先消费，否则 ScrollViewer | ListView 内滚不带动外层（可测） |
| B3 | 可选 X 轴 + Shift+wheel | `scrollX=true` 用例通过 |
| B4 | 解析 XML/JSON + demo 布局 | `scroll-viewer` 别名可启动 |
| B5 | 窗口级 `m_scrollOffset` 标记为 legacy：新 demo 用 ScrollViewer | 文档说明；旧 layout 不破 |

### 虚拟化（P0 仅雏形，可选 B6）

| ID | 任务 | 完成判据 |
|----|------|----------|
| B6 | `ListBox` 或 `ListView`：仅创建可见行（+ overscan） | 1k 行不掉到不可用；API 可先 C++ `SetItemCount` + binder 回调 |

B6 可 defer 到 P0.5；B1–B5 为硬门槛。

---

## Track C — 进程内多根窗口（Multi-root）

### 为何

`WM_DESTROY` → `PostQuitMessage` 使同进程多窗困难；异形窗用 `CreateProcess` 绕过。DirectUI 引擎应支持 **N 个 Host × 各一棵 UI 树**。

### 主文件

| 文件 | 职责 |
|------|------|
| `src/main.cpp` | 将 `App` 拆为 `UiHost`（单窗）+ `Application`（进程/消息循环） |
| 新 `src/ui_host.h` / `.cpp`（推荐） | HWND、renderer、root、chrome、input |
| `src/ui_context.h` | 每 host 一份 context（勿全局单例） |
| event 层 | `openHeartWindow` 改为 `Application::OpenHost(layout, chrome, size)` 默认同进程；env `AI_WIN_UI_CHILD_PROCESS=1` 保留旧行为 |

### 任务拆解

| ID | 任务 | 完成判据 |
|----|------|----------|
| C1 | 抽出 `UiHost`：Initialize / WndProc / Paint / 输入 | 现有单窗行为零回归 |
| C2 | `Application`：host 列表、`Run` 消息泵、`last host destroy` 才 `PostQuitMessage` | 开 2 窗，关一扇另一扇仍在 |
| C3 | `OpenHost(layoutPath, chrome, size, renderer?)` | shaped hub 按钮同进程开窗 |
| C4 | 样式/主题：可共享 `StyleCatalog`/`Theme` 只读，或每 host 加载一次 | 子窗 `$style` 仍可用 |
| C5 | 更新 `doc/window-chrome.md`：默认同进程；多进程为可选 | 文档与 demo 一致 |

### 风险

- 静态 `WndProc` / 类名注册：每 host 用 `GWLP_USERDATA` 指 `UiHost*`（已有模式则复用）。
- Layered + 多 RT：确认 Skia/D2D 每 HWND 独立 renderer 实例。

---

## Track D — 最小自动化（load + measure golden）

### 为何

现仅有 `AI_WIN_UI_QUIT_AFTER_MS` 开窗 smoke，无法捕度量回归。

### 主文件

| 文件 | 职责 |
|------|------|
| 新 `tools/layout_measure_dump.cpp` 或 `src` 内 `#ifdef` 工具目标 | 无 GUI 或隐藏窗：BuildFromFile → Measure → dump |
| `CMakeLists.txt` | 可选 target `ai_win_ui_measure` |
| `tests/golden/*.json` | 期望宽高（按控件 Name/路径） |
| `scripts/run_measure_golden.ps1` | 跑 dump + diff |
| `src/ui.h` | 可选 `name`/`id` 属性便于路径（若尚无） |

### 任务拆解

| ID | 任务 | 完成判据 |
|----|------|----------|
| D1 | 元素 `name`/`id` 解析（XML attr + JSON prop） | 树可寻址 |
| D2 | Dump 格式：JSON 行 `{ "path": "root/panel0/label1", "w":.., "h":.. }` | 稳定、排序 |
| D3 | Golden：`yoga_measure_cases` + `core_validation` 子集 | 脚本 fail 当 diff > 1px |
| D4 | CI 本地脚本：`run_measure_golden.ps1 -Backend both` | 双后端可选阈值 |

D 不依赖真实像素截图；只锁 **布局度量**。

---

## 推荐执行顺序（串行主线）

```
Week 概念序（可按人力并行 A∥D 起步）:

  1) A1–A2     文本规则统一
  2) D1–D2     name + dump 骨架（立刻服务 A）
  3) A3–A5     对照调 parity；D3 录入 golden
  4) B1–B5     ScrollViewer
  5) C1–C5     Multi-root
  6) B6        虚拟化雏形（可选）
  7) 文档      layout-spec / window / README 同步
```

并行建议：

- 一人 A+D，一人 C 骨架，B 在 C1 完成后合入（滚轮路由清爽）。

---

## 完成定义（P0 Done）

- [x] 文本规则文档化；Skia measure/draw 共享 `CreateSkiaTextLayout`（含 bold/italic）；DWrite NoWrap 首行 parity（2026-07-15 起步）
- [x] `ScrollViewer` + `scroll_viewer_cases` + 解析（2026-07-15）
- [ ] shaped hub **默认同进程** 开第二窗，关子窗 hub 仍在（Track C 未做）
- [x] `name` + `AI_WIN_UI_MEASURE_DUMP` / `run_measure_dump.ps1`（golden 对比脚本后续）
- [x] `doc/layout-spec.md` 已记 ScrollViewer / name / dump

## Progress log

### 2026-07-15 — first code pass

- **A1/A2**: `skia_text_layout.h` 规则表；`CreateSkiaTextLayout(..., bold, italic)`；`renderer_skia` draw 传入 bold/italic；DWrite NoWrap 只量首行。
- **B1–B4**: `ScrollViewer` 控件 + XML/JSON 解析 + `scroll_viewer_cases.xml` + 别名 `scroll-viewer`。
- **D1/D2**: `UIElement::name`；`AI_WIN_UI_MEASURE_DUMP` NDJSON dump；`scripts/run_measure_dump.ps1`。
- **C**: 未开始（仍 CreateProcess 异形窗）。

---

## 与后续 P1 的接口（避免返工）

| P0 决策 | 留给 P1 |
|---------|---------|
| ScrollViewer 单内容槽 | Popup 层、模态焦点 |
| OpenHost 显式 API | 数据绑定、命令路由 |
| name 路径 dump | UIA AutomationId 映射 |
| 文本规则结构体 | 富文本 / IME 合成窗 |

---

## 相关文档

- `doc/mvp-execution-plan.md` — 原 MVP 收尾顺序
- `doc/mvp-roadmap.md` — 缺口总览
- `doc/skia-integration.md` — Skia 限制
- `doc/layout-spec.md` — DSL
- `doc/window-chrome.md` — 多窗 / layered
