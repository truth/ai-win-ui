# DirectUI 产品化路线 · 分人物（角色）分工

**状态：** Active  
**日期：** 2026-07-15  
**前置：** P0 硬化已大部落地（ScrollViewer、多根、catalog、gallery、smoke/golden 骨架）  
**目标：** 从「可演示的 retained UI 引擎」演进为「可嵌入、可测、可交付的 DirectUI 产品层」

本文件回答三件事：

1. 产品化分哪几层、什么叫「够产品」  
2. **按人物（角色）** 怎么并行，谁拥有哪些文件与验收  
3. 时间片（Wave）如何串并行，避免互相踩踏  

---

## 1. 产品化愿景（三层）

```
┌─────────────────────────────────────────────────────────┐
│  L3  应用交付层   嵌入库 / 示例 App / 文档 / 打包发布      │
├─────────────────────────────────────────────────────────┤
│  L2  产品能力层   绑定·虚拟化·Popup·无障碍·换肤·动画       │
├─────────────────────────────────────────────────────────┤
│  L1  引擎可信层   文本 parity·布局稳定·测试·多 Host API    │  ← P0 大部分在此
└─────────────────────────────────────────────────────────┘
```

| 层级 | 「产品化完成」判据（摘要） |
|------|---------------------------|
| **L1** | 双后端文本/布局可 golden；无 COM 退出崩；多窗 API 稳定；smoke+measure CI |
| **L2** | 列表千行可滚；Popup 统一；可换肤；至少基础 UIA 名；IME 可用 |
| **L3** | 静态库/动态库可链；公开 C++ API 头；示例工程；版本号与 changelog |

P0 之后默认 **Wave 1 = 收口 L1**，再开 L2/L3 并行。

---

## 2. 人物（角色）一览

共 **6 个角色**。可一人兼多角；多人时按下表认领，**同文件尽量单写手**。

| 代号 | 人物 / 角色 | 使命一句话 | 主要领地（文件） |
|------|-------------|------------|------------------|
| **R** | **渲染与文本** Renderer/Text | 双后端画得准、量得准 | `renderer*.cpp`、`skia_text_*`、`text_measurer*`、`skia_font_shared` |
| **L** | **布局与滚动** Layout | Yoga/Flex 可信，滚动模型统一 | `layout_engine*`、`ui.h`（Panel/ScrollViewer/Grid）、窗口级 scroll 收敛 |
| **C** | **控件与交互** Controls | 列表/输入/浮层行为产品级 | `ui.h` 控件后半、parser 控件分支、overlay/Popup |
| **S** | **样式与主题** Style | 换肤、token、catalog 可运维 | `style*`、`theme*`、`resource/styles`、`resource/themes` |
| **H** | **宿主与多窗** Host | 可嵌入、多根、生命周期干净 | `main.cpp`→拆 `ui_host`/`application`、`window_chrome*` |
| **Q** | **质量与工具** QA/Tooling | 自动化、golden、gallery、CI | `scripts/*`、`tests/golden`、smoke、文档验收表 |

### 协作边界（防冲突）

| 冲突点 | 约定 |
|--------|------|
| `ui.h` | **按区段认领**：Panel/Scroll/Grid→**L**；List*/Input*/Popup→**C**；改接口先 PR 说明 |
| `layout_parser.*` | 新类型谁加控件谁改；**S** 只动 style 解析；合并前对方 review |
| `main.cpp` | **H** 主写；**Q** 只加 env/事件/调试钩子经 H ack |
| 资源 `resource/layouts` | 功能 owner 加 demo；**Q** 维护 `demo_gallery.xml` 索引 |
| 公开 API | **H** 冻结头文件路径；其它角色不私自改导出符号 |

---

## 3. 各人物任务板（可勾选）

### 3.1 R · 渲染与文本

**Wave 1（L1 收口）**

