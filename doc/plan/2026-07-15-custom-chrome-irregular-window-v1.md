# Plan: Custom Window Chrome & Irregular Window v1

## Context

当前 `ai-win-ui` 使用标准系统框架窗口：

- 创建样式：`WS_OVERLAPPEDWINDOW | WS_VSCROLL`（`src/main.cpp`）
- 客户区由 Direct2D / Skia 绘制 retained UI 树
- 消息循环覆盖 resize / mouse / keyboard / paint / DPI，**没有**非客户区定制

目标能力：

1. **自定义 Title Bar**：去掉系统标题栏，用 UI 树绘制标题栏与 min/max/close，并保留拖动、缩放、最大化等系统交互语义。
2. **不规则窗体（分档）**：先支持视觉圆角 / 无边框壳；真异形 + 每像素透明作为后续阶段，不阻塞 v1 主路径。

本计划是**可分支落地的任务计划书**：先定边界与 PR 切分，再在独立分支上实现与验收。

---

## 目标与非目标

### 目标（v1 必须交付）

- Borderless（或等价无系统标题栏）可运行窗口
- UI 自绘 Title Bar（图标、标题、min / max-restore / close）
- 标题栏空白区域可拖动；双击标题栏可最大化 / 还原
- 窗口四边 / 四角可缩放（DPI 相关边框宽度）
- 最大化时不遮挡任务栏（工作区钳制）
- 任务栏按钮、Alt+Space 系统菜单仍可用
- 示例 / 验证布局可演示自定义 chrome
- 文档说明启用方式与限制

### 明确不做（v1 边界）

| 项 | 原因 |
|---|---|
| ❌ Layered `UpdateLayeredWindow` 真异形 / 每像素 Alpha 主路径 | 需替换 Present 管线，工作量大，单独 v2 |
| ❌ 任意多边形 `SetWindowRgn` 作为主方案 | 硬裁切、无抗锯齿，仅作可选实验 |
| ❌ 多显示器边缘吸附以外的 Aero Snap 定制 | 依赖系统默认即可 |
| ❌ 自定义系统菜单外观 | 保留原生 `SC_*` 即可 |
| ❌ 去掉全部系统滚动条并重做滚动语义 | 与 chrome 解耦；v1 建议**关闭系统 `WS_VSCROLL`**，继续用现有客户区滚动逻辑 |
| ❌ 运行时热切换 chrome 模式（有框 ↔ 无框） | v1 启动时固定；后续可加 |
| ❌ 把 Window chrome 做成跨平台抽象 | 本仓库仅 Win32 |

---

## 替你拍板的设计决策

| 决策 | 选择 | 理由 |
|---|---|---|
| 主实现路径 | **无边框客户区扩展 + `WM_NCHITTEST` + UI TitleBar** | 业界标准；兼容现有 HWND D2D/Skia 渲染，无需改 Present |
| 窗口样式 | `WS_POPUP \| WS_THICKFRAME \| WS_MINIMIZEBOX \| WS_MAXIMIZEBOX \| WS_SYSMENU`；可选保留可调大小 | 去掉 caption，保留缩放与系统菜单语义 |
| 非客户区 | `WM_NCCALCSIZE` 使客户区铺满整窗 | 标题栏完全由 UI 绘制 |
| 拖动命中 | 标题栏空白 → `HTCAPTION`；交互控件 → `HTCLIENT` | 系统处理拖动 / 双击最大化 |
| 缩放命中 | 客户区边缘 N px（DPI 缩放）→ `HTLEFT` / `HTTOP` / … | 无系统边框时必须自实现 |
| 系统按钮 | UI `Button` + click 转发 `ShowWindow` / `WM_CLOSE`；**不强制** `HTMINBUTTON` 等 | 样式完全可控；避免与自绘按钮双重命中 |
| 命中区域来源 | 布局后的元素矩形 + 显式属性 `caption` / `hitTest` | 与资源驱动模型一致，避免在 App 里写死坐标 |
| DWM | v1 启用：`DwmExtendFrameIntoClientArea`（轻量）+ Win11 `DWMWA_WINDOW_CORNER_PREFERENCE` 圆角 | 阴影 / 圆角成本低；失败则降级为直角 |
| 启用方式 | 环境变量 `AI_WIN_UI_CHROME=custom` **或** 根布局属性 `chrome="custom"` | 默认保持系统标题栏，避免破坏现有 demo / 验证脚本 |
| 默认行为 | **默认仍为系统 chrome**；custom 为 opt-in | 降低回归风险 |
| 不规则 v1 | **视觉圆角 + DWM 圆角**；真透明异形进 v2 | 先交付可用产品形态 |
| 滚动条 | custom chrome 时**不创建 `WS_VSCROLL`** | 避免系统非客户区滚动条破坏无框观感 |
| 代码落点 | 新增 `WindowChrome` 辅助模块，App 只做接线 | 避免 `main.cpp` 继续膨胀 |

