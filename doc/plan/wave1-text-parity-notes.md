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

## R4 多行 wrap

| 场景 | 期望 |
|------|------|
| Constrained card 长句 | wrapH ≈ box 内多行；无底裁切 |
| Narrow column | 断行不溢出卡片 |

## R3 caret

| 操作 | 期望 |
|------|------|
| TextInput 聚焦点中间 | 光标落在字符间隙 |
| 拖选 | 高亮与字符对齐，无整段偏移 |

## 已知可接受差（R5）

- CJK 回退字体若系统缺字，宽度差可能 >2px → 先固定 YaHei UI 再比。
- ClearType vs Skia 次像素抗锯齿：视觉略不同，以 **metrics** 为主。

## 自动化对比

```powershell
.\scripts\compare_text_dumps.ps1 -Layout core-validation -TolerancePx 2
.\scripts\compare_text_dumps.ps1 -Layout cjk-render -TolerancePx 2
```

默认容差 **2px**（R1 判据）。失败会保留两侧 dump 路径。

## 状态

| ID | 状态 | 日期 |
|----|------|------|
| R1 规则表 + 对比脚本 | **PASS** core-validation @2px（2026-07-15 实测） | 2026-07-15 |
| R2 多行 golden | 用 compare 看 wrapH；大 diff 再修 Skia wrap | |
| R3 caret | 待 | |
| A6 TEXT_DUMP | **已实现** `AI_WIN_UI_TEXT_DUMP` | 2026-07-15 |
