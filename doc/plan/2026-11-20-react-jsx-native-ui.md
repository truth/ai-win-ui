# React JSX Native UI Architecture (`QuickJS-ng` + Dual-Thread Bridge)

- **Date**: `2026-11-20`
- **Branch**: `feature/react-jsx-ui`
- **Status**: MVP Implemented & Verified (awaiting user interactive test before merging to `master`)

---

## 1. 背景与设计目标

为 `ai-win-ui` 引入基于 React JSX 声明式语法的 UI 开发能力，同时彻底规避传统 React Native for Windows (RNW) 中存在的冗余 JS 层（如 JS 端打包完整 React/Fiber + Reconciler + JS 侧样式/布局桥接 + JSON 字符串序列化消息通道导致的体积膨胀与高延迟）：

1. **零冗余 JS 运行时**：不引入 `react-native`、Fiber 或 DOM polyfills；自研轻量 TypeScript 运行时仅 ~15.4 KB（minified），完整 Showcase 演示含全部运行时仅 ~24.5 KB，完整同花顺股票终端含行情引擎与运行时仅 ~41.5 KB。
2. **原生 C++ 直连渲染与布局**：JSX 直接映射到 `ai-win-ui` C++ 保留模式节点（`Panel`、`TextInput`、`TreeView`、`Slider`、`Pagination`、`FinancialChart` 等 19 个内置控件与 `<Spacer>`），继续由 C++ Yoga 完成 Flexbox 排版、由 Skia / Direct2D 执行 144Hz 原生硬件绘制。
3. **整型二进制化指令协议**：不使用 JSON 字符串桥接，跨线程提交采用整型 `uint32_t nodeId`、`NodeTypeId`、`PropId`、`EventTypeId` 与强类型 `MutationOp` 批次。
4. **纯 C++ 隔离边界**：`ai_win_ui_lib` 零依赖 QuickJS 头文件，通过 `src/react_bridge_iface.h` 纯虚接口与可选 CMake 开关 `AI_WIN_UI_ENABLE_REACT=ON` 挂载 `ai_win_ui_react`。

---

## 2. 双线程 Fabric 风格架构

```text
+--------------------------------------+       MutationBatch (typed ops)      +---------------------------------------+
|      JS Worker Thread                | -------------------------------------> |     CommitQueue (lock-free/mutex)     |
|  QuickJS-ng v0.18.0 (quickjs_ng)     |    PostMessageW(kMsgReactCommit)       |                                       |
|  - custom TS React reconciler        |                                        +-------------------+-------------------+
|  - hooks / keyed diff / batching     |                                                            |
|  - globalThis.__dispatchEvent        | <-------------------------------------                     v
+--------------------------------------+   UiEvent (nodeId, eventId, seq, val)  +---------------------------------------+
                                                                                |      Win32 UI Thread                  |
                                                                                |  UiHost + ReactNodeRegistry           |
                                                                                |  - BeginBatchUpdate / EndBatchUpdate  |
                                                                                |  - Optimistic input + ackSeq guard    |
                                                                                |  - Visual-only fast path (skip Yoga)  |
                                                                                |  - Skia / Direct2D native rendering   |
                                                                                +---------------------------------------+
```

### 关键机制

1. **视觉更新快路径 (`needsLayout == false`)**：
   - 当批次只修改文本/颜色/值且目标控件为固定尺寸（或 `TextInput` 具备固定宽度 / `flexGrow > 0`）时，`ReactNodeRegistry` 通过 `EndBatchUpdate(false)` 仅触发重绘，跳过 Yoga 全树重新布局。
2. **乐观 UI 优先与 `eventSeq` 过期回显拦截**：
   - `TextInput`、`Slider`、`Switch`、`Checkbox`、`Pagination` 在 UI 线程 0ms 即时响应用户输入并递增每节点单调计数 `eventSeq`。
   - JS 处理事件后回传属性更新携带 `ackSeq`；若 `ackSeq < currentEventSeq`，`ReactNodeRegistry` 直接丢弃过时回显，避免快速打字/拖拽时闪烁回退；程序性强制重置（`ackSeq >= currentEventSeq`）与 `TextInput::SetTextPreserveCaret` 配合保证光标位置不跳变。
