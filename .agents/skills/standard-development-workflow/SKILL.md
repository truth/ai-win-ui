---
name: standard-development-workflow
description: ai-win-ui 标准任务开发工作流，包含需求分析、分支创建、实施计划、代码执行、CI测试验证、Master合并与文档归档更新规范。
---

# ai-win-ui 标准任务开发与归档工作流程

本指南定义了在 `ai-win-ui` 项目中处理重构、新功能开发或优化任务时的**标准 6 步工作流闭环**。

---

## 阶段 1：分析与整体规划 (Architecture & Roadmap Plan)

1. 审查全局架构，在对话或 `brain` 空间梳理总体优化演进路线（如 7 大阶段 Phase 1 ~ Phase 7）。
2. 向用户汇报全局方向，并确定本次任务的目标阶段。

---

## 阶段 2：创建独立功能分支 (Branch Management)

1. 基于 `master` 最新代码创建独立的功能分支（例如 `feature/xxx` 或 `v2` / `v3`）：
   ```powershell
   git checkout -b v2
   ```
2. 确保在独立分支上进行所有更改，避免破坏 `master` 的稳定性。

---

## 阶段 3：撰写实施计划 (Implementation Plan)

1. 在规划模式下建立 `implementation_plan.md`，明确：
   - **更改文件清单**：按 Component 标明 `[MODIFY]`、`[NEW]`、`[DELETE]`。
   - **技术方案**：描述解耦、缓存、算法或接口的落地方式。
   - **验证方案**：指定测试命令。
2. 设置 `user_facing = true`, `request_feedback = true` 提交给用户审核，获得批准后进入执行。

---

## 阶段 4：增量编码与本地构建 (Execution & Build)

1. **原则**：保持代码模块化、高可读性、保持头文件与实现分离，遵从项目的 C++20 与命名规范。
2. **构建验证**：每次修改完成后，调用 `.\build.ps1` 校验 MSVC CMake 编译无 Warning/Error：
   ```powershell
   powershell -ExecutionPolicy Bypass -File .\build.ps1
   ```

---

## 阶段 5：自动化 CI 全套回归测试 (CI Verification)

1. 运行本地 PowerShell 自动化 CI 验证脚本：
   ```powershell
   powershell -ExecutionPolicy Bypass -File .\scripts\run_ci_local.ps1 -SkipBuild
   ```
2. **退出标准 (Exit Criteria)**：全套 7 项自动化测试（包含了 Gallery 覆盖率、Skia/Direct2D 无头 Smoke 测试、Core/Yoga/ScrollViewer Golden 测量）必须实现 **100% PASS (Passed: 7, Failed: 0)**。

---

## 阶段 6：提交、合并与文档归档更新 (Commit, Merge & Doc Update)

1. **Git 提交与分支合并**：
   - 在功能分支上提交变动：
     ```powershell
     git add .
     git commit -m "feat/perf/refactor: <descriptive message>"
     ```
   - 切换到 `master` 并使用 Fast-forward 进行合并：
     ```powershell
     git checkout master
     git merge v2
     ```
2. **同步更新项目正式文档**：
   - **`DEVELOPMENT_LOG.md`**：在顶部新增日期条目（如 `## YYYY-MM-DD`），记录本次 Wave/Phase 的核心更改。
   - **`doc/plan/YYYY-MM-DD-<name>-v1.md`**：在 `doc/plan/` 中创建带日期的正式设计与实现归档文件。
   - **`doc/README.md`**：更新 `doc/plan/` 文件索引与系统快照表 (Current System Snapshot)。
   - **`walkthrough.md`**：在 `brain` 空间提供简明的完成情况总结。
3. **提交文档更新**：
   ```powershell
   git add .
   git commit -m "docs: update DEVELOPMENT_LOG, doc/README, and add design document"
   ```
