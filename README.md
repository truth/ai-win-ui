# AI WinUI Renderer

本仓库实现一个基于 Win32 + Direct2D + DirectWrite 的轻量级 Retained UI 引擎，支持资源驱动的 JSON / XML 布局定义。

## 项目目标

- 提供可扩展的 UI 组件体系（`Panel`、`Label`、`Button`、`Image`）
- 支持资源目录模式 `resource/`，运行时加载布局与图片
- 支持 JSON 和 XML 两种布局定义格式
- 支持优先加载 `assets.zip`，并回退到 `resource/` 目录模式
- `assets.zip` 当前是首选资源包，缺失资源时自动回退到 `resource/` 目录加载
- 为未来集成 Skia、Yoga 或其它渲染/布局后端提供可扩展基础

## 当前实现

- `src/main.cpp`
  - 程序入口，初始化 Win32 窗口与渲染器
  - 加载 `resource/layouts/ui.json` 或 `resource/layouts/ui.xml`
  - 支持窗口尺寸调整、鼠标事件转发、`WM_MOUSELEAVE` 状态回收与 UI 重绘
- `src/renderer.*`
  - 封装 Direct2D / DirectWrite / WIC 基础绘制能力
  - 支持文本、矩形、边框与位图绘制
- `src/ui.h`
  - 定义 UI 元素树与组件
  - 支持递归布局、渲染与事件命中
  - 支持通用 `width` / `height` / `margin` 与 `Panel` 的 `alignItems` / `justifyContent`
  - 已开始渲染层抽象，便于未来支持 Skia 后端
  - 支持 `TextInput` 文本编辑、选择与剪贴板交互，优化 `Panel`/`GridPanel` 测量与自动尺寸计算
- `src/resource_provider.*`
  - 实现 `DirectoryResourceProvider`，从 `resource/` 目录读取文件
- `src/layout_parser.*`
  - 支持从 JSON / XML 构建 UI 树
  - 支持 `Panel`、`Label`、`Button` 和 `Image`

## 目录说明

- `doc/`：设计文档与集成说明
- `lib/`：预编译依赖库存放位置
- `react/`：零冗余 TypeScript React 运行时与 JSX 示例工程
  - `react/runtime/`：自研微型 Reconciler、Hooks 与 19 个原生组件类型定义（含 `<FinancialChart>`）
  - `react/demos/`：`react_showcase.tsx` 综合展示页与 `stock_terminal/` 同花顺股票终端
- `resource/`：运行时资源目录
  - `resource/layouts/`：布局定义（XML / JSON）
  - `resource/react/`：Bun 打包生成的 QuickJS-ng 运行时 `.bundle.js`
  - `resource/styles/`：跨文件命名样式（JSON catalog，`$style.xxx`）
  - `resource/themes/`：主题 tokens（`$color.x` 等）
  - `resource/images/` / `resource/icons/`：位图与 SVG
- `src/`：源代码（含 `src/react/` 双线程原生桥接层）
- `third_party/`：第三方依赖源码（含内置 `third_party/quickjs-ng/` v0.18.0）
- `build/`：CMake 构建输出

## 相关文档

