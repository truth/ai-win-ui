# Implementation Plan: 响应式动效系统 (Transition DSL MVP)

## 1. 目标 (Goals)
实现 UI 元素的属性平滑过渡（过渡动效 Transition）。当前，当控件状态（如 Hover）发生改变时，背景色等属性会产生突变；本项目旨在提供一种类似 CSS `transition` 的机制，让属性可以经历一段时间的插值。

## 2. 架构设计与详细步骤

### Step 1: 解析器与模型层拓展
- **DSL 设计**：支持 `transition="background 200ms ease-out, border 150ms linear"`。
- **配置结构**：在 `BoxDecoration` 或 `UIElement` 中新增一个类似 `std::vector<TransitionDef>` 的配置项，用于存储具体属性的过渡时长和缓动函数类型 (`EasingType`)。
- **Parser 拓展**：在 `layout_parser.cpp` 和 `style_catalog.cpp` 中新增对 `transition` 属性的解析逻辑。

### Step 2: 动画控制器 (AnimationController) 与状态持有
- 修改 `src/animation_controller.h`，添加动画状态追踪。例如 `AnimatableColor` 类：
  - 持有 `Current`、`Target`、`Start`，以及动画开始的时间戳或已消耗的时间。
  - 提供 `Tick(uint32_t dt)` 返回是否需要继续请求重绘。
- 在 `UIElement` 的子类（如 `Panel` 和 `Button`）中，拦截状态改变导致的样式更新。
  - 核心逻辑：若目标样式发生变化，不直接改变渲染属性，而是启动或更新 `AnimatableColor`。

### Step 3: Tick 与渲染循环的贯通
- 目前的 `OnTimer` 触发间隔约为 16ms。
- 在 `UIElement::OnTimer` 的向下派发过程中（由 `UiHost` 发起），传递经过的 `dt`。
- 如果任意 `AnimatableProperty` 尚未完成插值，其 `Tick` 就会返回 `true`，从而向上冒泡，让 `UiHost` 调用 `InvalidateRect`。
- `Renderer` 在绘制时（`OnPaint`）直接读取 `AnimatableColor` 的 `Current` 值进行绘制，而不需要知道背后的动画逻辑。

### Step 4: 测试与验证
- 创建 `resource/layouts/transition_demo.xml`，放置具备 Hover 状态并在其上开启了 `transition` 的卡片和按钮。
- 配置到 `demo_gallery.xml` 中，进行手动视觉测试。
- 执行 `scripts/run_ci_local.ps1 -SkipBuild` 确保 7 项自动化回归（包含布局测量的 Golden tests）全部保持通过。
