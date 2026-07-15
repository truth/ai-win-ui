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
- `resource/`：运行时资源目录
  - `resource/layouts/`：布局定义（XML / JSON）
  - `resource/styles/`：跨文件命名样式（JSON catalog，`$style.xxx`）
  - `resource/themes/`：主题 tokens（`$color.x` 等）
  - `resource/images/` / `resource/icons/`：位图与 SVG
- `src/`：源代码
- `build/`：CMake 构建输出

## 相关文档

- `doc/README.md`：文档索引与现状快照
- `doc/plan/2026-07-15-directui-productization-personas.md`：**产品化路线 · 分人物（R/L/C/S/H/Q）**
- `doc/layout-spec.md`：**布局 XML/JSON 规则**（含 `$style`、ShapePanel、chrome）
- `doc/style-catalog.md`：样式目录 `import` / `extend` / 引用语法
- `doc/window-chrome.md`：自定义标题栏与 layered 异形窗
- `doc/resource-packaging.md`：资源打包与加载方案
- `doc/skia-integration.md`：Skia 集成分析
- `doc/mvp-roadmap.md`：MVP 与迭代规划

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