---

## 架构落点

```
┌──────────────────────────────────────────────────────────┐
│  App (main.cpp)                                          │
│  - 按 chrome 模式创建窗口样式                               │
│  - 转发 NC 消息给 WindowChrome                           │
│  - 系统按钮命令（min / max / close）                      │
└────────────────────────────┬─────────────────────────────┘
                             │
┌────────────────────────────▼─────────────────────────────┐
│  WindowChrome (new)                                      │
│  - 模式：System / Custom                                 │
│  - WM_NCCALCSIZE / WM_NCHITTEST / WM_GETMINMAXINFO       │
│  - 边框宽度（DPI）、最大化工作区钳制                      │
│  - DWM 初始化（可选）                                    │
│  - 从 UI 树收集 caption / client-only 区域               │
└────────────────────────────┬─────────────────────────────┘
                             │ 布局完成后更新命中矩形
┌────────────────────────────▼─────────────────────────────┐
│  UI 树 + LayoutParser                                    │
│  - TitleBar 区域（caption）                              │
│  - 按钮 hitTest=client                                   │
│  - 内容区                                                │
└────────────────────────────┬─────────────────────────────┘
                             │ 不变
┌────────────────────────────▼─────────────────────────────┐
│  IRenderer (HWND path)                                   │
│  - v1 不改 Present；客户区仍是整窗矩形                    │
└──────────────────────────────────────────────────────────┘
```

### 核心类型（草案）

```cpp
// src/window_chrome.h
#pragma once
#include <Windows.h>
#include <vector>

enum class WindowChromeMode {
    System,  // 默认：WS_OVERLAPPEDWINDOW 行为
    Custom,  // 无系统标题栏 + UI chrome
};

struct WindowChromeHitRegion {
    RECT rect{};           // 客户区坐标
    bool caption = false;  // true → HTCAPTION（若未被 client 覆盖）
    bool clientOnly = false; // true → 强制 HTCLIENT（按钮、输入框）
};

class WindowChrome {
public:
    void SetMode(WindowChromeMode mode);
    WindowChromeMode Mode() const;

    void SetDpi(UINT dpi);
    void SetHitRegions(std::vector<WindowChromeHitRegion> regions);

    // 创建窗口时使用
    DWORD WindowStyle() const;
    DWORD WindowExStyle() const;

    bool InitializeDwm(HWND hwnd);
    LRESULT HandleNcCalcSize(HWND hwnd, WPARAM wParam, LPARAM lParam, bool& handled);
    LRESULT HandleNcHitTest(HWND hwnd, LPARAM lParam, bool& handled);
    LRESULT HandleGetMinMaxInfo(HWND hwnd, LPARAM lParam, bool& handled);

    int ResizeBorderPx() const;   // DPI-aware
    int TitleBarHeightDip() const; // 可选默认值；真实高度以 UI 为准
};
```

### 布局属性（v1）

在现有 XML / JSON 解析中增加可选属性（未知属性忽略，保持向后兼容）：

| 属性 | 作用域 | 语义 |
|---|---|---|
| `chrome="system\|custom"` | 根节点或 `Window` 元数据 | 覆盖启动 chrome 模式（若环境变量未强制） |
| `hitTest="caption\|client\|default"` | 任意元素 | `caption`：可拖动；`client`：强制客户区交互 |
| `role="titleBar"`（可选） | Panel | 语义标记；默认整区 caption，子控件默认 client |

