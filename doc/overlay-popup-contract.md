# Overlay / Popup 契约（C2 → C4）

控件浮层（Combo 下拉、未来 Menu 飞出、通用 Popup）共用宿主三钩子，避免 z 序与点外关闭各写一套。

## UIElement 钩子

| 方法 | 时机 | 职责 |
|------|------|------|
| `RenderOverlay` | 主树 `Render` 之后第二遍 | 画出超出自身 layout 盒的浮层 |
| `FindOverlayHitAt(x,y)` | 鼠标命中（优先于普通树） | 返回消费该点的 overlay 所有者 |
| `DismissOverlaysAt(x,y)` | 点击且未命中任何 overlay | 关闭**不包含**该点的浮层 |
| `DismissAllOverlays()` | Esc 且焦点控件未消费 | 关闭**全部**打开的浮层 |

默认实现：向子节点递归。只有「拥有浮层」的控件覆盖后三者中相关项。

## 宿主（UiHost / main.cpp）

1. 绘制：`m_root->Render` → `m_root->RenderOverlay`
2. 鼠标按下：
   - `overlayHit = FindOverlayHitAt`
   - 若无 `overlayHit` → `DismissOverlaysAt`（点外关闭）
   - 命中优先走 overlay 的 `OnMouseDown`
3. 键盘：焦点 `OnKeyDown` 之后若仍为 `VK_ESCAPE` → `DismissAllOverlays`

## 已接入

| 控件 | RenderOverlay | FindOverlayHit | DismissAt | DismissAll |
|------|---------------|----------------|-----------|------------|
| **ComboBox** | 是 | 是 | 是 | 是 |
| **Popup** (C4) | 是 | 是 | 是 | 是 |
| ContextMenu | 否（常驻面板） | — | — | — |
| MenuStrip | 否（无 flyout） | — | — | — |

## Popup（C4）

```xml
<Popup name="demoPopup" triggerText="Open" width="150" height="36"
       popupWidth="280" popupHeight="150" placement="below|above">
  <Label text="Body…" />
</Popup>
```

| 属性 | 含义 |
|------|------|
| `triggerText` / `text` | 触发按钮文案 |
| `popupWidth` / `popupHeight` | 浮层逻辑尺寸 |
| `placement` | `below`（默认）或 `above`（不足时翻面） |
| `open` | 初始是否打开 |

事件：`togglePopup:elementName`（可选；直接点 trigger 也可切换）。

Demo：`layouts/popup_theme_demo.xml`（别名 `popup-theme`）。

## 验收

- Gallery → Core Controls V2：打开 Combo → 点页面空白处关闭；Esc 关闭  
- Gallery → Popup + Theme：打开 flyout → 点外/Esc 关闭；Theme Light/Dark 切换标题与 token 控件  
- `doc/keyboard-matrix.md` Combo Esc 行可标 ok  

## 相关代码

- `src/ui.h` — `UIElement` 钩子 + `ComboBox` / `Popup`  
- `src/main.cpp` — `UiHost` 鼠标/键盘路径；`applyTheme:` / `togglePopup:`  
- `src/ui_host.h` — 宿主公开 API 入口  