| ID | 任务 | 完成判据 | 依赖 |
|----|------|----------|------|
| R1 | A3：单行 Label/Button/TextInput Skia↔D2D 行高 ≤2px @96DPI | 对照表 + `cjk_render_test` 双后端截图/笔记 | — |
| R2 | A4：多行 wrap 断行与 measured height 一致 | golden 固定字符串高度 | R1 |
| R3 | A5：TextInput caret/选区矩形与 measure 同源 | 选区无跳变 | R1 |
| R4 | （可选）`AI_WIN_UI_TEXT_DUMP` 稳定 NDJSON | 脚本可 diff | R2 |
| R5 | 裁剪/位图采样 D2D↔Skia 已知差异写进 `skia-integration.md` | 文档「已知差」表 | — |

**Wave 2（L2）**

| ID | 任务 | 完成判据 |
|----|------|----------|
| R6 | 省略号 / 单行 ellipsis 策略 | DSL 或 TextRenderOptions 文档 |
| R7 | 渐变画刷 / 简单阴影原语（IRenderer） | 双后端 + demo 卡 |
| R8 | SvgIcon tint + handle 缓存 | 同 path 不重复 parse |

**不负责：** Yoga 规则、控件业务逻辑。

---

### 3.2 L · 布局与滚动

**Wave 1**

| ID | 任务 | 完成判据 | 依赖 |
|----|------|----------|------|
| L1 | 窗口级 `m_scrollOffset` 标 legacy；新 demo 只用 ScrollViewer | gallery/新页无依赖窗口滚 | — |
| L2 | ScrollViewer 双轴 + 嵌套滚轮消费矩阵写验收表 | 内 List 滚不带动外层 | — |
| L3 | Yoga edge：space-between + gap、min/max 组合用例 | `yoga_measure_cases` 增补 | — |
| L4 | measure golden 扩：`core_validation` + 双后端 | `run_measure_golden` 绿 | Q 协作 |

**Wave 2**

| ID | 任务 | 完成判据 |
|----|------|----------|
| L5 | 百分比宽高 / 基础 absolute（若产品需要） | 设计评审 + demo |
| L6 | GridPanel → Yoga grid 或文档化「永不替换」决策 | 决策写入 layout-spec |
| L7 | catalog `padding` 可选同步到 Yoga（opt-in） | 与 **S** 协议字段 |

**不负责：** 文本测量算法细节（归 R）。

---

### 3.3 C · 控件与交互

**Wave 1**

| ID | 任务 | 完成判据 | 依赖 |
|----|------|----------|------|
| C1 | 控件键盘操作矩阵（List/Combo/Tab/Table）文档 + 缺口补齐 | `mvp-acceptance` 增补 | — |
| C2 | Combo/ContextMenu overlay 命中与焦点陷阱清单 | 无「点外面关不了」 | — |
| C3 | IME：TextInput 合成串位置不飞 | CJK 输入可测 | R3 优先 |

**Wave 2（产品化关键）**

| ID | 任务 | 完成判据 |
|----|------|----------|
| C4 | **通用 Popup / Flyout** 宿主（统一 z 序与 hit） | Combo/Menu 迁入或适配 |
| C5 | **虚拟列表**（ListBox 或 ListView 其一）：`SetItemCount` + binder | 1k 行可滚流畅 |
| C6 | 拖放 / 光标 API（若需要） | 设计范围冻结后做 |
| C7 | 模态对话框骨架（Message 风格） | 阻塞输入 + Escape |

**不负责：** 主题 token 命名（归 S）；Host 生命周期（归 H）。

---

### 3.4 S · 样式与主题

**Wave 1**

| ID | 任务 | 完成判据 | 依赖 |
|----|------|----------|------|
| S1 | 布局硬编码色 → token 迁移策略（命名表） | 文档 + 1～2 页试点 | — |
| S2 | Style catalog 与 Theme 职责边界写清 | style-catalog.md 增补 | — |
| S3 | 启动日志打印已加载 styles/theme 路径 | 调试粘连 env 友好 | H 可代写 |