- `doc/README.md`：文档索引与现状快照
- `doc/plan/2026-11-20-skia-textblob-and-multi-dirty-rect.md`：**Skia 文本 `SkTextBlob` 缓存与多脏矩形渲染优化方案**（UTF-16 直通度量、`DirtyRegion` + `SkRegion` 裁剪与局部 `BitBlt`）
- `doc/reports/2026-11-20-skia-opt-before-after-report.md` / `.html`：**Skia 优化前后 Release 实测对比报告**（Stock Terminal / 60Hz 压测 / Day-K 多脏矩形）
- `doc/plan/2026-11-20-react-jsx-native-ui.md`：**React JSX 原生 UI 架构与同花顺股票终端方案**（QuickJS-ng + 双线程整型协议）
- `doc/reports/2026-11-20-react-vs-xml-perf-report.md` / `.html`：**React (QuickJS-ng) vs XML (C++) 性能评估对比报告**
- `doc/plan/2026-07-15-directui-productization-personas.md`：**产品化路线 · 分人物（R/L/C/S/H/Q）**（Wave1 已收口，Wave2 进行中）
- `doc/plan/2026-07-16-embed-api-v1.md`：H5 嵌入 API 设计草案
- `doc/layout-spec.md`：**布局 XML/JSON 规则**（含 `$style`、`$color`、ellipsis、VirtualListBox、Popup）
- `doc/style-catalog.md`：样式目录 `import` / `extend` / 引用语法
- `doc/window-chrome.md`：自定义标题栏与 layered 异形窗
- `doc/resource-packaging.md`：资源打包与加载方案
- `doc/skia-integration.md`：Skia 集成与性能架构（`SkTextBlob` LRU、UTF-16 度量、`DirtyRegion`、R5/R6/R7/R8）
- `doc/troubleshooting-and-pitfalls.md`：**核心排坑与架构避坑指南**（Thickness、默认内边距、D2D 全帧边界、BatchUpdate、Skia Debug 软光栅）
- `doc/mvp-roadmap.md`：MVP 与迭代规划
- `doc/mvp-acceptance.md`：手工验收（含 Wave2 检查表）

## 可嵌入库（Wave3）

```cpp
#include <ai_win_ui/host.h>
auto host = ai_win_ui::Host::Create(hInstance, info);
return host->Run();
```

- 静态库：`ai_win_ui_lib`（`build/Debug/ai_win_ui_lib.lib`）
- 头文件：`include/ai_win_ui/host.h`、`version.h`（当前 **0.3.2**，引擎线目标 **0.4.0**）
- 示例：`samples/embed_host` — shell + **parent 子窗嵌入** + 外部消息泵  
  → `build/Debug/embed_host.exe`
- 示例：`samples/sticky_notes`（便签）— 内联布局 XML + 托盘 + 多个浮动 layered 窗口 + 管理器  
  → `build/Debug/sticky_notes.exe`，说明见 `samples/sticky_notes/README.md`
- 说明：`doc/plan/2026-07-16-embed-api-v1.md`、`CHANGELOG.md`

## React JSX 原生 UI 与股票终端

采用内置 **QuickJS-ng v0.18.0** + **双线程整型 `MutationBatch` 协议** + **自研轻量 TS 运行时**（无 React Native / DOM 冗余 JS），JSX 直连 C++ Yoga 排版与 Skia/Direct2D 绘制：

```powershell
# 1. 编译 TSX Bundle（支持 -Watch）
.\scripts\build_react.ps1

# 2. 启动 React JSX Showcase 演示（支持 -Watch 热重载 / -Renderer direct2d）
.\scripts\run_react_demo.ps1 -Renderer skia

# 3. 启动 React 版同花顺金融终端（含 500ms 零重排实时行情、分时/K线与键盘精灵）
.\scripts\run_stock_react.ps1 -Renderer skia

# 4. 运行 React 桥接与组件单元测试
.\build\Debug\react_unit_test.exe
```

详细架构与接口说明见：`doc/plan/2026-11-20-react-jsx-native-ui.md`。

## Skia 性能优化与基准压测

> 提示：Debug 构建链接未优化的 `Debug-x64/skia.lib`（软光栅耗时约为 Release 的 ~28×），性能压测与交互体验评测请统一使用 `-Configuration Release`。

```powershell
# 1. 构建 Release 并运行 Skia 专项单元测试（DirtyRegion + UTF-16 度量 + LRU 缓存）
.\build.ps1 -Configuration Release
.\build\Release\skia_unit_test.exe

# 2. 运行 XML vs React Release 性能对比基准（Stock Terminal + 60Hz 100格 + 极速打字）
.\scripts\bench_react_vs_xml.ps1 -Renderer both -Configuration Release

# 3. 运行 Day-K 500ms 四角分散更新多脏矩形专项压测
.\scripts\bench_skia_dirty_region.ps1 -Renderer both -Configuration Release
```

详见设计文档 `doc/plan/2026-11-20-skia-textblob-and-multi-dirty-rect.md` 与实测报告 `doc/reports/2026-11-20-skia-opt-before-after-report.md`（及 `.html` 可视化报告）。

