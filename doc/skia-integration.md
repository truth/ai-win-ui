# Skia 集成分析

本项目当前使用 Direct2D 作为渲染后端。若希望引入 Skia 以增强渲染能力，可按以下思路整合：

## 1. 目标

- 使用 Skia 作为渲染层，替代或并行于现有 Direct2D。 
- 保持现有 UI 树与事件系统不变。
- 支持组件事件交互、文本渲染与 UI 形状绘制。

## 2. 为什么选择 Skia

- Skia 提供跨平台 GPU/CPU 渲染支持。
- 丰富的图形 API，可以实现圆角、不规则遮罩、渐变、阴影等效果。
- 可以与 Windows 的 D3D11、ANGLE、Vulkan、CPU raster 等后端配合。

## 3. 设计方案

### 3.1 抽象渲染接口

建议先将当前 `Renderer` 封装成一个渲染接口：

- `IRenderer`
  - `bool Initialize(HWND hwnd)`
  - `void Resize(UINT width, UINT height)`
  - `void BeginFrame(const Color& clearColor)`
  - `void EndFrame()`
  - `void FillRect(const Rect& rect, const Color& color)`
  - `void DrawRect(const Rect& rect, const Color& color, float strokeWidth)`
  - `void DrawTextW(...)`

然后保留 `D2DRenderer`，并新增 `SkiaRenderer`。

### 3.2 Skia 渲染后端实现路径

#### 路径 A：GPU 后端（推荐）

1. 使用 D3D11 设备创建 `GrDirectContext`。
2. 通过 `SkSurface::MakeFromBackendRenderTarget` 创建 Skia surface。
3. 获得 `SkCanvas` 进行绘制。
4. 绘制结束后执行 `SkSurface::flush()`。

优点：
- 渲染性能高
- 可利用硬件加速

缺点：
- 依赖 D3D11/ANGLE/Vulkan 等后端，集成复杂度稍高。

#### 路径 B：CPU Raster 后端（更简单）

1. 使用 Skia 提供的 raster 后端创建 `SkSurface`。
2. 直接绘制到内存像素缓冲区。
3. 使用 Win32 GDI/Direct2D 或 `UpdateLayeredWindow` 将结果提交到窗口。

优点：
- 集成简单，适合作为 PoC。
- 可绕过 GPU 后端依赖。

缺点：
- 性能低于 GPU。

## 4. CMake 集成示例

可以在 `CMakeLists.txt` 中新增：

```cmake
find_package(Skia REQUIRED CONFIG)

target_include_directories(ai_win_ui PRIVATE ${SKIA_INCLUDE_DIRS})

target_link_libraries(ai_win_ui PRIVATE
    d2d1
    dwrite
    windowscodecs
    skia
)
```

如果没有 `find_package(Skia)`，可直接指定 Skia 的 include 路径与库路径：

```cmake
target_include_directories(ai_win_ui PRIVATE "${SKIA_DIR}/include")
link_directories("${SKIA_DIR}/lib")
target_link_libraries(ai_win_ui PRIVATE skia)
```

## 5. SkiaRenderer 关键实现

### 5.1 初始化

- `SkiaRenderer::Initialize(HWND hwnd)`
  - 使用 `SkSurfaceProps`、`SkFontMgr`、`SkTypeface`。
  - 若使用 GPU 后端，则创建 D3D11 设备/上下文并构建 `GrDirectContext`。

### 5.2 绘制方法对应关系

- `FillRect` -> `SkCanvas::drawRect`
- `DrawRect` -> `SkCanvas::drawRect` + `SkPaint::setStyle(SkPaint::kStroke_Style)`
- `DrawTextW` -> `SkTextBlob::MakeFromText` 或 `SkFont::measureText`

### 5.3 字体与文本

Skia 的文本绘制建议使用 `SkFont` + `SkTextBlob`：

- 支持 Unicode / Wide string。
- 可将 `wchar_t*` 转换为 UTF-8，或者使用 `SkTextUtils::DrawString`。
- 也可以配合 `SkFontMgr::matchFamilyStyle` 加载 `Segoe UI`。

## 6. 与当前 UI 结构整合

### 6.1 保持 UI 逻辑不变

当前 `src/ui.h` 的布局和事件分发已具备：
- 绝对布局由 `Panel::Arrange` 计算。
- 事件分发由 `OnMouseMove/Down/Up` 递归实现。
- `Button` 已支持 hover/pressed/click。