**Wave 2**

| ID | 任务 | 完成判据 |
|----|------|----------|
| S4 | **运行时换肤**（Reload theme + catalog，树 ApplyThemeDefaults） | 按钮切换 light/dark demo |
| S5 | 设计 token 版本 / 缺省回退策略 | 无 token 不崩 |
| S6 | 控件 DefaultStyle 全覆盖审计表 | 表中控件均有 theme 路径 |

---

### 3.5 H · 宿主与多窗

**Wave 1**

| ID | 任务 | 完成判据 | 依赖 |
|----|------|----------|------|
| H1 | 从 `main.cpp` 抽出 `UiHost` / `Application`（逻辑拆分，可仍单 TU） | 单测/手工多窗无回归 | — |
| H2 | `OpenHost(options)` 公开参数结构（layout/chrome/size/renderer） | 替代仅靠 env | — |
| H3 | 启动时可选打印/忽略粘连 `AI_WIN_UI_*`（`AI_WIN_UI_IGNORE_ENV=1`） | 文档 + 行为 | Q 验收 |
| H4 | 退出路径 COM 顺序回归（已有修复保持） | 双后端 10 次开关无 WER | R 协测 |

**Wave 2–3（L3）**

| ID | 任务 | 完成判据 |
|----|------|----------|
| H5 | **可嵌入 API**：`AiWinUi_CreateHost` / 消息泵可选外部 | 独立 sample 工程链入 |
| H6 | 静态库/动态库 CMake target `ai_win_ui_lib` | 示例 `samples/embed_host` |
| H7 | 版本宏 `AI_WIN_UI_VERSION` + CHANGELOG | 发版流程一页纸 |
| H8 | 多监视器 / DPI 变更黄金用例 | 文档 + 手工步骤 |

---

### 3.6 Q · 质量与工具

**Wave 1**

| ID | 任务 | 完成判据 | 依赖 |
|----|------|----------|------|
| Q1 | `demo_gallery` 与真实 layouts **同步检查清单**（缺卡即 fail 脚本可选） | 新 layout 必登记 | — |
| Q2 | golden：`core_validation` skia+d2d | CI/本地一键 | L4/R |
| Q3 | headless smoke 纳入 gallery 代表页 | full profile 绿 | — |
| Q4 | 手工验收表：gallery 每卡「打开/关子窗/无崩」 | `mvp-acceptance` 增节 | — |
| Q5 | 环境粘连：文档醒目提示 + 可选 `scripts/clear_ai_win_env.ps1` | 新人 0 次踩坑 | — |

**Wave 2**

| ID | 任务 | 完成判据 |
|----|------|----------|
| Q6 | 交互录制/回放（最小：键鼠脚本）或放弃并写明 | 决策 |
| Q7 | 像素/截图 diff（可选，后置） | 非阻断 L1 |
| Q8 | CI（GitHub Actions / 本地 agent）跑 smoke+golden | 主分支保护 |

---

## 4. Wave 时间片（串并行）

### Wave 1 — 「引擎可信收口」（约 2–4 周，视人力）

```
        ┌── R1 → R2 → R3 ──┐
        │                  ▼
Start ──┼── L1,L2,L3 ──► L4 ◄── Q2
        │                  │
        ├── C1,C2,C3       │
        ├── S1,S2,S3       │
        ├── H1,H2,H3,H4    │
        └── Q1,Q3,Q4,Q5 ───┴──► Wave1 Exit
```

**Wave 1 Exit（全员同意再进 Wave 2）：**

- [ ] 双后端 core 页文本无「明显错位/裁切」（R）  
- [ ] measure golden ≥2 布局 × 双后端（Q+L）  
- [ ] 多窗 OpenHost 参数化 + 粘连 env 可处理（H）  
- [ ] gallery 手工全绿（Q）  
- [ ] 无已知启动/退出 AV（H+R）