## 本地 CI（Q8）+ GitHub 绿勾

```powershell
# 首次 / CI 机：解压并编译 Yoga
.\scripts\setup_ci_deps.ps1

# 构建 + gallery + smoke + measure golden
.\scripts\run_ci_local.ps1

# 已编译时
.\scripts\run_ci_local.ps1 -SkipBuild
```

- 无本地 Skia 时自动退化为 **Direct2D only**（与 GitHub 一致）。
- GitHub Actions：`.github/workflows/ci.yml`（`windows-latest`，D2D smoke+golden）。

## 开发日志

当前开发记录见：`DEVELOPMENT_LOG.md`

## 快速启动

1. 运行 `.\build.ps1`
2. 或运行 `build.cmd`
3. 启动 `build/Debug/ai_win_ui.exe`

## 编译脚本

- `build.ps1`
  - 默认执行 Debug 构建
  - 自动调用 CMake 配置与编译
  - 自动把 `resource/` 复制到可执行文件旁边，保证运行时资源可用
  - 可选参数：
    - `-Configuration Release`
    - `-Clean`
    - `-Run`
- `build.cmd`
  - Windows 命令行包装脚本，内部调用 `build.ps1`

### 示例

- `.\build.ps1 -Configuration Debug`
- `.\build.ps1 -Configuration Release -Clean`
- `.\build.ps1 -Run`

### 清理编译缓存

使用 `clean.ps1` 或 Windows 命令行入口 `clean.cmd`，只清理、不重新编译：

```powershell
# 预览待清理目录，不删除文件
.\clean.ps1 -WhatIf

# 删除编译产物及 CMake 缓存，下次构建会重新配置
.\clean.ps1
# 或在 cmd 中运行：clean.cmd

# 同时删除 dist/ 打包产物
.\clean.ps1 -IncludeDist
```

默认清理 `build/`（包含所有 CMake presets 输出）、`build_test/`、
`test_parser/build/` 和 `.tmp-build/`；不存在的目录自动跳过。
保留 `third_party/`、`lib/`、`resource/`、`vcpkg_installed/` 和默认的 `dist/`，
不会删除已准备的第三方依赖。脚本以自身所在目录为基准，可以从其他工作目录调用，
支持 `-Confirm` 逐项确认；检测到清理目录或其内容包含链接时拒绝清理，避免误删链接目标。
运行前请先结束构建和相关程序，避免文件占用导致清理失败。
与 `build.ps1 -Clean` 不同，clean 脚本不会在清理后立即重新构建。

## 自定义标题栏 / 异形窗口 / 样式目录

默认仍使用系统标题栏。

```powershell
# 无边框自绘标题栏（HWND）
.\scripts\run_custom_chrome_demo.ps1 -Configuration Release -BuildIfMissing

# Layered 圆角卡片（每像素透明；默认 Skia，可切 Direct2D）
.\scripts\run_layered_chrome_demo.ps1 -Configuration Release -BuildIfMissing
.\scripts\run_layered_chrome_demo.ps1 -Renderer direct2d

# 样式 catalog（$style.xxx）+ ShapePanel 预览
.\scripts\run_layout_demo.ps1 -Layout style-catalog -Renderer skia

# 网格案例总览（手工点开每个 demo）
.\scripts\run_demo_gallery.ps1
# 或: .\scripts\run_layout_demo.ps1 -Layout demo-gallery

# 点击打开心形/花瓣等异形窗
.\scripts\run_shaped_windows_demo.ps1

# 若启动异常：清掉粘连的 AI_WIN_UI_* 环境变量
.\scripts\clear_ai_win_env.ps1

# Wave1 质量：gallery 覆盖 / measure golden / 文本 dump
.\scripts\check_gallery_coverage.ps1
.\scripts\run_measure_golden.ps1 -Layout core-validation -Renderer both
.\scripts\run_text_dump.ps1 -Layout core-validation -Renderer skia
```

