# Medlio Medical Dashboard 高精度 UI 对齐设计与技术归档

**日期**: 2026-11-01  
**状态**: Approved & Implemented  
**分支**: `feature/medlio-dashboard`  

---

## 1. 背景与对齐挑战分析

在用户测试布局 `layouts/medlio_dashboard.xml` 时，指出“这种大 UI，对齐还原度比较低”。经过对设计图（`media_1793460819648.png`）的像素级比对与反思，确定了导致还原度偏低的四大核心原因：

1. **容器与侧边栏架构层级颠倒**：
   - *旧方案*：将最外层做成一个白底圆角大卡片，并把侧边栏作为白底列放置在大卡片内部；
   - *原图真实现象*：外层大卡片是一个宝蓝色（`#2764FF`）的大容器底板，左侧的侧边栏完全直接承载于深蓝底板之上，导航文字为淡蓝白字，激活项为纯白底蓝字药丸胶囊；右侧主工作区才是一个嵌入的大型浅灰白色悬浮工作卡片（`#F6F8FB`，圆角 20px）。
2. **材质与卡片层次感（Visual Elevation）缺失**：
   - 主内容区底色与内部卡片未区分。原图底色为温和浅灰 `#F6F8FB`，其上承载的 4 个 KPI 卡片、每日预约柱状图、月收入面积图、诊断化验面板、预约表格均为**纯白底 `#FFFFFF` + 极细浅灰微边框 `#EAEFF5`**，层次极其鲜明。
3. **图表与微细节（Micro Details）未到位**：
   - 图表控件之前保留了默认的图例和边框，干扰了紧凑排版；
   - 日预约图表需高亮 Wed 突出的 395 预约量柱体，并隐去图例以贴合原图；
   - 收入面积图带有波浪填充与 `+12.8%` 的绿色增长药丸徽章。
4. **组件化未充分利用**：
   - 之前部分卡片代码内联冗长；现全面使用 `<Include src="..." />` 将 4 个 KPI 卡片与表格行抽象为独立子组件。

---

## 2. 系统重构方案

### 2.1 主架构重构
- **外层画布**：`<Panel direction="row" background="#2764FF" padding="20,18,20,18" alignItems="stretch">`
- **左侧深蓝 Sidebar**：
  - 纯宝蓝背景 `#2764FF`，白字 Logo `Medlio`（粗体 24px）；
  - 激活项：纯白药丸胶囊 `<Panel background="#FFFFFF" cornerRadius="10">` + 蓝字 `Dashboard`；
  - 未激活项：文字与图标采用 `#BACFFF` 与 `#D8E5FF`；
  - 底部：半透明分割线 `#3B72FF` + Settings / Support。
- **右侧嵌入式工作台卡片**：
  - `<Panel direction="column" background="#F6F8FB" cornerRadius="20" flexGrow="1">`，内嵌 `<ScrollViewer>`。

### 2.2 模块化子组件清单
1. `resource/layouts/components/kpi_stat_card.xml`：Total Appointments（白底、小蓝图标、28px 加粗数字）；
2. `resource/layouts/components/kpi_stat_card_revenue.xml`：Revenue（白底、小蓝图标、4280 加粗数字）；
3. `resource/layouts/components/kpi_stat_card_referrals.xml`：Referrals（白底、小蓝图标、9 加粗数字）；
4. `resource/layouts/components/appointment_row.xml`：预约记录行（Adesh Shetty，Checked_in 绿色药丸，Start Session / EMR 按钮）；
5. `resource/layouts/components/appointment_row_scheduled.xml`：预约记录行（Ansh Singh，Scheduled 浅蓝灰药丸）。

---

## 3. 测试与验证结果

- **构建编译**：`.\build.ps1` 0 警告 0 报错；
- **自动化测试**：`.\scripts\run_ci_local.ps1 -SkipBuild`，7 项自动化回归测试 100% PASS（耗时 65.9s）：
  1. build: PASS (skipped)
  2. gallery coverage: PASS
  3. headless smoke (skia / smoke): PASS
  4. headless smoke (direct2d / smoke): PASS
  5. measure golden (core-validation / both): PASS
  6. measure golden (yoga-measure / both): PASS
  7. measure golden (scroll-viewer / both): PASS
- **运行时无头 Smoke 验证**：
  `.\scripts\run_layout_demo.ps1 -Layout "medlio_dashboard" -Renderer "skia" -QuitAfterMs 1500` 正常退出（exit code 0）。