### Wave 2 — 「产品能力」

并行主线：

| 人物 | 主线 |
|------|------|
| C | Popup 统一 + 虚拟列表 |
| S | 运行时换肤 |
| R | 渐变/阴影/SVG tint |
| L | Grid 决策 + Scroll 深化 |
| H | 嵌入 API 草案 |
| Q | CI + golden 扩展 |

**Wave 2 Exit：** 1k 行列表可演示；换肤 demo；Popup 不漏点；嵌入 API 设计评审通过。

### Wave 3 — 「交付形态」

| 人物 | 主线 |
|------|------|
| H | `ai_win_ui_lib` + sample |
| Q | 发版检查单 + CHANGELOG |
| S/C | API 表面与样式约定冻结 |
| 全员 | 文档与 layout-spec 与代码一致 |

**Wave 3 Exit：** 外部工程可 `#include` + link 跑起一个 Host；版本号可查。

---

## 5. 认领表（填写用）

| 角色 | 负责人 | 备份 | 当前 Wave 焦点 |
|------|--------|------|----------------|
| R 渲染文本 | _待填_ | | |
| L 布局滚动 | _待填_ | | |
| C 控件交互 | _待填_ | | |
| S 样式主题 | _待填_ | | |
| H 宿主多窗 | _待填_ | | |
| Q 质量工具 | _待填_ | | |

**单人模式建议兼岗：**

| 人力 | 兼岗建议 |
|------|----------|
| 1 人 | Wave1：R → Q2 → H3 → C3；Wave2：C5 或 C4 二选一 |
| 2 人 | 甲：R+L+Q；乙：C+S+H |
| 3 人 | 甲 R+L；乙 C+S；丙 H+Q |
| 6 人 | 严格一角色一人，按 Wave Exit 对齐 |

---

## 6. 接口契约（跨人物）

### 6.1 事件 / Demo

- Gallery / 布局统一：`openDemo:path[|chrome][|size]`（已实现）  
- **Q** 增案例只改 `demo_gallery.xml` + 可选 alias；不改 Host 内核  

### 6.2 度量 / 测试

- 元素 `name`/`id` 稳定路径（已有）  
- golden 格式 NDJSON：`path,dw,dh,w,h`  
- **新增控件必须：** parser + gallery 卡 +（若影响布局）golden 更新说明  

### 6.3 样式

- `$color.*` = Theme 标量；`$style.*` = Catalog 整套外观  
- 换肤（S4）不得破坏 layout 结构，只刷 style/theme  

### 6.4 宿主

- 默认多窗 = 进程内；`AI_WIN_UI_CHILD_PROCESS=1` = 多进程  
- 嵌入后消息泵可外置（H5），控件代码禁止假设「全局只有一个 App」  

---

## 7. 与旧文档关系

| 文档 | 关系 |
|------|------|
| `2026-07-15-directui-p0-hardening.md` | L1/P0 细节；本文件是其上的产品化总图 |
| `mvp-execution-plan.md` | MVP 收尾；**以本文件 Wave1 为准扩展** |
| `mvp-roadmap.md` | 缺口列表；完成项应回写勾选 |
| `advanced-rendering-workplan.md` | 控件交付史；新产品能力以本文件 C/R 为准 |
| `layout-spec.md` / `window-chrome.md` | 规范；各角色改行为必须改对应文档 |

---

## 8. 本周可立即开工（不依赖认领填满）

1. **R1** 文本对照表（哪怕先手工）  
2. **Q5** `scripts/clear_ai_win_env.ps1` + README 醒目提示  
3. **Q2** golden 加 `core_validation`  
4. **H3** 启动打印有效 env（layout/chrome/renderer）到 Debug 输出  
5. **Q1** gallery 与 layouts 目录 diff 脚本  

---

## 9. 修订记录

| 日期 | 变更 |
|------|------|
| 2026-07-15 | 初版：六人物 + 三 Wave + 认领表 + 契约 |
