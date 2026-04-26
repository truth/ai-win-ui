# Plan: SVG 集成 v1（Step 4 — SkSVG + SvgIcon 组件）

## Context

dashboard / 工具栏 / 状态指示等场景需要矢量图标——bitmap 在高 DPI 屏会糊，每种尺寸打多张是设计师/工程师都不愿意的。SVG 是矢量原生方案，Skia 的 `modules/svg` (SkSVGDOM) 现成可用。

**先决条件已经具备**（这是把工作量从"1 周"降到"~3 天"的关键）：
- [third_party/skia-m124](third_party/skia-m124) 含完整 Skia + SVG module headers
- [third_party/skia-m124/out/Debug-x64/svg.lib](third_party/skia-m124/out/Debug-x64/svg.lib) 与 Release-x64 都已预编译
- [src/renderer_skia.cpp](src/renderer_skia.cpp) 407 行已实现所有 12 个 IRenderer 方法（CPU raster via SkSurfaces::WrapPixels），生产可用
- [CMakeLists.txt:81-91](CMakeLists.txt#L81) 已 GLOB Debug-x64/*.lib，svg.lib 自动链接
- [src/resource_provider.h](src/resource_provider.h) 已有 `LoadBytes(path)`，无需扩展

唯一缺的是：编译开关默认 OFF + IRenderer 没有 SVG 接口 + 没有 SvgIcon 组件。

## 用户拍板的两个决策

- ✅ **CMake 自动检测 Skia SDK**：检测到 `third_party/skia-m124/out/Debug-x64/skia.lib` 就自动 enable + provider=local-sdk + dir 指向它；检测不到回退 OFF。零配置。
- ✅ **D2D backend 下 SvgIcon 画占位符**：灰框 + 居中 "SVG" 文字。让用户立刻知道需要切 backend，而不是静默空白。

## 架构

### 数据类型（[src/graphics_types.h](src/graphics_types.h) 新增）

```cpp
class SvgResource {
public:
    virtual ~SvgResource() = default;
    virtual Size GetIntrinsicSize() const = 0;  // SVG viewBox / width-height
};
using SvgHandle = std::shared_ptr<SvgResource>;
```

完全镜像 [BitmapResource / BitmapHandle](src/graphics_types.h#L60) 的设计——两者生命周期模式一致。

### IRenderer 接口扩展（[src/renderer.h](src/renderer.h) 三个新方法）

```cpp
virtual SvgHandle CreateSvgFromBytes(const uint8_t* data, size_t size) = 0;
virtual Size GetSvgSize(SvgHandle svg) = 0;
virtual void DrawSvg(SvgHandle svg, const Rect& rect) = 0;
```

**注意**：双 backend 都必须实现（因为是 pure virtual）。

### Skia backend 实现（[src/renderer_skia.cpp](src/renderer_skia.cpp) 加 ~80 行）

- `SkiaSvgResource : public SvgResource`：持有 `sk_sp<SkSVGDOM>`，构造时解析。`GetIntrinsicSize()` 读 `containerSize()` 或 `viewBox`。
- `CreateSvgFromBytes`：用 `SkMemoryStream` 包字节 → `SkSVGDOM::Builder().make(stream, font_mgr)`。font_mgr 复用 [skia_font::CreateDefaultFontManager()](src/renderer_skia.cpp#L339)。
- `DrawSvg`：`canvas->save()` → `translate(rect.left, rect.top)` → `setContainerSize(rect.size)` → `dom->render(canvas)` → `restore()`。
- 失败时（无效 SVG）`CreateSvgFromBytes` 返回 nullptr；`DrawSvg` 收到 nullptr 走 placeholder。

包含的头文件：`#include "modules/svg/include/SkSVGDOM.h"`、`#include "include/core/SkStream.h"`。

### Direct2D backend 实现（[src/renderer.cpp](src/renderer.cpp) 加 ~30 行）

- `Direct2DSvgResource : public SvgResource`：持有空，`GetIntrinsicSize()` 返回 `{16, 16}` 默认。
- `CreateSvgFromBytes`：返回一个 sentinel `Direct2DSvgResource`（即使解析不了，记录"曾经请求过 SVG"）。
- `DrawSvg`：画 `FillRect(rect, dimGray)` + `DrawRect(rect, gray, 1)` + `DrawTextW(L"SVG", center, ...)`。

### SvgIcon 组件（[src/ui.h](src/ui.h) 加 ~80 行，跟在 [Image (line 1404)](src/ui.h#L1404) 后面）

完全镜像 Image 的模式：

```cpp
class SvgIcon : public UIElement {
public:
    explicit SvgIcon(std::wstring source);

    enum class Stretch { Fill, Uniform, UniformToFill };
    Stretch stretch = Stretch::Uniform;

    void SetSvgData(std::vector<uint8_t> bytes);  // LayoutParser 调

protected:
    float MeasurePreferredWidth(float availableWidth) const override;
    float MeasurePreferredHeight(float width) const override;

public:
    void Render(IRenderer& renderer) override;
    // 内部：第一次 Render 时 CreateSvgFromBytes 缓存 SvgHandle

private:
    std::wstring m_source;
    std::vector<uint8_t> m_svgData;
    SvgHandle m_svg = nullptr;
    Size m_intrinsicSize = {16, 16};  // fallback before first render
};
```

**v1 不做**：tint（着色），缓存（同一文件多 Icon 实例分别解析，每个组件一份）。

### LayoutParser entry point（[src/layout_parser.cpp:1739](src/layout_parser.cpp#L1739) Image 旁加 ~25 行）

```json
{
  "type": "SvgIcon",
  "props": {
    "source": "icons/check.svg",
    "stretch": "uniform",
    "width": 24,
    "height": 24
  }
}
```

实施：照搬 Image 的 [parsing 模式](src/layout_parser.cpp#L1739)——`provider.LoadBytes(source)` + `icon->SetSvgData(...)`。

### CMake 自动检测（[CMakeLists.txt](CMakeLists.txt) 改动）

在 `option(AI_WIN_UI_ENABLE_SKIA ... OFF)` 上方加：

```cmake
# Auto-enable Skia when local SDK + libs are present, so users with the
# bundled third_party/skia-m124 get SVG support without flipping a flag.
set(_ai_win_ui_default_skia_dir "${CMAKE_SOURCE_DIR}/third_party/skia-m124")
set(_ai_win_ui_skia_autodetected OFF)
if(EXISTS "${_ai_win_ui_default_skia_dir}/out/Debug-x64/skia.lib"
   AND EXISTS "${_ai_win_ui_default_skia_dir}/out/Release-x64/skia.lib"
   AND EXISTS "${_ai_win_ui_default_skia_dir}/modules/svg/include/SkSVGDOM.h")
    set(_ai_win_ui_skia_autodetected ON)
endif()

option(AI_WIN_UI_ENABLE_SKIA "..." ${_ai_win_ui_skia_autodetected})
if(_ai_win_ui_skia_autodetected AND NOT AI_WIN_UI_SKIA_DIR)
    set(AI_WIN_UI_SKIA_DIR "${_ai_win_ui_default_skia_dir}" CACHE PATH "..." FORCE)
    set(AI_WIN_UI_SKIA_PROVIDER "local-sdk" CACHE STRING "..." FORCE)
endif()
```

**运行时 backend 默认仍为 D2D**——不改 [main.cpp:124 ParseRendererBackend](src/main.cpp#L124)。要看 SVG 用环境变量 `AI_WIN_UI_RENDERER=skia` 切换。这避免破坏 D2D 优先用户的现有体验。

## 关键文件汇总

| 文件 | 改动 | 行数估计 |
|---|---|---|
| [src/graphics_types.h](src/graphics_types.h) | + SvgResource / SvgHandle | +10 |
| [src/renderer.h](src/renderer.h) | + 3 个 SVG 虚方法 | +10 |
| [src/renderer.cpp](src/renderer.cpp) | + Direct2D placeholder 实现 | +30 |
| [src/renderer_skia.cpp](src/renderer_skia.cpp) | + SkiaSvgResource + 3 个方法 | +80 |
| [src/ui.h](src/ui.h) | + SvgIcon 类 | +80 |
| [src/layout_parser.cpp](src/layout_parser.cpp) | + SvgIcon JSON parsing | +25 |
| [CMakeLists.txt](CMakeLists.txt) | + 自动检测 Skia SDK + 链接 svg.lib（已自动 GLOB） | +15 |
| `resource/icons/` | + 几个测试 SVG（check / arrow / star） | 新增目录 |
| `resource/layouts/test_svg.json` | + SVG 演示 layout | 新增 |
| `doc/plan/2026-04-27-skia-svg-integration-v1.md` | + plan 归档 | 新增 |

## 不做的事（明确边界）

- ❌ **SVG 着色 (tint)**：用 SkColorFilter 改色需要重 render，v1 直接出原色
- ❌ **SVG 缓存**：同一 source 多个 SvgIcon 各自解析，不共享。Skia 的 SkSVGDOM 解析对中等图标毫秒级开销，v1 接受
- ❌ **SVG 动画 (SMIL)**：SkSVGDOM 不完整支持
- ❌ **外部 SVG 资源 `xlink:href`**：v1 只支持 self-contained SVG
- ❌ **D2D 真画 SVG**：仅占位符。要看真 SVG 切 Skia backend
- ❌ **SvgIcon 改 Skia 默认 backend**：现有 D2D 用户不受影响

## 验证

### 编译验证
1. `.\build.ps1` —— CMake 应自动启用 Skia（检测到 third_party/skia-m124）
2. 检查 cmake 输出含 `Skia will be resolved from a local SDK`
3. binary 应正常生成

### 功能验证
1. 准备 3 个 SVG 文件放 `resource/icons/`：
   - `check.svg`（简单勾选）
   - `arrow-right.svg`（路径填充）
   - `star.svg`（多色或渐变）
2. 写 `resource/layouts/test_svg.json`：5 个 SvgIcon，覆盖三种 stretch + 不同 width/height
3. **Skia backend 验证**：
   ```powershell
   $env:AI_WIN_UI_RENDERER = "skia"
   $env:AI_WIN_UI_LAYOUT = "layouts/test_svg.json"
   .\build\Debug\ai_win_ui.exe
   ```
   预期：5 个图标按 SVG 内容显示，stretch 模式正确
4. **D2D placeholder 验证**：
   ```powershell
   Remove-Item Env:\AI_WIN_UI_RENDERER  # 默认回到 D2D
   .\build\Debug\ai_win_ui.exe
   ```
   预期：5 个 SVG 位置都是灰框 + "SVG" 字
5. **回归**：跑现有 ui.json + core_controls_v2.json + test_theme.json，视觉无变化（SVG 接口新增，不影响现有渲染路径）

## 风险与权衡

1. **CMake 自动检测可能误判**：如果 third_party/skia-m124 不完整（缺 .lib），autodetect 仍 ON 但 link 失败。Mitigation：检测条件检查 skia.lib + svg.lib + SkSVGDOM.h 三者都存在，缺一不开。
2. **Skia backend 现有 bug**：renderer_skia.cpp 是 CPU raster + StretchDIBits。性能可能比 D2D 差，但不阻塞 SVG 集成；性能优化是独立话题。
3. **SVG font 渲染**：SkSVGDOM 处理 `<text>` 元素时需要 SkFontMgr——已经通过 [CreateDefaultFontManager](src/renderer_skia.cpp#L339) 提供。
4. **SvgHandle = shared_ptr 的资源传递**：跨 backend 切换时（虽然 v1 不支持运行时切）旧 handle 会失效，需重新 CreateSvgFromBytes。SvgIcon 自己持有 m_svgData，重 render 时检测 backend 变化重建。v1 简化：不检测 backend 变化，只在第一次 Render 时建。
5. **占位符在 D2D 下的 hit-test 一致性**：占位符仍占 SvgIcon 的 m_bounds，hit-test 正常。

## 时间估算

- 类型 + IRenderer 接口扩展 + Direct2D placeholder：0.5 天
- Skia 实现 SkSVGDOM 集成：1 天
- SvgIcon 组件 + LayoutParser parsing：0.5 天
- CMake autodetect + 验证 Skia 链接：0.5 天
- SVG 资源准备 + test_svg.json + 端到端验证：0.5 天
- **合计：~3 天**

## 后续（v2 / 不在本计划范围）

- SVG tint：用 SkColorFilter 着色 + LayoutParser 接 `tint="$color.fg"` 字段
- SVG 缓存：跨 SvgIcon 共享 SvgHandle（按 source path key）
- 把 dashboard 的图标位换成 SvgIcon 替换 PNG
- Slider/ListBox 接 sub-style ComponentStyle（独立 plan）
- Step 5 Grid 布局