解析后写入 `UIElement` 标志位，例如：

```cpp
enum class HitTestRole : uint8_t {
    Default = 0,
    Caption = 1,
    Client = 2,
};
```

布局 / 每帧更新后，App 遍历可见树，把最终 `bounds` 转成 `WindowChromeHitRegion` 列表：

- `Caption` 区域 → `caption=true`
- `Client` 或可交互控件（Button / TextInput / …）→ `clientOnly=true`
- `HandleNcHitTest`：**先**匹配 clientOnly，**再**匹配 caption，**再**边缘缩放，最后 `HTCLIENT`

---

## 改动文件（按 PR 阶段）

### PR1 — WindowChrome 内核 + 无框窗口

| 文件 | 改动 |
|---|---|
| `src/window_chrome.h` | 新增 |
| `src/window_chrome.cpp` | 新增：样式、NCCALCSIZE、NCHITTEST、MINMAXINFO、DPI 边框 |
| `src/main.cpp` | 接线创建样式与 NC 消息；opt-in 开关 |
| `CMakeLists.txt` | 加入 `window_chrome.cpp`；链接 `dwmapi` |
| `doc/plan/...` | 本计划（已存在） |

### PR2 — UI TitleBar + 系统按钮 + 命中区域收集

| 文件 | 改动 |
|---|---|
| `src/ui.h` / 相关 | `HitTestRole` 标志；遍历收集 bounds |
| `src/layout_parser.*` | 解析 `hitTest` / `role` / 根 `chrome` |
| `src/main.cpp` | 布局后刷新 hit regions；min/max/close 命令 |
| `resource/layouts/custom_chrome_demo.xml`（+ json 可选） | 演示布局 |
| `resource/themes/default.json` | 标题栏 token（可选） |

### PR3 — DWM 圆角 / 阴影降级 + 验证 + 文档

| 文件 | 改动 |
|---|---|
| `src/window_chrome.cpp` | DWM 初始化与失败降级 |
| `scripts/run_custom_chrome_demo.ps1` | 启动脚本 |
| `doc/window-chrome.md` | 使用说明与限制 |
| `doc/mvp-roadmap.md` 或 `DEVELOPMENT_LOG.md` | 状态记录 |

### 后续（v2，不在本分支必须范围）

| 主题 | 内容 |
|---|---|
| Layered Present | 离屏渲染 + `UpdateLayeredWindow`；真透明 / 异形 |
| Region 实验 | `SetWindowRgn` 圆角硬裁切开关 |
| 属性化 Window 根节点 | `<Window chrome=... cornerRadius=...>` 正式 schema |

---

## 分支与落地策略

### 分支模型

```
master
  └── feature/custom-window-chrome-v1     # 总集成分支
        ├── pr/chrome-1-window-host      # PR1
        ├── pr/chrome-2-titlebar-ui      # PR2
        └── pr/chrome-3-dwm-docs         # PR3
```

若团队不用堆叠 PR，可在单分支 `feature/custom-window-chrome-v1` 上按 commit 阶段推进，合并前 squash 或保留清晰 commit。

### 落地顺序

1. **从 `master` 拉分支** `feature/custom-window-chrome-v1`
2. 按 PR1 → PR2 → PR3 提交；每阶段可独立编译运行
3. 每阶段跑现有验证脚本，确认 **默认 system chrome 无回归**
4. custom 模式用新 demo 布局人工验收
5. 合并回 `master` 前更新 `DEVELOPMENT_LOG.md`

### 启用方式（验收与演示）

```powershell
# 环境变量强制 custom
$env:AI_WIN_UI_CHROME = "custom"
.\build\Release\ai_win_ui.exe
# 或 demo 脚本
.\scripts\run_custom_chrome_demo.ps1
```

优先级建议：

1. 环境变量 `AI_WIN_UI_CHROME`（最高，便于脚本）
2. 根布局 `chrome="custom"`
3. 默认 `system`

---

## 分阶段任务清单

### Phase 1 — Window Host / Chrome 内核

