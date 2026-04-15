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
- `src/resource_provider.*`
  - 实现 `DirectoryResourceProvider`，从 `resource/` 目录读取文件
- `src/layout_parser.*`
  - 支持从 JSON / XML 构建 UI 树
  - 支持 `Panel`、`Label`、`Button` 和 `Image`

## 目录说明

- `doc/`：设计文档与集成说明
- `lib/`：预编译依赖库存放位置
- `resource/`：运行时资源目录
  - `resource/layouts/`：布局定义文件
  - `resource/images/`：图片资源
- `src/`：源代码
- `build/`：CMake 构建输出

## 相关文档

- `doc/README.md`：项目设计与实现概览
- `doc/resource-packaging.md`：资源打包与加载方案
- `doc/skia-integration.md`：Skia 集成分析
- `doc/mvp-roadmap.md`：MVP 与迭代规划

## 开发日志

当前开发记录见：`DEVELOPMENT_LOG.md`

## 快速启动

1. 运行 `cmake -S . -B build`
2. 运行 `cmake --build build --config Debug`
3. 启动 `build/Debug/ai_win_ui.exe`