| 环境变量 | 含义 |
|----------|------|
| `AI_WIN_UI_CHROME` | `system` \| `custom` \| `layered` |
| `AI_WIN_UI_RENDERER` | `skia` \| `direct2d` |
| `AI_WIN_UI_LAYOUT` | 布局路径，如 `layouts/ui.xml` |
| `AI_WIN_UI_THEME` | 主题 JSON |
| `AI_WIN_UI_STYLES` | 额外合并的 styles JSON |
| `AI_WIN_UI_SIZE` | 客户区 `WxH`（异形小窗） |
| `AI_WIN_UI_REACT` | 次级/子进程扩展隔离变量（或用 `--react <bundle.js>` 启动 `.bundle.js`） |

布局里 `style` 的三种写法与 XML/JSON 差异见 `doc/layout-spec.md`、`doc/style-catalog.md`。
Chrome / 多进程异形见 `doc/window-chrome.md`。

## Script Usage

The repository includes a few helper scripts for building the app, preparing
the local Skia SDK, and launching validation layouts.

### `build.ps1`

Build the default local project output with CMake.

Common usage:

```powershell
.\build.ps1
.\build.ps1 -Configuration Release
.\build.ps1 -Clean
.\build.ps1 -Run
```

Parameters:

- `-Configuration`
  - build configuration such as `Debug` or `Release`
- `-BuildDir`
  - custom build output directory
- `-Generator`
  - optional CMake generator override
- `-Clean`
  - delete the selected build directory before configuring
- `-Run`
  - launch the built executable after the build succeeds

### `build.cmd`

Windows command prompt wrapper for `build.ps1`.

Example:

```cmd
build.cmd
build.cmd -Configuration Release
```

### `scripts/setup_skia_prebuilt.ps1`

Download and extract the local Skia SDK package used by the Skia preset.

Example:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\setup_skia_prebuilt.ps1
```

Optional usage:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\setup_skia_prebuilt.ps1 `
  -VersionTag m124-08a5439a6b `
  -Destination third_party/skia-m124
```

Parameters:

- `-VersionTag`
  - release tag used to download the prebuilt package
- `-Destination`
  - target directory under the repository
- `-IncludeDebug`
  - include the Debug package
- `-IncludeRelease`
  - include the Release package

### `scripts/run_layout_demo.ps1`

Launch any layout under `resource/layouts/` with either backend.

Example:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\run_layout_demo.ps1 `
  -Renderer skia `
  -Layout layouts/core_validation.xml
```

Useful variants:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\run_layout_demo.ps1 `
  -Renderer direct2d `
  -Layout layouts/yoga_measure_cases.xml

powershell -ExecutionPolicy Bypass -File .\scripts\run_layout_demo.ps1 `
  -Renderer skia `
  -Layout layouts/skia_image_cases.xml `
  -BuildIfMissing
```

Parameters:

- `-Layout`
  - relative path under `resource/`, absolute path, or alias
    (`style-catalog`, `shaped-hub`, `table-components`, `layered-chrome`, …)
- `-Renderer`
  - `skia` or `direct2d`
- `-BuildIfMissing`
  - configure and build automatically when the target executable is missing
- `-Rebuild`
  - force rebuild
- `-NoLaunch`
  - validate inputs and prepare the launch without starting the app

### `scripts/run_core_validation.ps1`

Convenience wrapper for the main MVP validation page:
`resource/layouts/core_validation.xml`.

Example:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\run_core_validation.ps1 `
  -Renderer skia

powershell -ExecutionPolicy Bypass -File .\scripts\run_core_validation.ps1 `
  -Renderer direct2d `
  -NoLaunch
```

Supported parameters:

- `-Renderer`
  - `skia` or `direct2d`
- `-BuildIfMissing`
  - build the selected runtime if needed
- `-NoLaunch`
  - skip starting the executable

### `scripts/run_dashboard_reference.ps1`

Convenience wrapper for `resource/layouts/dashboard_reference.xml`.

Example:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\run_dashboard_reference.ps1 `
  -Renderer skia
```

Supported parameters:

- `-Renderer`
  - `skia` or `direct2d`
- `-BuildIfMissing`
  - build the selected runtime if needed
- `-NoLaunch`
  - skip starting the executable
