# 控件键盘操作矩阵（Wave1 · C1）

手工验收时在 gallery 打开对应页，确认下列行为。状态：`ok` / `partial` / `missing`。

| 控件 | Tab 焦点 | 方向键 | Enter/Space | Esc | Home/End | PageUp/Dn | 其它 | 状态 |
|------|----------|--------|-------------|-----|----------|-----------|------|------|
| Button | 是 | — | 激活 click | — | — | — | | ok |
| TextInput | 是 | 左右移 caret | — | 清选区? | 行首/尾 | — | Backspace/Delete/Ctrl+C/V | partial |
| Checkbox | 是 | — | 切换 | — | — | — | | ok |
| RadioButton | 是 | 组内切换? | 选中 | — | — | — | | partial |
| Slider | 是 | 调值 | — | — | min/max? | 大步? | | partial |
| ProgressBar | 否 | — | — | — | — | — | 只读 | ok |
| ListBox | 是 | 上下选 | 确认? | — | 首/末 | 翻页 | | ok |
| ComboBox | 是 | 开列表后上下 | 开/选 | 关列表 | — | — | 点外关闭 | ok |
| TabControl | 是 | 左右 tab? | 激活 | — | — | — | | partial |
| ListView | 是 | 上下选 | — | — | 首/末 | 翻页 | 左右横滚 | ok |
| TreeView | 是 | 上下/左右展开 | — | — | — | — | | ok |
| DataTable | 是 | 选行 | F2 编辑 | 取消编辑? | — | — | Ctrl 多选 | partial |
| NumericUpDown | 是 | 上下步进 | — | — | — | — | | partial |
| DateTimePicker | 是 | 段内调整 | — | — | — | — | | partial |
| RichTextBox | 是 | 移动/选区 | — | — | — | — | 基础编辑 | partial |
| MenuStrip | 是 | 导航 | 激活 | 关菜单 | — | — | | partial |
| ContextMenu | 弹出后 | 上下 | 激活 | 关闭 | — | — | | partial |
| ScrollViewer | 否* | — | — | — | — | — | 滚轮/拖拽 | ok |

\* ScrollViewer 本身不抢 Tab；内容内焦点控件可聚焦。

## 已知缺口（C2/C3 后续）

1. Combo 下拉 **点外 + Esc 关闭**：`DismissOverlaysAt` / `DismissAllOverlays`（见 `overlay-popup-contract.md`）；通用 Popup 宿主仍待 **C4**。  
2. Radio 组内箭头键切换未强制。  
3. **IME**：caret 锚点 + candidate exclude + composition font 已加；CJK 实机仍建议手测。  
4. DataTable 编辑态 Esc 取消需验收。

## 相关页面

- `core_validation`、`core_controls_v2`、`navigation_components`、`advanced_inputs`、`table_components`
- 总入口：`demo_gallery`
