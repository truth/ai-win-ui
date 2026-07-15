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
| ContextMenu | 否（常驻面板） | — | — | — |
| MenuStrip | 否（无 flyout） | — | — | — |

## C4 目标：通用 Popup

计划形态（尚未实现）：

```text
PopupHost (UiElement 或 UiHost 服务)
  - Anchor rect / placement (below, above, flip)
  - content root
  - light-dismiss + Esc
  - z 序：统一走 RenderOverlay
```

Combo / 未来 Context flyout 迁入或适配该宿主，而不是再复制命中逻辑。

## 验收

- Gallery → Core Controls V2：打开 Combo → 点页面空白处关闭；Esc 关闭  
- 打开 Combo → 点另一控件：列表关且焦点可转移  
- `doc/keyboard-matrix.md` Combo Esc 行可标 ok  

## 相关代码

- `src/ui.h` — `UIElement` 钩子 + `ComboBox` 覆盖  
- `src/main.cpp` — `UiHost` 鼠标/键盘路径  
- `src/ui_host.h` — 宿主公开 API 入口  
