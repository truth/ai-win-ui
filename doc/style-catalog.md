# Style Catalog（跨文件命名样式）

`StyleCatalog` 把可复用的 `ComponentStyle` 放在 `resource/styles/*.json`，布局用
`$style.name` 引用。它是 **主题 tokens（`$color.x`）之上的一层**：tokens 解析数值/颜色；
catalog 解析整套多状态外观（含嵌套 `itemStyle` / `tabStyle` / `dropdownStyle`）。

相关代码：`src/style_catalog.*`、`src/layout_parser.*`（`g_activeStyleCatalog`）、
`UIContext::styleCatalog`。布局语法总览见 `doc/layout-spec.md`。

---

## 加载时机

启动 `BuildUI` 时：

1. 若存在 `styles/default.json` → `StyleCatalog::LoadFromResource`（处理 `import[]`）
2. 若设置 `AI_WIN_UI_STYLES=styles/app.json` → 再 `MergeFile` 合并追加/覆盖同名样式
3. `UIContext.styleCatalog` 指向该实例；`LayoutParser::BuildFromFile` 解析时激活它

无 catalog 或未命中名称时：`$style.xxx` 静默跳过（Debug 输出
`[Style] Unknown $style...`），元素仍可用内联 `style` 对象或 quick-set 字段。

---

## 样式文件格式

路径相对于 resource provider 根（通常为 `resource/`）。

```json
{
  "import": ["buttons.json", "cards.json"],
  "styles": {
    "primaryButton": {
      "base": {
        "background": "#2D6CDF",
        "cornerRadius": 8,
        "border": { "width": 1, "color": "#4C8DFF" },
        "foreground": "#F5F8FC",
        "fontSize": 14,
        "padding": [14, 8, 14, 8]
      },
      "hover": { "background": "#3B7AE8" },
      "pressed": { "background": "#1F54B5" }
    },
    "dangerButton": {
      "extend": "$style.primaryButton",
      "base": {
        "background": "#C23B4A",
        "border": { "width": 1, "color": "#E85D6C" }
      }
    }
  }
}
```

| 字段 | 含义 |
|------|------|
| `import` | 字符串或字符串数组：相对本文件目录的其它样式 JSON。先 import，再解析本文件 `styles`。循环 import 会拒绝并打日志。 |
| `styles` | 命名样式表。若省略 `styles` 键，根对象里除 `import`/`styles` 外的对象键也当作样式名。 |
| 样式对象 | 与布局内联 `style` **同一套** `ParseComponentStyle` 规则（见下）。 |
| `extend` | 字符串：`$style.baseName` 或裸名。以已解析的 base 为起点，再覆盖本对象字段。同文件内任意 key 顺序均可（双遍解析）。 |

`import` 路径解析：先相对当前文件目录，再按 provider 绝对相对路径尝试。

仓库默认树：

```
resource/styles/
  default.json    # import buttons + cards，再定义 hub 用样式
  buttons.json
  cards.json
```

---

## 布局里如何引用（XML / JSON 规则影响）

### JSON

`props.style` 支持 **三种形态**（互斥，按解析顺序）：

| 形态 | 写法 | 说明 |
|------|------|------|
| 内联对象 | `"style": { "base": { ... }, "hover": { ... } }` | 与 v1 声明式样式相同；可含 `extend` |
| Catalog 字符串 | `"style": "$style.primaryButton"` | 推荐；前缀 `$style.` 可省略（裸名也会查 catalog） |
| styleRef | `"styleRef": "primaryButton"` | 裸名别名，不带 `$style.` 前缀 |

```json
{
  "type": "Button",
  "props": {
    "text": "Primary",
    "style": "$style.primaryButton"
  },
  "events": {
    "onClick": "primaryAction"
  }
}
```

注意：JSON 的 `onClick` 必须在 **`events.onClick`**，不能写在 `props` 里（与 XML 不同）。

### XML

