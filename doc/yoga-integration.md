# Yoga 集成说明

本文记录 `ai-win-ui` 在当前阶段接入 Yoga 的实际过程，目标是把现有 `Panel` 的垂直栈布局切换到真正的 Flexbox 布局引擎，同时保持 UI 公共接口不暴露后端实现细节。

## 1. 集成目标

- 保持 `UIElement`、`Panel`、`LayoutParser`、`UIContext` 不依赖 Yoga 具体类型。
- 通过 `ILayoutEngine` 把布局实现细节收口到独立后端。
- 先打通 `Panel -> ILayoutEngine -> Yoga` 这条最小闭环，再逐步扩展到更多 Flex 属性。

## 2. 为什么没有直接使用官方预编译包

本次确认到的官方状态是：

- Yoga 官方仓库提供源码、CMake 构建逻辑和多语言绑定。
- 官方 README 明确说明主实现基于 C++20，并推荐通过 CMake 构建。
- 官方没有直接提供可拿来即用的 Windows C++ 预编译 `lib` 包。
- 官方 README 也提到了 `vcpkg` 端口，但为了让当前仓库的依赖路径更可控，这里先采用“固定版本源码 + 本地构建”的方式。

参考：

- `third_party/yoga-3.2.1/README.md`
- `third_party/yoga-3.2.1/yoga/CMakeLists.txt`

## 3. 获取源码

当前接入使用的是 `Yoga 3.2.1`。

源码目录：

- `third_party/yoga-3.2.1`

下载来源：

- `https://github.com/facebook/yoga`
- `https://github.com/facebook/yoga/releases`

## 4. 本地编译结果

当前已经在本地完成了 Yoga 静态库构建，产物如下：

- Debug: `third_party/yoga-3.2.1/build/yoga/Debug/yogacore.lib`
- Release: `third_party/yoga-3.2.1/build/yoga/Release/yogacore.lib`

Yoga 在 CMake 里的核心目标名是：

- `yogacore`

主工程当前直接链接这两个已编译产物，而不是把 Yoga 作为子目录一起参与主工程构建。

这样做有两个目的：

- 避免主工程第一次配置时再额外拉起 Yoga 自身的大量 CMake 目标。
- 让应用工程和第三方库构建边界更清晰，便于后面替换为 `vcpkg` 或预编译包方案。

## 5. Windows 编译兼容修补

在当前 Windows 环境下，Yoga 3.2.1 的一个头文件会因为源文件中的特殊引号字符触发 MSVC `C4819`。Yoga 自身默认把警告当错误处理，所以会导致库构建失败。

本地解决方式是在 Yoga 自身的 CMake 配置里，对 `yogacore` 仅追加一条私有编译选项：

```cmake
if (MSVC)
    target_compile_options(yogacore PRIVATE /wd4819)
endif()
```

位置：

- `third_party/yoga-3.2.1/yoga/CMakeLists.txt`

这个修补只影响第三方库自身，不改变主工程的告警策略。

## 6. 官方 API 入口

当前接入阶段主要关注的是 Yoga 的 C API。主入口头文件是：

- `third_party/yoga-3.2.1/yoga/Yoga.h`

第一阶段用到的关键能力包括：

- 创建与释放节点：`YGNodeNew`、`YGNodeFreeRecursive`
- 搭建节点树：`YGNodeInsertChild`
- 设置布局样式：`YGNodeStyleSetFlexDirection`、`YGNodeStyleSetAlignItems`、`YGNodeStyleSetJustifyContent`
- 设置盒模型：`YGNodeStyleSetPadding`、`YGNodeStyleSetMargin`、`YGNodeStyleSetGap`
- 设置尺寸：`YGNodeStyleSetWidth`、`YGNodeStyleSetHeight`
- 计算布局：`YGNodeCalculateLayout`
- 读取结果：`YGNodeLayoutGetLeft`、`YGNodeLayoutGetTop`、`YGNodeLayoutGetWidth`、`YGNodeLayoutGetHeight`

对应头文件主要是：

- `third_party/yoga-3.2.1/yoga/YGNode.h`
- `third_party/yoga-3.2.1/yoga/YGNodeStyle.h`
- `third_party/yoga-3.2.1/yoga/YGNodeLayout.h`

## 7. 当前工程里的接入方式

### 7.1 架构落点

Yoga 不直接进入 `UI` 公共接口，而是挂在：

- `src/layout_engine.h`
- `src/layout_engine.cpp`

外部只看到：

- `ILayoutEngine`
- `CreateDefaultLayoutEngine()`
- `CreateYogaLayoutEngine()`

