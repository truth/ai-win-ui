# Plan: 声明式样式系统 v1（地基阶段，Step 0/1/2）

## Context

当前 `f:\AI-Projects\ai-win-ui` 是一个 C++/Skia/Yoga 的 Windows UI 框架，组件样式有三个累积痛点：

1. **每个组件在 `Render()` 里手写 fill+stroke+state 分支**——`m_pressed && m_hovered` / `m_hovered` / `m_hasFocus` 的颜色判断在 [Button (src/ui.h#L1532)](src/ui.h#L1532) 和 [TextInput (src/ui.h#L1939)](src/ui.h#L1939) 等组件里重复了 N 次。
2. **Border 不参与盒模型布局**——确认 bug：`YGNodeStyleSetBorder` 全项目从未调用，[layout_engine.cpp#L163-175](src/layout_engine.cpp#L163-L175) 只设置了 padding/margin。后果：当 border > 0 时，子内容会贴到边框外沿，例如 TextInput focus 时 border 1→2 px 会让内容跳 1 px。
3. **没有声明式样式入口**——[layout_parser.cpp](src/layout_parser.cpp) 解析 JSON 时把样式参数（cornerRadius/background/fontSize）作为顶层字段直接赋值给组件公共属性，没有结构化的 `style` 块，无法表达多状态外观。

**这次改动的目标**：建立"决定式样式系统"的最小可用版本——`BoxDecoration` + `ComponentStyle{base, overrides[StyleState]}`——为后续 Theme tokens、Grid 布局、SVG 集成打地基。**不做** CSS 选择器、cascade、specificity、渐变、阴影、运行时主题切换。

**用户拍板的两个设计决策**：
- ✅ Border 采用 **inset** 绘制（border 完全在 m_bounds 内，与 CSS box-sizing: border-box 一致）
- ✅ 现有组件公共字段（`button->background = ...`）**保留**作为 quick-set，setter 内部同步到 `m_style.base.decoration.background`，所有现有 ui.json 零改动

---

## Step 0: 修复 border 盒模型 bug（约 1 天）

### 改动文件
- [src/layout_engine.cpp#L163](src/layout_engine.cpp#L163)：在 `ApplyPadding`/`ApplyMargin` 旁加 `ApplyBorder()`。
- [src/layout_engine.h](src/layout_engine.h)：在 `LayoutSpec`/`ChildLayoutSpec` 加 `Thickness border` 字段。
- [src/layout_engine.cpp#L191](src/layout_engine.cpp#L191)（`BuildStackLayout`）：在 root 节点 `ApplyPadding(root, style.padding)` 后调用 `ApplyBorder(root, style.border)`，每个 child 节点同样。
- [src/ui.h](src/ui.h) 的 `UIElement` 基类：加 `Thickness m_border`（与 `m_margin` 并列），加 `void SetBorder(const Thickness&)`。
- [src/ui.h](src/ui.h) 的 `Panel`：把 `BuildStackLayout` 调用处的 `ChildLayoutSpec` 填充改为读取 `child.m_border`。

### 实现要点
```cpp
// layout_engine.cpp 新增
void ApplyBorder(YGNodeRef node, const LayoutSpacing& border) {
    YGNodeStyleSetBorder(node, YGEdgeLeft,   border.left);
    YGNodeStyleSetBorder(node, YGEdgeTop,    border.top);
    YGNodeStyleSetBorder(node, YGEdgeRight,  border.right);
    YGNodeStyleSetBorder(node, YGEdgeBottom, border.bottom);
}
```

### 验证
1. 新建临时 layout `resource/layouts/test_border.json`：Panel padding=16 border=4 background=#202020，里面放固定大小红色 child。
2. 修复前运行 `build.ps1 -Run`，截图——内容会越过边框 4 px。
3. 修复后再次运行——内容正确居中，边框完整可见。
4. 跑现有 [resource/layouts/ui.json](resource/layouts/ui.json) 全 dashboard，确认无回归（现有组件 border=0，不应有视觉变化）。

---

## Step 1: BoxDecoration + DrawBoxDecoration 自由函数（约 3-4 天）

### 新增文件
- **`src/box_decoration.h`**：定义 `CornerRadius`、`BorderSpec`、`BoxDecoration` 三个 struct + `DrawBoxDecoration` 自由函数声明。
- **`src/box_decoration.cpp`**：`DrawBoxDecoration` 实现，仅依赖现有 [IRenderer (src/renderer.h#L42)](src/renderer.h#L42) 原语（`FillRoundedRect` / `DrawRoundedRect` / `FillRect` / `DrawRect`）——**不修改 IRenderer 接口**，避免 Direct2D + Skia 双 backend 都要改。

### 类型定义
```cpp
// box_decoration.h
struct CornerRadius {
    float topLeft = 0, topRight = 0, bottomRight = 0, bottomLeft = 0;
    static CornerRadius Uniform(float r) { return {r, r, r, r}; }
    bool IsUniform() const;
    float MaxRadius() const;
};

struct BorderSpec {
    Thickness width;     // 四边可独立；v1 调用方通常对称
    Color color;
    bool IsZero() const { return width.left==0 && width.top==0 && width.right==0 && width.bottom==0; }
};

struct BoxDecoration {
    Color background = ColorFromHex(0, 0.0f);  // 默认透明
    BorderSpec border;
    CornerRadius radius;
    float opacity = 1.0f;
    // v2 扩展点（不在本计划范围）：
    // Brush backgroundBrush;
    // std::vector<BoxShadow> shadows;
};

void DrawBoxDecoration(IRenderer& r, const Rect& bounds, const BoxDecoration& d);
```

### 实现要点（inset border 关键算法）
1. 取 `maxBorder = max(width.left, width.top, width.right, width.bottom)`。v1 假设四边等宽（不等宽 v2 处理）。
2. 背景填充矩形：`fillRect = bounds`（背景画在整个 bounds，被 border 覆盖外缘）。
3. **inset 关键**：Skia stroke 居中绘制，要让 stroke 完全在 bounds 内，传给 `DrawRoundedRect` 的矩形必须向内偏移 `maxBorder/2`，半径相应减小：
   ```cpp
   const float halfBorder = maxBorder * 0.5f;
   Rect strokeRect = bounds.Inset(halfBorder);
   float strokeRadius = max(0.0f, radius.topLeft - halfBorder);  // v1 假设 IsUniform
   r.FillRoundedRect(bounds, d.background, radius.topLeft);
   if (!d.border.IsZero()) {
       r.DrawRoundedRect(strokeRect, d.border.color, maxBorder, strokeRadius);
   }
   ```
4. `radius` 非均匀（四角不同）时，v1 用 `MaxRadius()` 退化为单值 + warning，v2 再做 `SkRRect`。
5. `opacity < 1.0f`：v1 用 `Color.a *= opacity` 简单实现，v2 走 layer。

### 替换 Panel 的 Render 路径作为样板
- [Panel::Render (src/ui.h#L569)](src/ui.h#L569) 改为：
  ```cpp
  void Render(IRenderer& r) override {
      BoxDecoration d{
          .background = background,
          .border = {{0,0,0,0}, ColorFromHex(0,0)},
          .radius = CornerRadius::Uniform(ScaleValue(cornerRadius)),
      };
      DrawBoxDecoration(r, m_bounds, d);
      // 子节点 render 由基类循环
  }
  ```

### 验证
1. 用 [resource/layouts/ui.json](resource/layouts/ui.json) 跑 main.exe，所有 Panel 视觉与改动前**像素级一致**（应该是，因为只是封装）。
2. 写一个临时 demo layout：Panel border=2 color=#FF0000 cornerRadius=8 background=#202020。视觉确认 border 完全在 bounds 内、不越界。

---

## Step 2: ComponentStyle + StyleState + 改造 Button & TextInput（约 1 周）

### 新增文件
- **`src/style.h`**：`StyleState` enum + `StyleSpec` struct + `ComponentStyle` struct + `Resolve` 声明。
- **`src/style.cpp`**：`ComponentStyle::Resolve(StyleState)` 实现——字段级合并（`std::optional` 字段缺省时不覆盖 base）。

### 类型定义
```cpp
// style.h
enum class StyleState : size_t {
    Normal = 0, Hover, Pressed, Focused, Disabled, Selected, ReadOnly,
    Count
};

struct StyleSpec {
    std::optional<BoxDecoration> decoration;
    std::optional<Thickness>     padding;
    std::optional<Thickness>     margin;
    std::optional<Thickness>     border;       // border thickness, 配合 decoration.border
    std::optional<Color>         foreground;   // 文本/前景色
    std::optional<float>         fontSize;
    std::optional<float>         opacity;
};

class ComponentStyle {
public:
    StyleSpec base;
    std::array<StyleSpec, static_cast<size_t>(StyleState::Count)> overrides;
    
    StyleSpec Resolve(StyleState state) const;  // 字段级合并：base 的每个字段被对应 override 覆盖（仅当 override 字段 has_value）
};
```

### UIElement 基类扩展（[src/ui.h#L25](src/ui.h#L25)）

**破坏性改动**：把 `m_hovered`、`m_pressed` 从派生类（Button [src/ui.h#L1461](src/ui.h#L1461)、TextInput [src/ui.h#L1615](src/ui.h#L1615)、Checkbox、RadioButton、Slider 等）提到基类，与现有 `m_hasFocus` 并列。

```cpp
// UIElement 基类新增
protected:
    bool m_hovered  = false;   // 提升自派生类
    bool m_pressed  = false;   // 提升自派生类
    bool m_disabled = false;   // 新增
    ComponentStyle m_style;
    
public:
    void SetStyle(ComponentStyle style) { m_style = std::move(style); }
    const ComponentStyle& Style() const { return m_style; }
    
    virtual StyleState GetCurrentState() const {
        // 优先级：Disabled > Pressed > Hover > Focused > Normal
        if (m_disabled) return StyleState::Disabled;
        if (m_pressed)  return StyleState::Pressed;
        if (m_hovered)  return StyleState::Hover;
        if (m_hasFocus) return StyleState::Focused;
        return StyleState::Normal;
    }
    
    void SetEnabled(bool e) { m_disabled = !e; }
```

**注意**：Selected 和 ReadOnly 状态由派生类自行 override `GetCurrentState()` 返回（如 ListBox 的 selected item、TextInput 的 readonly 模式）。

### 改造 Button（[src/ui.h#L1532](src/ui.h#L1532)）

```cpp
// Render 改为：
void Render(IRenderer& renderer) override {
    const StyleSpec resolved = m_style.Resolve(GetCurrentState());
    const BoxDecoration deco = resolved.decoration.value_or(BoxDecoration{});
    DrawBoxDecoration(renderer, m_bounds, deco);
    
    Rect textRect = m_bounds;
    textRect.left += ScaleValue(10.0f);
    const Color fg = resolved.foreground.value_or(foreground);
    const float fs = resolved.fontSize.value_or(fontSize);
    const TextRenderOptions opts{TextWrapMode::NoWrap, TextHorizontalAlign::Center, TextVerticalAlign::Center};
    renderer.DrawTextW(m_text.c_str(), m_text.size(), textRect, fg, ScaleValue(fs), opts);
}

// 提供静态默认样式工厂，复刻当前外观：
static ComponentStyle Button::DefaultStyle() {
    ComponentStyle s;
    s.base.decoration = BoxDecoration{
        .background = ColorFromHex(0x2D2D30),
        .border = {{1,1,1,1}, ColorFromHex(0x6A6A6A)},
        .radius = CornerRadius::Uniform(4.0f),
    };
    s.overrides[(size_t)StyleState::Hover].decoration = BoxDecoration{
        .background = ColorFromHex(0x3E3E42),
        .border = {{1,1,1,1}, ColorFromHex(0x6A6A6A)},
        .radius = CornerRadius::Uniform(4.0f),
    };
    s.overrides[(size_t)StyleState::Pressed].decoration = BoxDecoration{
        .background = ColorFromHex(0x0E639C),
        .border = {{1,1,1,1}, ColorFromHex(0x6A6A6A)},
        .radius = CornerRadius::Uniform(4.0f),
    };
    s.overrides[(size_t)StyleState::Focused].decoration = BoxDecoration{
        .background = ColorFromHex(0x2D2D30),
        .border = {{2,2,2,2}, ColorFromHex(0xFFFFFF)},  // focus ring 粗化
        .radius = CornerRadius::Uniform(4.0f),
    };
    return s;
}

// 构造函数里：m_style = DefaultStyle();
```

### Quick-set 兼容（保留公共字段）

`background`、`cornerRadius`、`fontSize`、`foreground` 字段仍然是 public，但要变成 setter 同步到 `m_style.base`：

```cpp
// 改写法：把 public 字段改为 getter + 内部自动同步
public:
    void SetBackground(Color c) { 
        background = c; 
        if (m_style.base.decoration) m_style.base.decoration->background = c;
        else m_style.base.decoration = BoxDecoration{.background = c};
    }
    Color background = ColorFromHex(0x2D2D30);  // public 字段保留给现有代码（先这样，迁移后改为只读）
```

**实施策略**：v1 先保留 public 字段裸赋值（`button->background = ...` 仍工作），但在 `Render()` 调用前的某个钩子里同步——具体方案：在 Render 入口加一个 `SyncQuickSetToStyle()` 钩子，对比 public 字段与 `m_style.base.decoration->background`，若不同则覆盖到 style。这样向后兼容零成本，新代码用 `SetStyle()` 走完整路径。

### 改造 TextInput（[src/ui.h#L1939](src/ui.h#L1939)）

- Render 流程同 Button，先 `DrawBoxDecoration`，再画选区/文本/光标。
- focus 态 border 1→2 通过 `overrides[Focused].decoration->border.width = {2,2,2,2}` 表达。
- 选区高亮（[src/ui.h#L1965](src/ui.h#L1965) 的蓝色 FillRect）和光标（[src/ui.h#L1995](src/ui.h#L1995) 的 1px DrawRect）**不属于 BoxDecoration**，保留组件自己画。

### LayoutParser 扩展（[src/layout_parser.cpp#L1459](src/layout_parser.cpp#L1459)）

在 `ApplyCommonProperties` 或单组件解析里识别 `"style"` 嵌套对象：

```json
{
  "type": "Button",
  "text": "Submit",
  "style": {
    "base":    { "background": "#2D2D30", "border": {"width": 1, "color": "#6A6A6A"}, "cornerRadius": 4 },
    "hover":   { "background": "#3E3E42" },
    "pressed": { "background": "#0E639C" },
    "focused": { "border": {"width": 2, "color": "#FFFFFF"} }
  }
}
```

实施：在 [src/layout_parser.cpp](src/layout_parser.cpp) 加 `ParseStyleSpec(const JsonValue&)` 和 `ParseComponentStyle(const JsonValue&)` 两个工具函数，返回 `ComponentStyle`，赋值给 `element->SetStyle(...)`。颜色解析复用现有 hex 字符串解析。

### CMakeLists.txt 更新

```cmake
# 在 src 文件列表加：
src/box_decoration.h
src/box_decoration.cpp
src/style.h
src/style.cpp
```

### 关键文件汇总

| 文件 | 改动类型 | 关键行 |
|---|---|---|
| `src/box_decoration.h` | 新增 | — |
| `src/box_decoration.cpp` | 新增 | — |
| `src/style.h` | 新增 | — |
| `src/style.cpp` | 新增 | — |
| [src/layout_engine.cpp](src/layout_engine.cpp) | 加 `ApplyBorder` 函数 | L163 旁 |
| [src/layout_engine.h](src/layout_engine.h) | `LayoutSpec`/`ChildLayoutSpec` 加 border | — |
| [src/ui.h](src/ui.h) UIElement | 加 m_hovered/m_pressed/m_disabled/m_style | L25-308 |
| [src/ui.h](src/ui.h) Button | Render 重写 + DefaultStyle() | L1461-1610 |
| [src/ui.h](src/ui.h) TextInput | Render 重写 + DefaultStyle() | L1615-2050 |
| [src/ui.h](src/ui.h) Panel | Render 走 DrawBoxDecoration | L569 |
| [src/layout_parser.cpp](src/layout_parser.cpp) | 加 ParseStyleSpec/ParseComponentStyle | L1459 区 |
| [CMakeLists.txt](CMakeLists.txt) | 注册新源文件 | — |

---

## 不做的事（明确边界）

- ❌ 渐变 / 图片背景 / 多层背景（v2，等 `Brush` 类型）
- ❌ box-shadow（v2，需要 Skia `SkDropShadowImageFilter` 或 layer 实现）
- ❌ 主题 token 系统（Step 3，独立计划）
- ❌ CSS 选择器 / specificity / cascade
- ❌ Grid 布局（Step 5，独立计划）
- ❌ SVG 集成（Step 4，独立计划，可与本计划并行）
- ❌ 单元测试框架（项目原本就没有，本计划不引入）
- ❌ 改造除 Button/TextInput/Panel 之外的组件（Checkbox/RadioButton/Slider/ListBox 等等下一波，不在 Step 2 范围内——它们在过渡期继续用旧 Render 路径，可以工作但没享受到样式系统）

---

## 风险与权衡

1. **状态字段提升是破坏性改动**——Button、TextInput、Checkbox、RadioButton、Slider、TabControl、ListBox 等都有 `m_hovered`/`m_pressed` 私有字段需要删掉用基类版本。`grep -n "m_hovered\|m_pressed" src/ui.h` 一次性找出所有点统一改。
2. **Quick-set 同步钩子的开销**——每帧 Render 时比对 public 字段与 style，是 O(组件数 × 字段数) 的比较。v1 接受，v2 改为 setter 主动写 style + 移除 public 字段。
3. **Border inset 的圆角失真**——`strokeRadius = radius - halfBorder` 在 border 厚度接近 radius 时圆角会消失。v1 接受，记录在代码注释里；用户极端用法（4px border + 4px radius）会得到直角。
4. **DrawBoxDecoration 不进 IRenderer**——意味着 BoxDecoration 加渐变/阴影时，要么扩展 IRenderer，要么 BoxDecoration 内部直接拿到 Skia/Direct2D 句柄旁路调用。届时再决策。
5. **现有 ui.json 零改动是硬约束**——`button->background = ...` 这条路必须继续工作，否则[resource/layouts/ui.json](resource/layouts/ui.json) 整个 dashboard 会崩。是 Step 2 的验收门。

---

## 端到端验证

### Step 0 验证
1. 临时新建 `resource/layouts/test_border.json`：Panel `padding=16 border=4 background=#202020`，里面放红色固定大小子节点。
2. 修复前 `build.ps1 -Run` → 截图，内容贴边外。
3. 修复后再次运行 → 内容正确居中。
4. 跑 [resource/layouts/ui.json](resource/layouts/ui.json) 全 dashboard，确认无回归（现有组件 border=0，应无视觉变化）。

### Step 1 验证
1. 跑现有 ui.json，所有 Panel 视觉与改动前**像素级一致**。
2. 临时 demo：Panel `border=2 color=#FF0000 radius=8 background=#202020`，确认 border 完全在 bounds 内（不越界）、圆角正确。

### Step 2 验证
1. **回归**：跑 [resource/layouts/ui.json](resource/layouts/ui.json) 全 dashboard，所有 Button/TextInput/Panel 视觉一致（依赖 `DefaultStyle()` 复刻准确性 + quick-set 同步钩子工作）。
2. **新功能**：写 `resource/layouts/test_styles.json`，用 `style` 块定义一个自定义 Button 的 base/hover/pressed/focused 四态颜色。运行 main.exe，鼠标 hover/click/Tab focus 时颜色按预期变化。
3. **TextInput focus 态**：运行 dashboard 中的 TextInput，Tab 进入时 border 由 1px 变 2px，**内容不跳动**（依赖 Step 0 的 inset + border 接入 Yoga）。这是 Step 0 与 Step 2 的协同验收点。
4. **Disabled 态**：在 test_styles.json 里给一个 Button 标 `"disabled": true`，运行确认对应 disabled 样式生效（默认透明度降低或灰阶背景）。
5. **错误样式**：layout 里写错颜色字符串、缺字段，确认 LayoutParser 输出 warning 而非崩溃。

### 时间合计
- Step 0: 1 天
- Step 1: 3-4 天
- Step 2: 5-7 天
- **总计：约 2 周**

---

## 后续计划（不在本次范围）

- **Step 3**：Theme tokens（颜色/间距/圆角命名表 + 引用解析），约 3-5 天
- **Step 4**：SkSVG 集成 + `<SvgIcon>` 组件，独立线，可与 Step 0-2 并行，约 3-5 天
- **Step 5**：Grid 布局（自实现 CSS Grid 子集，配合 Yoga），约 2-3 周——dashboard.png 作为验收

这三步在本计划完成后再各自规划。
