# Wave1 · R 文本 parity 对照笔记

用于 **R1–R5** 手工/半自动验收。自动化辅助：

```powershell
.\scripts\run_text_dump.ps1 -Layout core-validation -Renderer skia
.\scripts\run_text_dump.ps1 -Layout core-validation -Renderer direct2d
.\scripts\run_text_dump.ps1 -Layout cjk-render -Renderer skia
```

输出 NDJSON 字段：`path, font, maxW, wrapW/H, nowrapW/H, boxW/H`。

## 固定条件

| 项 | 值 |
|----|-----|
| 客户区 | `AI_WIN_UI_SIZE=1000x700` |
| DPI | 优先 100% 主屏 @96 逻辑 DPI |
| 字体 | Segoe UI（Skia/DWrite 默认链） |
| 页面 | `layouts/core_validation.xml`、`layouts/cjk_render_test.xml` |

## R1 单行对照（判据 ≤2px 行高）

| 控件场景 | 页面区域 | Skia wrapH | D2D wrapH | 差 | OK? |
|----------|----------|------------|-----------|-----|-----|
| 标题 30px | Core Validation Surface | | | | |
| 正文 15px | 首段说明 | | | | |
| 按钮标签 14px | Primary action… | | | | |
| 输入 14px | Editing text… | | | | |

## R4 / R2 多行 wrap

| 场景 | 期望 |
|------|------|
| Constrained card 长句（含 CJK） | wrapH ≈ box 内多行；Skia↔D2D ≤2px |
| Narrow column | 断行不溢出卡片；高度一致 |
| Ultra-narrow 正常英文词 | 高度一致 |

```powershell
$env:AI_WIN_UI_SIZE = "1000x700"
.\scripts\compare_text_dumps.ps1 -Layout text-wrap -HeightsOnly -TolerancePx 2
```

## R3 caret

| 操作 | 期望 |
|------|------|
| TextInput 聚焦点中间 | 光标落在字符间隙 |
| 拖选 | 高亮与字符对齐，无整段偏移 |

## 已知可接受差（R5）

- CJK 回退字体若系统缺字，宽度差可能 >2px → 先固定 YaHei UI 再比。
- ClearType vs Skia 次像素抗锯齿：视觉略不同，以 **metrics** 为主。
- **超长无空格 token**（如 `longtokenwithoutspaces`）在极窄宽下，Skia emergency mid-word break 可能比 DWrite 多 1 行（wrapH 差 ~20px）。正常词边界换行已对齐。
- wrapW 在窄列上可差数 px（行宽填充策略不同），R2 以 **HeightsOnly** 为验收。

## 自动化对比

```powershell
.\scripts\compare_text_dumps.ps1 -Layout core-validation -TolerancePx 2
.\scripts\compare_text_dumps.ps1 -Layout text-wrap -HeightsOnly -TolerancePx 2
.\scripts\compare_text_dumps.ps1 -Layout cjk-render -TolerancePx 2
```

默认容差 **2px**（R1 判据）。失败会保留两侧 dump 路径。

> **脚本坑（已修）：** PowerShell 里 `if ($a > $b)` 是**重定向**不是比较，会写出垃圾文件 `2`/`1.5` 并永远 PASS。必须用 `-gt`。

## 状态

| ID | 状态 | 日期 |
|----|------|------|
| R1 规则表 + 对比脚本 | **PASS** core-validation @2px | 2026-07-15 |
| R2 多行 wrapH | **PASS** `text-wrap -HeightsOnly`（CJK soft-break + 脚本 -gt 修复） | 2026-07-15 |
| R3 caret | **done**（最近边界 snap） | 2026-07-15 |
| A6 TEXT_DUMP | **已实现** `AI_WIN_UI_TEXT_DUMP` | 2026-07-15 |
