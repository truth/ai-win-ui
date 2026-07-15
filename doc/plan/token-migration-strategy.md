# Theme token 迁移策略（Wave1 · S1）

## 目标

布局与控件默认色逐步从硬编码 hex 迁到 `resource/themes/default.json` 的
`$color.*` / `$spacing.*` / `$radius.*` / `$fontSize.*`，便于换肤（S4）。

## 原则

1. **先 token 后批量替换**：先在 theme 文件补全键，再改布局。  
2. **布局显式色优先**：parser 里写死的 `color="#..."` 仍覆盖 theme DefaultStyle。  
3. **catalog 与 theme 分工**：  
   - `$color.x` = 标量  
   - `$style.x` = 多状态 ComponentStyle（可内部引用 `$color`）  
4. **不破坏 demo**：每次迁移一页，golden/smoke 通过后再下一批。

## 命名建议（初表）

| 用途 | 建议 key |
|------|----------|
| 页面底 | `surface-0` / `bg-app` |
| 卡片底 | `surface-1` / `surface-2` |
| 边框 | `border-subtle` / `border-focus` |
| 正文/弱化 | `fg` / `fg-muted` / `fg-strong` |
| 强调 | `accent` / `accent-soft` |
| 危险/成功 | `danger` / `success` |

已有 `DefaultStyle(theme)` 的控件优先用上表 key（见 `ComponentStyle::ThemeColor`）。

## 分批顺序

| 批次 | 范围 | 验收 |
|------|------|------|
| B1 | `core_validation` 背景/标题色 | 视觉 + measure golden |
| B2 | `demo_gallery` 卡片色 | gallery 打开正常 |
| B3 | `dashboard_*` | 手工 |
| B4 | 控件 DefaultStyle 审计 | 表：控件 → token 覆盖 |

## 禁止

- 在布局里发明未登记的 `$color.foo`（会品红 + 警告）  
- 一次 PR 改全部 json（冲突与回归面过大）

## 状态

| 项 | 状态 |
|----|------|
| 策略文档 | **done** 2026-07-15 |
| B1+ 迁移 | open |
