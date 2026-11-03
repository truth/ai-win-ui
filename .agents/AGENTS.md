# ai-win-ui 开发规范与工作流指南

## 标准开发工作流

在执行重构、功能开发、性能优化等任务时，必须严格遵守以下 6 步标准流程：

1. **分析规划**：明确总体优化路线图（Roadmap）。
2. **创建分支**：基于 `master` 签出独立功能分支（如 `feature/xxx` 或 `v2`）。
3. **实施计划**：撰写 `implementation_plan.md`，明确修改内容、技术方案与验证指标，并请求用户审核。
4. **代码执行**：按模块化原则编码，使用 `.\build.ps1` 校验编译。
5. **自动化测试**：运行 `.\scripts\run_ci_local.ps1 -SkipBuild`，确保 7 项 CI 回归测试 **100% PASS**。
6. **合并与归档**：
   - 合并分支至 `master`；
   - 更新 `DEVELOPMENT_LOG.md` 顶部开发日志；
   - 在 `doc/plan/` 增加带日期的设计归档文档；
   - 更新 `doc/README.md` 中的系统快照与文档索引。

详细流程参照 Skill 指南：`skills/standard-development-workflow/SKILL.md`。

---

## 核心架构与排版避坑守则 (Critical Pitfalls)

为了避免后续迭代中反复出现排版下溢、文字裁切、重绘闪烁等历史陷阱，开发时必须严格遵守以下守则（详见 `doc/troubleshooting-and-pitfalls.md`）：

1. **Thickness 顺序非 CSS 顺序**：
   - Windows/ai-win-ui 标准顺序为 **`Left, Top, Right, Bottom` (L, T, R, B)**。
   - 水平 10px、垂直 0px 必须写作 `10, 0, 10, 0` 或 `10, 0`，严禁写成 `0, 10, 0, 10`。
2. **紧凑行容器必须显式指定 padding="0"**：
   - `Panel` 默认具备 8px 内边距（1.25x DPI 下上下达 20px）。
   - 定义行高 `<= 24px` 的紧凑行容器（表格行、指标条）时，必须显式声明 `padding="0,0,0,0"`，防止内容高度塌陷为 2~3px 导致 Label 文字隐形。
3. **渲染器局部重绘边界**：
   - 局部脏矩形剪裁（`IntersectsDirtyRect`）仅允许在具备离屏持久 Surface 的后端（如 Skia）启用；
   - Direct2D（DXGI 交换链双缓冲）必须全帧提交，依靠组件内部的**几何点集缓存 (Geometry Cache)** 实现 144Hz 高刷性能，严禁跨帧局部跳过绘制导致底色黑屏。
4. **高频行情更新必须批量化**：
   - 批量更新数值文本时必须包裹在 `BeginBatchUpdate()` 与 `EndBatchUpdate(false)` 中，避免高频触发 Yoga 全树布局重排风暴。
5. **度量排障利器**：
   - 排版异常时，禁止盲猜尺寸，优先使用 `.\scripts\run_measure_dump.ps1` 转储 NDJSON 几何树，比对 `dh` 与 `h` 快速定位收缩根因。
