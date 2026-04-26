# Plan: Theme Tokens v1（Step 3）

## Context

Step 0/1/2 已经把样式系统的运行时数据结构搭好（`BoxDecoration`、`ComponentStyle`、`StyleState`），但所有颜色/间距/圆角仍然是散落各处的 hex 字面量与数字常量：
- C++ 端：`Button::DefaultStyle()` 写死 `0x2D2D30`/`0x3E3E42`/`0x0E639C`/`0xFFFFFF`
- JSON 端：[resource/layouts/ui.json](resource/layouts/ui.json) 里 `"background": "#0C1522"` 之类散布在几十个组件
- 改一次配色需要 grep 所有文件

Theme tokens 的目标：建立**命名表 + 引用解析**，让 layout JSON 用 `"$color.surface-1"` 替代裸 hex，配色变更只改一个 theme 文件。

**不做**（明确划出 v1 边界）：
- ❌ 运行时主题切换（启动时一次性加载，要换主题需重启）
- ❌ Token-in-token（一个 token 引用另一个 token）
- ❌ CSS 自定义属性那套 `var(--x, fallback)` 语法
- ❌ DefaultStyle() 改造为读 theme（保持硬编码 fallback，确保无 theme 文件时仍可运行）
- ❌ 每个字符串都识别 token（仅在 LayoutParser 已知的颜色/数字字段位置识别）

## 替你拍板的设计决策

| 决策 | 选择 | 理由 |
|---|---|---|
| Token 引用语法 | `"$color.surface-1"` / `"$spacing.md"` | 简洁，类 SCSS。`{...}` 太占字符，`@xxx` 与 CSS at-rule 撞 |
| Token 类别 | colors / spacings / radii / fontSizes / borderWidths | 覆盖现有 StyleSpec 全部数值字段 |
| Theme 文件位置 | `resource/themes/default.json`，启动时加载 | 与 layout 解耦；`AI_WIN_UI_THEME` 环境变量可覆盖 |
| 运行时切换 | v1 不支持 | 要重 resolve 所有已构造组件的 m_style 中的硬解析值，复杂度大；v2 加 |
| Token 缺失行为 | 解析失败 → 返回 fallback color (magenta `#FF00FF`) + stderr warning | Magenta 是设计师约定的"missing texture"颜色，肉眼立刻能发现 |
| DefaultStyle() | 保持硬编码 hex | 确保无 theme 也能跑；运行时 layout 的 token 引用会覆盖 |
| Token 大小写 | 全小写 + 连字符（`surface-1` 而非 `Surface1`） | 与 Tailwind/Material design tokens 习惯一致 |

## 改动文件

| 文件 | 改动类型 |
|---|---|
| `src/theme.h` | 新增 |
| `src/theme.cpp` | 新增 |
| `src/ui_context.h` | + `Theme* theme = nullptr` 字段 |
| `src/main.cpp` | 启动时加载 theme + 注入 UIContext |
| `src/layout_parser.cpp` | `ColorFromString` 加重载支持 theme；数字字段解析 helper 加 token 识别 |
| `src/layout_parser.h` | 加 `ColorFromString(value, theme)` 重载声明 |
| `CMakeLists.txt` | + `src/theme.cpp` |
| `resource/themes/default.json` | 新增（v1 默认主题，复刻当前散落配色的命名集合） |

## 类型设计

```cpp
// src/theme.h
#pragma once
#include "graphics_types.h"
#include <optional>
#include <string>
#include <unordered_map>

class Theme {
public:
    std::unordered_map<std::string, Color> colors;
    std::unordered_map<std::string, float> spacings;
    std::unordered_map<std::string, float> radii;
    std::unordered_map<std::string, float> fontSizes;
    std::unordered_map<std::string, float> borderWidths;

    std::optional<Color> ResolveColor(const std::string& key) const;
    std::optional<float> ResolveNumber(const std::string& category, const std::string& key) const;

    static std::unique_ptr<Theme> LoadFromJson(const std::string& jsonText);
};
```

```cpp
// src/ui_context.h 新增字段
class Theme;
struct UIContext {
    // ...existing fields...
    Theme* theme = nullptr;
};
```

## Token 引用语法定义

**字符串字段**（颜色）：
```json
{ "background": "$color.surface-1" }     // 引用
{ "background": "#0C1522" }              // 仍支持裸 hex
{ "background": "transparent" }          // 仍支持 keyword
```

**数字字段**（spacing/radius/fontSize/borderWidth）：JSON 数字类型不能直接写 `$xxx.y`。两条路：
- 简洁：允许该字段同时接受 string 形式 `"$spacing.md"` 或 number `12`
- 一致：统一用对象 `{ "$ref": "spacing.md" }`

**v1 选简洁路线**：
```json
{ "padding": "$spacing.md" }                       // 字符串引用整个 thickness
{ "padding": [12, 12, 12, 12] }                     // 仍支持数组
{ "padding": ["$spacing.md", "$spacing.md", 12, 12] } // 混用
```

每个 ParseThickness/ParseNumber helper 接 theme 后自动识别 `$xxx.yyy` 字符串。

## Theme JSON 文件结构