**任务**

1. 新增 `WindowChrome` 模块与单元级可测逻辑（纯函数优先：点是否在边框、最大化 rect 计算）。
2. `App::Initialize` 根据模式选择 `WindowStyle` / 是否 `WS_VSCROLL`。
3. 处理：
   - `WM_NCCALCSIZE`（wParam==TRUE 时客户区铺满；最大化钳制 `rcWork`）
   - `WM_NCHITTEST`（边框 + 临时全客户区 caption 或固定顶栏高度 fallback）
   - `WM_GETMINMAXINFO`（最小尺寸，避免缩没）
4. DPI 变化时更新边框宽度；复用现有 `WM_DPICHANGED`。
5. 默认 system 路径与现网行为一致（对照测试）。

**退出标准**

- [ ] `AI_WIN_UI_CHROME=custom` 启动无系统标题栏
- [ ] 窗口可拖动（Phase1 可用顶部固定高度 fallback）
- [ ] 四边可缩放
- [ ] 最大化后任务栏仍可见
- [ ] 不设环境变量时，现有 demo 外观与操作不变

### Phase 2 — TitleBar UI 与交互

**任务**

1. Layout 属性：`hitTest`、`role="titleBar"`、根 `chrome`。
2. Demo 布局：标题栏 + 内容区 + 三个窗口按钮。
3. 按钮命令：
   - min → `ShowWindow(SW_MINIMIZE)`
   - max → `IsZoomed ? SW_RESTORE : SW_MAXIMIZE`，图标状态随窗口态更新
   - close → `PostMessage(WM_CLOSE)` 或 `DestroyWindow` 路径一致
4. 布局 / resize 后收集 hit regions；`WM_NCHITTEST` 使用真实区域。
5. 标题栏按钮 hover / pressed 样式与现有 `Button` 一致或使用 theme token。
6. 双击标题栏空白 → 最大化 / 还原（依赖 `HTCAPTION`）。

**退出标准**

- [ ] 按钮可点且不触发拖动
- [ ] 标题空白可拖；双击最大化正常
- [ ] TextInput / 其他控件在内容区行为不变
- [ ] 根布局 `chrome="custom"` 可启用（无环境变量时）

### Phase 3 — DWM、验证、文档

**任务**

1. `DwmExtendFrameIntoClientArea` + Win11 圆角属性；API 失败静默降级。
2. 可选：自绘 1px 边框 / 标题栏底部分割线（纯 UI）。
3. 脚本 `run_custom_chrome_demo.ps1`。
4. 文档 `doc/window-chrome.md`：启用方式、消息职责、限制、v2 路线。
5. 回归：`run_validation_suite.ps1` / 核心 layout 在 system 模式通过。

**退出标准**

- [ ] Win11 上圆角可见（或文档注明降级条件）
- [ ] 文档完整
- [ ] system 模式无功能回归
- [ ] DEVELOPMENT_LOG 已记录

---

## 关键消息与行为表

| 消息 / 入口 | Custom 模式行为 | System 模式 |
|---|---|---|
| 创建样式 | POPUP + THICKFRAME + sysmenu 等 | 现有 OVERLAPPEDWINDOW |
| `WM_NCCALCSIZE` | 客户区 = 整窗；最大化钳制 | DefWindowProc |
| `WM_NCHITTEST` | 边框 / caption / client 自定义 | DefWindowProc |
| `WM_GETMINMAXINFO` | 最小宽高；可选 max 尺寸 | 默认或微调 |
| `WM_NCLBUTTONDBLCLK` | 依赖 HTCAPTION 默认行为 | 默认 |
| DWM | 初始化扩展 frame / 圆角 | 可不调用 |
| 系统滚动条 | 不启用 `WS_VSCROLL` | 保持现有 |

---

## 风险与缓解