3. **排版避坑内建守卫**：
   - JS/TS 层 `Thickness` 支持单值或 `{ x?, y?, left?, top?, right?, bottom? }`，自动标准化为 `ai-win-ui` 的 `(Left, Top, Right, Bottom)`；
   - React `<Panel>` 默认 `padding = {0, 0, 0, 0}`，避免继承旧 `Panel` 默认 8px 内边距导致的紧凑行高度塌陷问题；
   - 支持 `$color.*` 主题色与 `$style.*` 样式目录令牌引用。

---

## 3. 代码清单与模块职责

| 层级 | 路径 | 职责 |
|------|------|------|
| JS 引擎 | `third_party/quickjs-ng/` | Vendored QuickJS-ng v0.18.0 纯 C 核心源码（20 文件），编译为 `quickjs_ng` 静态库 |
| 核心解耦接口 | `src/react_bridge_iface.h`, `src/application.h` | `IReactHostSurface` / `IReactBridge` / `RegisterReactBridgeFactory` 与 `kMsgReactCommit` 消息定义 |
| Native 控件钩子 | `src/ui_element.h`, `src/ui_inputs.h`, `src/ui_pickers.h`, `src/ui_views.h`, `src/ui_financial_chart.h` | `InsertChildBefore`、`RemoveChild`、`TextInput::SetTextPreserveCaret`、`LockColors()`、`TreeView`/`TreeItem` 双向同步、`FinancialChart` (`UpdateLastPoint`, `symbolName`, `periodLabel`) |
| Host 集成 | `include/ai_win_ui/host.h`, `src/ui_host.cpp`, `src/main.cpp` | 支持 `--react <bundle.js>` / `--watch` 热重载轮询、`OnFirstReactCommitReady`、`WM_KEYDOWN` 热键转发与次级宿主隔离 |
| C++ React Bridge | `src/react/react_protocol.h`, `react_js_runtime.*`, `react_node_registry.*`, `react_host_bridge.*` | JS 工作线程托管、定时器驱动、整型指令应用、零 JSON 图表点阵/K线直读 (`chartSetPoints`/`chartAppendPoint`/`chartSetCandles`)、节点生命周期托管与同步单元测试 API |
| TS 微型运行时 | `react/tsconfig.json`, `react/runtime/{protocol,jsx-runtime,reconciler,components,index}.ts` | `jsx`/`jsxs`/`Fragment`、`useState`/`useReducer`/`useEffect`/`useLayoutEffect`/`useMemo`/`useCallback`/`useRef`/`createContext`/`useContext`/`useWindowKeyDown`、Keyed diff、19 原生控件（含 `<FinancialChart>`）与 `<Spacer>` 定义 |
| 示例与工具脚本 | `react/demos/react_showcase.tsx`, `react/demos/stock_terminal/`, `resource/react/{react_demo,stock_terminal}.bundle.js`, `samples/react_demo/`, `samples/react_unit_test/`, `scripts/build_react.ps1`, `scripts/run_react_demo.ps1`, `scripts/run_stock_react.ps1`, `resource/layouts/demo_gallery.xml` | Showcase 展示窗口与 React 同花顺股票终端（`<FinancialChart>` + `useWindowKeyDown` + 500ms Tick + 盘口/键盘精灵）、独立宿主 demo、单元测试与画廊卡片入口 |

---

## 4. 构建与运行命令

```powershell
# 1. 编译 TSX Showcase Bundle（需 Bun >= 1.x，支持 -Watch 热重载）
powershell -ExecutionPolicy Bypass -File .\scripts\build_react.ps1

# 2. 全量编译（含 ai_win_ui_lib, quickjs_ng, ai_win_ui_react, ai_win_ui, react_demo, react_unit_test）
.\build.ps1

# 3. 运行 React 独立示例 / React 股票终端（支持 -Renderer skia|direct2d 和 -Watch 热重载）
.\scripts\run_react_demo.ps1 -Renderer skia
.\scripts\run_stock_react.ps1 -Renderer skia

# 4. 直接运行单元测试与本地 CI 回归
.\build\Debug\react_unit_test.exe
.\scripts\run_ci_local.ps1 -SkipBuild
```