```json
{
  "colors": {
    "surface-0":   "#0A0E15",
    "surface-1":   "#0C1522",
    "surface-2":   "#122032",
    "surface-3":   "#1A1F2C",
    "border":      "#6A6A6A",
    "border-focus":"#FFFFFF",
    "fg":          "#EDEDED",
    "fg-muted":    "#B8C8D9",
    "accent":      "#3A86FF",
    "success":     "#2A8C57",
    "warning":     "#E0B341",
    "danger":      "#D14545",
    "btn-bg":      "#2D2D30",
    "btn-bg-hover":"#3E3E42",
    "btn-bg-press":"#0E639C"
  },
  "spacings": {
    "xs": 4, "sm": 8, "md": 12, "lg": 16, "xl": 24, "2xl": 32
  },
  "radii": {
    "sm": 4, "md": 8, "lg": 12, "xl": 16
  },
  "fontSizes": {
    "xs": 11, "sm": 12, "base": 14, "lg": 16, "xl": 18, "2xl": 22, "3xl": 28
  },
  "borderWidths": {
    "thin": 1, "medium": 2, "thick": 4
  }
}
```

## 解析流程改动

### LayoutParser::ColorFromString 重载
```cpp
// 原 API 保留（无 theme 时走 fallback）
static Color ColorFromString(const std::string& value);

// 新 API：传入 theme 后识别 "$color.xxx" 引用
static Color ColorFromString(const std::string& value, const Theme* theme);
```

实现：
- 字符串以 `$color.` 开头 → 截后缀 → `theme->ResolveColor(key)` → 命中返回；miss 返回 magenta + warning
- 否则走原 ParseHexColor / keyword 路径

### 数字 token helper（新增）
```cpp
float ParseNumberWithTheme(const JsonValue& value, const std::string& category, const Theme* theme);
Thickness ParseThicknessWithTheme(const JsonValue& value, const std::string& category, const Theme* theme);
```

`category` 是 "spacing"/"radius"/"fontSize"/"borderWidth" 决定查哪张表。

### 调用点改造（最小侵入）

`ApplyCommonJsonProps` 接收 `Theme* theme` 参数（往上传到 `BuildFromJson`，由 `BuildFromFile` 从 UIContext 取），在解析 padding/margin/border/style 时传入 theme。**所有现有 hex/数字字面量解析路径保持向后兼容**——不写 `$` 前缀就走原逻辑。

### main.cpp 启动流程
```cpp
// 启动顺序：renderer → resourceProvider → theme → layoutEngine → 加载 layout
const std::wstring requestedTheme = GetEnvironmentValue(L"AI_WIN_UI_THEME");
const std::string themePath = requestedTheme.empty()
    ? "themes/default.json" : Utf16ToUtf8(requestedTheme);

if (provider.Exists(themePath)) {
    auto themeData = provider.LoadString(themePath);
    m_theme = Theme::LoadFromJson(themeData);
}
m_uiContext.theme = m_theme.get();  // 可能为 nullptr，所有解析点 nullptr-safe
```

## 验证

### 单元测试性的 layout
新建 `resource/layouts/test_theme.json`：
- 一个 Panel 用 `"background": "$color.surface-1"`
- 一个 Button style 全用 token：`base.background = $color.btn-bg`、`hover.background = $color.btn-bg-hover` 等
- 一个 Panel 用 `"padding": "$spacing.lg"` + `"cornerRadius": "$radii.md"`
- 一个故意写错的 `"$color.does-not-exist"` 验证 fallback magenta + warning

### 回归
- 跑现有 [resource/layouts/ui.json](resource/layouts/ui.json) 全 dashboard：所有 hex 字面量原样工作（不动 ui.json 文件）
- 跑 [resource/layouts/test_styles.json](resource/layouts/test_styles.json)（Step 2 验证 layout）：原样工作

### 端到端
- 设 `AI_WIN_UI_THEME=themes/default.json` → 跑 `test_theme.json` → token 全部解析、视觉与 hex 字面量等价
- 删除 themes 目录 → main.exe 启动不崩，theme 为 nullptr，layout 中 token 引用走 magenta fallback + 警告（实测警告会出现在调试输出中）

## 风险与权衡

1. **DefaultStyle() 不读 theme**：意味着代码里直接 `new Button()` 的组件外观与 theme 无关。这是有意为之——保证零 theme 也能运行；想让 default 也跟主题，需要在组件 `SetContext` 钩子里 lazy resolve（v2）。
2. **Magenta fallback**：丑但显眼。比静默用黑色 / 透明的"软失败"好得多——肉眼一秒发现 typo。
3. **Token 不嵌套**：`{"color": "$color.surface-1"}` 里 `surface-1` 不能再是别的 token。v1 不做，否则要做循环检测。
4. **静态优先级**：theme 在 LayoutParser 解析阶段消化掉，不进 ComponentStyle。意味着同一个 layout 跑两个 theme 文件需要重新 parse + 重建组件树——v1 接受，v2 改为延迟解析。
5. **数字字段同时接受 string/number**：增加 LayoutParser 的判断分支，但避免引入 `{"$ref": "x"}` 这种 verbose 语法。

## 时间估算
- 类型 + Theme::LoadFromJson：1 天
- LayoutParser 解析点接 theme：1-2 天
- main.cpp 启动流程 + default theme 文件：0.5 天
- 验证 layout + 回归测试：0.5 天
- **合计：~3 天**

## 后续（不在本计划范围）
- v2：运行时主题切换（重 resolve + 重建）
- v2：token-in-token + 循环检测
- v2：DefaultStyle() lazy resolve from theme
- v2：暗/亮主题打包（themes/dark.json、themes/light.json）