| 属性 | 写法 | 说明 |
|------|------|------|
| `style` | `style="$style.primaryButton"` 或 `style="primaryButton"` | **仅字符串**引用 catalog；不能把整个 style 对象塞进 XML 属性 |
| `styleRef` | `styleRef="primaryButton"` | 与 JSON `styleRef` 相同 |

```xml
<Button text="Primary" style="$style.primaryButton" onClick="primaryAction" />
<Panel style="$style.surfaceCard" padding="16,16,16,16" spacing="12">
  ...
</Panel>
```

### 与内联 style / quick-set 的优先级

1. 解析时若命中 catalog → `SetStyle(catalog copy)`，标记 `m_styleFromLayout`
2. 同节点上的 quick-set（`background`、`cornerRadius` 等）仍会赋值公共字段
3. **绘制层行为因控件而异**：
   - `Button` / 多数交互控件：运行时 `Resolve(StyleState)` 以 `m_style` 为主
   - `Panel`：若 `m_styleFromLayout` 且 decoration 有内容，渲染时把 catalog 背景/边框/圆角折入绘制（显式 `background` 在 a>0 时仍可优先）
4. 主题 `DefaultStyle(theme)`：仅当布局 **未** 设置 style 时，`ApplyThemeDefaults` 才注入

### 内联 style 对象可写字段（catalog 与布局共用）

与 `doc/layout-spec.md`「Style System」一致，并支持：

| 键 | 用途 |
|----|------|
| `extend` | 继承 catalog 或已加载名 |
| `base` / `hover` / `pressed` / `focused` / `disabled` / `selected` / `readonly` | 状态覆盖 |
| 状态内 `background` / `border` / `cornerRadius` / `foreground` / `fontSize` / `padding` / `margin` / `borderWidth` / `opacity` | 外观 |
| `track` / `thumb` / `fill` | Slider、ProgressBar 等子装饰 |
| `itemStyle` | ListBox / ListView / TreeView / Menu 等行样式（嵌套完整 ComponentStyle） |
| `tabStyle` | TabControl 标签 |
| `dropdownStyle` | ComboBox 下拉面板 |

Catalog 里也可以写嵌套 `itemStyle`，布局 `$style.listShell` 一次带上。

---

## 与 Theme tokens 的关系

| 机制 | 语法 | 解析点 | 粒度 |
|------|------|--------|------|
| Theme | `$color.surface-1`、`$spacing.lg` | 字段值（颜色/数字） | 标量 |
| Style catalog | `$style.primaryButton` | 整个 `style` 属性 | 多状态 ComponentStyle |

可在 **styles JSON 的字段值** 里继续用 `$color.x`（解析时激活了 `g_activeTheme` 则生效）。  
不要把 `$style.xxx` 写进颜色字段；也不要把 `$color.xxx` 当成 style 名。

---

## 验证布局与脚本

| 资源 | 用途 |
|------|------|
| `resource/layouts/style_catalog_demo.xml` | XML：`$style` + ShapePanel 预览 |
| `resource/layouts/style_catalog_demo.json` | JSON：`style` 字符串 + `events.onClick` |
| `resource/layouts/shaped_windows_hub.xml` | catalog 按钮 + 打开异形窗 |
| 别名 | `style-catalog`、`shaped-hub`（`run_layout_demo.ps1`） |

```powershell
.\scripts\run_layout_demo.ps1 -Layout style-catalog -Renderer skia
$env:AI_WIN_UI_STYLES = "styles/default.json"   # 可选；default 已自动加载
```

---

## 限制

- 样式文件 **仅 JSON**（布局仍 XML/JSON 双格式）
- 无 CSS 选择器 / 运行时热切换 catalog（需重启进程或重新 `BuildUI`）
- XML 不能内联复杂 style 对象；复杂定义放 styles JSON，布局只引用
- `extend` 依赖 base 已在 catalog 中（import 先合并；同文件双遍解析）
- 未知 `$style` 名不崩溃，但外观回退到控件默认