Skia 只需替换绘制实现，不需要改动事件系统。

### 6.2 可能需要进一步的适配

- 将 `D2D1_RECT_F`、`D2D1_COLOR_F` 抽象为项目级 `Rect` 和 `Color` 类型。
- 在 `Renderer` 接口层统一坐标与颜色格式，避免依赖 Direct2D SDK。

## 7. Yoga 布局引擎

若希望将当前的 UI 布局扩展为真正的 Flex 布局引擎，推荐引入 Facebook 的 `Yoga`：

- `Yoga` 本身就是基于 Flexbox 的布局引擎，可以为 `Panel` 和其它容器提供 `flex-direction`、`justify-content`、`align-items`、`flex-grow`、`flex-shrink` 等属性。
- 当前 `Panel::Arrange` 可替换为 `Yoga` 计算布局结果后再设置子元素边界。
- 事件交互层无需改动，继续使用现有 `UIElement` 树的 hit test 与 mouse 事件分发。

### 7.1 集成方式

1. 在 CMake 中添加 Yoga 源码或库，推荐使用 `git submodule` 或通过包管理器获取。 
2. 为每个 `UIElement` 保存一个 `YGNodeRef`：
   - `YGNodeStyleSetFlexDirection(node, YGFlexDirectionColumn);`
   - `YGNodeStyleSetJustifyContent(node, YGJustifyFlexStart);`
   - `YGNodeStyleSetAlignItems(node, YGAlignStretch);`
   - `YGNodeStyleSetFlexGrow(node, 1.0f);`
3. 在布局阶段调用 `YGNodeCalculateLayout(rootNode, width, height, YGDirectionLTR);`
4. 从 Yoga 的 `YGNodeLayoutGetLeft/Top/Width/Height` 读取最终位置并传给 `UIElement::SetBounds()`。

### 7.2 适配建议

- 抽象现有 `Panel` 布局，使其仅负责“子元素容器逻辑”，并把实际布局委托给 Yoga。
- 保持 `Panel` 的 padding/spacing 逻辑，可在 Yoga 的容器节点上使用 `YGNodeStyleSetPadding` 和额外的占位子节点实现间距。
- 对于非容器元素（如 `Label`、`Button`），可继续使用固定高度或 `auto` 高度，Yoga 会根据 `YGNodeStyleSetFlexBasis` 自动计算。

### 7.3 CMake 配置参考

```cmake
add_subdirectory(external/yoga)

target_link_libraries(ai_win_ui PRIVATE yoga)
```

或通过预编译库：

```cmake
find_package(Yoga REQUIRED)

target_link_libraries(ai_win_ui PRIVATE Yoga::Yoga)
```

### 7.4 结合 Skia 的整体方案

- Yoga 负责布局，Skia 负责绘制。
- 先执行 `Yoga` 布局计算，再在 `Render()` 中调用 `SkiaRenderer` 绘制最终边界。
- 组件状态（hover/pressed）仍由事件系统驱动，布局与绘制分离。

## 8. 不规则窗口与无 title 模式

Skia 本身只负责内部绘制，窗口形状由 Win32 控制：

- 无 title 模式：使用 `WS_POPUP` 或 `WS_OVERLAPPEDWINDOW` 并隐藏标题栏。
- 自定义拖拽区：在顶部区域处理 `WM_LBUTTONDOWN`，并调用 `ReleaseCapture()` + `SendMessage(hwnd, WM_NCLBUTTONDOWN, HTCAPTION, 0)`。
- 不规则窗口：使用 `SetWindowRgn` 或 `UpdateLayeredWindow` 创建圆角/复杂形状。

内部布局仍然可以用 `Panel` + flex 规则保持一致。

## 8. 组件事件交互扩展建议

- 增加 `OnMouseLeave`、`OnMouseEnter`、`OnKeyDown`、`OnKeyUp`。
- 支持 `Focusable` 元素与键盘焦点。
- 引入 `Click`, `Hover`, `Press`, `Drag` 等通用事件回调。
- 对于不规则窗口，补充可见性/遮罩命中测试，以避免透明区域误触。

---

创建本分析文档的目的是让 `ai-win-ui` 项目能够：
- 清晰定义当前 Direct2D 渲染与事件处理能力；
- 规划 Skia 作为后端替代的实现路径；
- 保持组件交互与窗口布局的稳定性。