`Panel` 继续只和 `ILayoutEngine` 打交道，`App` 通过 `UIContext` 注入布局引擎。

### 7.2 第一版实现策略

第一版接入以“稳定替换当前垂直栈行为”为目标，因此采用了一个偏稳健的桥接策略：

- 先继续使用现有 `UIElement::Measure()` 计算每个子元素的期望高度。
- 再把这些测量结果映射到 Yoga 子节点上。
- 让 Yoga 负责容器级的 padding、对齐、主轴分布和最终位置计算。

这意味着当前已经是“真接 Yoga”，但还不是“所有叶子节点都直接通过 Yoga 的 measure callback 求尺寸”的最终形态。

## 8. 当前范围与已知边界

当前 Yoga 接入范围：

- `Panel` 的垂直栈布局
- `Panel` 的 `row / column` 主轴切换
- `padding`
- `spacing`
- `alignItems`
- `justifyContent`
- 子项 `margin`
- 叶子控件的 Yoga `measure func` 回调接入
- 叶子控件测量结果的轻量缓存

当前还没有展开的内容：

- `flex-grow`
- `flex-shrink`
- `flex-basis`
- 更完整的测量缓存与行为对齐

## 12. 测量验证样例

为了专门验证 Yoga 的叶子测量行为，仓库里新增了两份样例布局：

- `resource/layouts/yoga_measure_cases.xml`
- `resource/layouts/yoga_measure_cases.json`

这组样例主要覆盖：

- 受限宽度下的标签换行
- 横向 `row` 容器里的文本、按钮、输入框混排
- 固定高度按钮与普通自适应文本的组合
- `Spacer` 在测量回调接入后的主轴扩展行为

运行时可以通过环境变量切换到这组样例，而不必改默认布局文件：

```powershell
$env:AI_WIN_UI_LAYOUT = "layouts/yoga_measure_cases.xml"
```

应用窗口标题会显示当前实际加载的布局路径，便于手动对照验证。

一个需要明确记录的小差异是：

- 旧版手写 `Panel` 对 `SpaceBetween + spacing` 的处理是“最小间距”语义。
- Yoga 的 `justify-content: space-between` 与 `gap` 组合后，空余空间的分配方式和旧逻辑不完全一致。

这不影响当前主链路打通，但后续如果要追求行为完全一致，需要再做一次针对 `SpaceBetween` 的语义校准。

## 9. 主工程构建接法

当前主工程做了三件事：

1. 把 Yoga 头文件目录加入包含路径。
2. 根据构建配置链接对应的 `yogacore.lib`。
3. 把默认布局引擎切换为 Yoga 实现。

这样 `main.cpp` 里的 `CreateDefaultLayoutEngine()` 不需要改调用方逻辑，应用初始化路径保持稳定。

## 10. 后续建议

推荐继续按下面顺序演进：

1. 为 `SpaceBetween` 做行为校准，尽量对齐旧 `Panel` 语义。
2. 为 Yoga 布局补充更明确的测量缓存和边界验证。
3. 把更多容器布局能力统一收进 `ILayoutEngine`。
4. 等 Yoga 稳定后，再继续推进 `SkiaRenderer`，避免布局迁移和渲染迁移同时发生。

## 11. 布局示例

当前仓库已经把默认示例布局更新成 Yoga 展示版，文件在：

- `resource/layouts/ui.xml`
- `resource/layouts/ui.json`

这个示例重点演示了三件事：

- 固定高度区块和自适应高度区块混排。
- `Spacer + flexGrow=1 + flexBasis=0` 吸收剩余垂直空间。
- 底部操作区被稳定压到容器底边，而不是依赖手写坐标。
- 横向 `direction="row"` 工具条里，按钮和状态文本沿主轴分布。

### 11.1 XML 示例片段

```xml
<Panel height="620" padding="20,20,20,20" spacing="14" alignItems="stretch">
  <Panel height="96" />
  <Grid height="160" columns="4" spacing="12" rowHeight="136" />
  <Panel padding="16,16,16,16" spacing="8" />
  <Spacer flexGrow="1" flexBasis="0" />
  <Panel padding="16,16,16,16" spacing="10" />
</Panel>
```

### 11.2 JSON 示例片段

```json
{
  "type": "Spacer",
  "props": {
    "flexGrow": 1,
    "flexBasis": 0
  }
}
```

### 11.3 适合继续扩展的用法

基于当前这套桥接方式，接下来比较适合补的布局模式是：

- 顶部固定、底部固定、中间内容弹性撑开。
- 表单区固定高度，底部按钮区贴边。
- 卡片堆叠中插入一个或多个弹性间隔器。
- 横向工具条、筛选栏、标签条。
