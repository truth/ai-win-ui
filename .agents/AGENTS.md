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