---

## 5. React 同花顺金融终端复刻 (`react/demos/stock_terminal/`)

- **原生控件与快捷键扩充**：
  - 将 `FinancialChart` 作为第 19 个原生 JSX 组件纳入协议（`NodeTypeId::FinancialChart = 19`），通过 QuickJS C API 直接读取 JS 对象数组（`chartSetPoints` / O(1) `chartAppendPoint` / `chartSetCandles`），零 JSON 序列化且不触发 Yoga 布局；
  - `src/ui_financial_chart.h` 增加 `UpdateLastPoint`、`symbolName` 与 `periodLabel` 字段以动态显示切换后的证券名称与周期标签（保留默认行为不影响原 `stock_terminal.exe`）；
  - `IReactBridge::OnKeyDown` 将窗口快捷键（`T` 切换主题、`F5` 重置分时、`Escape` 收起检索）经 `EventTypeId::KeyDown` 转发至 `useWindowKeyDown` Hook。
- **高频 0 重排设计与排版细节**：
  - 右侧高密度 10×2 盘口与固定 8 槽位短线精灵中的动态 `Label` 均显式指定固定 `width`/`height` 与 `padding={0}`，确保 500ms 行情 Tick 更新走视觉快路径（`layoutCount` 增加 0 次，0 节点重建、0 底部溢出）。

---

## 6. 验证结果摘要

1. **包体积对比与构建效率**（Bun `1.3.6` 实测）：
   - 运行时单独 minified（`react/runtime/index.ts`）：**15,774 bytes (~15.40 KB)**
   - Showcase 含运行时 minified（`react/demos/react_showcase.tsx`）：**25,048 bytes (~24.46 KB)**
   - React 股票终端含行情引擎与运行时 minified（`react/demos/stock_terminal/index.tsx`）：**42,528 bytes (~41.53 KB)**，未压缩开发包 **95,215 bytes (92.98 KB)**（打包耗时 `20ms`）；
   - `.\build.ps1` 全量目标 Debug 编译 **0 Error / 0 Warning**。
2. **单元测试 (`react_unit_test.exe`)**：
   - `[PASS] TestEventSeqSuppressionAndRegistry`：验证 `Panel` 零内边距默认值、视觉快路径跳过 Yoga、`eventSeq`/`ackSeq` 过时回显拦截与合法重置、`InsertBefore` 兄弟重排、子树销毁通知；
   - `[PASS] TestShowcaseBundleExecutionAndDiff`：真实运行 `react_demo.bundle.js`，验证初始化原生节点、`useEffect` 标题同步、`TextInput` 乐观输入与 `TextChanged` 视觉快路径、点击 `Only Open` -> `Reverse` 原生行 `Panel` 指针就地 Keyed 重排 -> `+ Add Item` 挂载、QuickJS 定时器与零泄漏销毁（`nodes=85`）；
   - `[PASS] TestStockTerminalBundleAndFinancialChart`：真实运行 `stock_terminal.bundle.js`，验证 `FinancialChart` 分时加载、500ms Tick 追加点位且 **Yoga 布局次数为 0**、`日K` 切换加载 90 根 K 线、`T` 键切换 Modern 主题（`nodes=262, candles=90`）。
3. **无头冒烟与几何度量转储**：
   - `react_demo.bundle.js`（1280×840，`skia` / `direct2d`）：exit 0，91 节点 NDJSON，`collapsed=0`；
   - `stock_terminal.bundle.js`（1280×820，`skia` / `direct2d`）：exit 0，262 节点 NDJSON，`collapsed=0`，短线精灵末行严格位于容器边界内；
   - `react_demo.exe --renderer skia`：exit 0。
4. **本地全量 CI 门禁 (`.\scripts\run_ci_local.ps1 -SkipBuild`)**：
   - **7 / 7 项 100% PASS**（`Passed: 7  Failed: 0`，含画廊覆盖率、Skia/Direct2D 无头冒烟与 3 组布局 Golden 度量）。