| 风险 | 影响 | 缓解 |
|---|---|---|
| 最大化遮挡任务栏 | 严重 UX | `MonitorFromWindow` + `rcWork` 在 NCCALCSIZE 处理 |
| 按钮区域被当成 caption | 点不到按钮 | clientOnly 优先于 caption |
| DPI 边框过窄/过宽 | 难拖边缘 | 使用 `GetSystemMetricsForDpi(SM_CXFRAME)` + padding |
| D2D/Skia 与 DWM 透明冲突 | 黑边 / 闪烁 | v1 不走全透明 frame；clear 色不透明 |
| main.cpp 继续膨胀 | 可维护性 | 逻辑进 `WindowChrome` |
| 验证脚本依赖系统标题栏高度 | 坐标假设失败 | custom 仅 opt-in；suite 默认 system |
| Skia 与 D2D 表现差异 | 标题栏观感 | chrome 逻辑与后端无关；用同一 layout 双后端抽查 |

---

## 验收清单（合并前）

### 功能

- [ ] System 模式：与合并前行为一致（标题栏、滚动、验证布局）
- [ ] Custom 模式：无系统标题栏，UI 标题栏可见
- [ ] 拖动、缩放、最大化、还原、最小化、关闭
- [ ] 双击标题栏切换最大化
- [ ] 标题栏按钮 hover / click 正常
- [ ] DPI 125% / 150% 下边框与标题栏比例合理（至少抽查一档）

### 工程

- [ ] Release / Debug 均可编译
- [ ] 新增源文件已入 CMake
- [ ] 链接 `dwmapi` 仅在需要时
- [ ] 计划文档与使用文档已提交

### 非功能

- [ ] 不强制所有 demo 改为 custom
- [ ] 无 Layered 半成品路径混入主 Present（避免半吊子回归）

---

## 工作量粗估

| 阶段 | 预估 |
|---|---|
| Phase 1 WindowChrome 内核 | 0.5–1 d |
| Phase 2 TitleBar UI + parser + demo | 0.5–1 d |
| Phase 3 DWM + 文档 + 回归 | 0.5 d |
| **v1 合计** | **约 1.5–2.5 d** |
| v2 Layered 异形 | 另计 2–4 d+ |

---

## 实现时注意（给落地分支的约束）

1. **默认 system，custom opt-in** — 保护现有脚本与验收基线。
2. **先消息后 UI** — Phase1 用 fallback 顶栏高度也能拖；Phase2 再换真实区域。
3. **不要在 v1 改 IRenderer 接口** — 保持 HWND 路径。
4. **Hit-test 顺序固定**：resize border → clientOnly → caption → HTCLIENT。
5. **最大化图标状态** 在 `WM_SIZE` 时同步（`SIZE_MAXIMIZED` / `SIZE_RESTORED`）。
6. **关闭路径** 与现有 `WM_DESTROY` → `PostQuitMessage` 一致。
7. **中文源文件编码** 注意 C4819；新文件优先 UTF-8 with BOM 或纯 ASCII 注释策略与仓库一致。

---

## 后续分支落地入口（执行时按此操作）

```text
1. git checkout master && git pull
2. git checkout -b feature/custom-window-chrome-v1
3. 按 Phase 1 → 2 → 3 实现，每阶段本地编译：
     .\build.ps1 -Configuration Release
4. 验收 system 默认 + custom demo
5. 更新 DEVELOPMENT_LOG.md
6. 开 PR 合并 master
```

可选拆分 PR 标题：

- `chrome(p1): borderless window host and NC hit-testing`
- `chrome(p2): UI title bar, hitTest attrs, system commands`
- `chrome(p3): DWM corners, demo script, docs`

---

## 参考

- 现有窗口入口：`src/main.cpp`（`CreateWindowExW`、`HandleMessage`）
- 渲染：`src/renderer.cpp` / `src/renderer_skia.cpp`（HWND 目标）
- 布局解析：`src/layout_parser.*`
- 历史计划体例：`doc/plan/2026-04-26-theme-tokens-v1.md`
- Win32：`WM_NCCALCSIZE`、`WM_NCHITTEST`、`WM_GETMINMAXINFO`、`DwmExtendFrameIntoClientArea`

---

## 修订记录

| 日期 | 说明 |
|---|---|
| 2026-07-15 | 初版任务计划书：自定义 Title Bar + 不规则窗体分档；v1 不含 Layered 真异形 |
