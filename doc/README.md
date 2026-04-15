# AI WinUI 项目文档

本目录为 `ai-win-ui` 的设计与集成说明文档。

## 目录结构

- `doc/README.md`
  - 本文件，总览项目目标、当前实现、渲染方案与事件交互支持。
- `doc/skia-integration.md`
  - Skia 渲染库集成分析与落地方案。

## 项目概述

当前项目基于 Win32 + Direct2D + DirectWrite，构建了一个简单的 retained-mode UI 树：

- `src/ui.h`
  - 定义了 `UIElement`、`Panel`、`Label`、`Button` 等组件。
  - 使用 `Panel` 进行垂直布局，具备类似 Flex Layout 的 padding、spacing、自动宽度填充。
- `src/renderer.h` / `src/renderer.cpp`
  - 基于 Direct2D 提供基础绘制能力。
  - 支持矩形填充、矩形描边、文本绘制。
- `src/main.cpp`
  - 初始化窗口，构建 UI 树，转发 `WM_SIZE`、`WM_PAINT` 与鼠标事件。

## UI 设计规则

- 使用 `Panel` 作为容器，实现垂直排列与间距控制。
- 子元素宽度自动扩展到布局区域，不需要手动指定每个子项宽度。
- 组件支持背景、边距、间距，适合不规则窗口与无 title 模式的内部内容布局。
- 事件交互执行顺序为“从顶层向下、从后到前”遍历，保证鼠标事件命中最前面的可见组件。

## 当前事件交互支持

- `UIElement::OnMouseMove/OnMouseDown/OnMouseUp`
  - 自动递归到子节点，并在命中子组件后停止传播。
- `Button`
  - 支持 hover、pressed、click 三种交互状态。
  - 点击触发 `SetOnClick` 注册的回调。

## Skia 集成建议

详见 `doc/skia-integration.md`。

## 进一步扩展建议

- 将当前 `Renderer` 抽象为统一接口，以便支持多种后端（Direct2D、Skia、Vulkan、Metal 等）。
- 将 `Panel` 布局模型扩展为真正的 Flex 布局，增加 `flex-grow`、`flex-shrink`、对齐方式等属性。
- 考虑使用 `Yoga` 布局引擎将现有容器体系升级为标准 Flexbox 布局。
- 使用 XML/JSON 定义布局，建立数据驱动 UI 构建流程。
- 支持将布局与资源打包为 `assets.zip`，同时兼容开发阶段的 `resource/` 目录模式。
- 增加 `OnMouseLeave`、键盘事件、焦点管理、拖拽区域等事件能力。

## 进一步文档

- `doc/resource-packaging.md`：资源加载、ZIP 打包与 XML/JSON 布局定义方案。

## 目录约定

- `lib/`：存放引用依赖包，例如预编译的 Yoga、Skia 或其它 native 库。
- `resource/`：程序运行时使用的资源目录，包含布局文件、图片、字体、样式等静态资源。
- `resource/layouts/`：布局定义文件（JSON / XML）。
- `resource/images/`：图片资源。
- 支持无 title 窗口的自定义拖拽区域与圆角/不规则形状窗口，不改变内部布局逻辑。

## MVP 与发展路线

详见 `doc/mvp-roadmap.md`。