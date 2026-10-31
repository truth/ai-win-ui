# 声明式组件嵌套复用系统 (Phase 1: 文件级 <Include> 组件引用) 设计计划

- **日期**：2026-10-31
- **分支目标**：`feature/component-include`
- **计划状态**：已评审确认 (Ready for Implementation)

---

## 1. 目标与背景 (Goals & Background)

在之前的版本中，`ai-win-ui` 支持跨文件的 `StyleCatalog`（样式目录）复用，但缺乏**组件树结构（DOM 树）层面的复用能力**。当开发者需要构建通用卡片、导航头、状态标签栏等具有固定子节点层次的功能组件时，只能在各个页面中重复复制粘贴 XML/JSON 片段。

本设计旨在引入**文件级组件引用与嵌套功能**（`<Include>`）：
1. 允许将独立的 UI 片段提取为独立的 `.xml` 或 `.json` 文件；
2. 在任意父级布局中通过 `<Include src="..." />` 声明式嵌入；
3. 支持父节点对被引用组件根节点的属性覆盖（如 `name`、尺寸、外边距等）；
4. 提供防循环引用、智能相对路径查找及优雅容错机制。

---

## 2. 核心架构与技术设计 (Architecture & Technical Design)

### 2.1 语法形式
- **XML 语法**：
  ```xml
  <Include src="components/user_card.xml" width="320" margin="8" name="userHeader" />
  ```
- **JSON 语法**：
  ```json
  {
    "type": "Include",
    "props": {
      "src": "components/user_card.json",
      "width": 320,
      "margin": 8,
      "name": "userHeader"
    }
  }
  ```

### 2.2 核心决策与边界处理
1. **格式对齐原则**：
   - 严格格式对齐：XML 只能引用 `.xml` 组件；JSON 只能引用 `.json` 组件。若扩展名不匹配，记录警告并返回占位提示。
2. **属性覆盖机制 (Attribute Override)**：
   - `<Include>` 节点自身不生成额外的包裹层，而是将 `<Include>` 上声明的所有属性合并覆盖至被引入组件的根节点上（例如覆盖根节点的 `name`、`width`、`height`、`margin` 等）。
3. **路径解析策略 (Path Resolution)**：
   - 维持当前的引用栈（Include Stack），获取当前正在解析的文件所在目录；
   - `src` 优先相对于当前文件的相对路径解析（如父文件为 `layouts/pages/home.xml`，`src="cards/user.xml"` 会先查找 `layouts/pages/cards/user.xml`）；
   - 若未找到，则回退为相对于 Resource Provider 根目录查找。
4. **循环引用与容错机制 (Circular Dependency & Error Handling)**：
   - 维护一个全局访问路径栈 `std::vector<std::string> g_includeStack`；
   - 每次引入前检查路径是否已在栈中：若存在即判定为循环引用（如 A $\rightarrow$ B $\rightarrow$ A）；
   - 当文件不存在或出现循环引用时，不抛异常崩溃，而是输出调试警告，并生成一个带红色边框的错误占位 Panel（标有 Error 提示），确保宿主界面健壮不闪退。

---

## 3. 实施步骤清单 (Implementation Roadmap)

1. **分支创建**：基于 `master` 签出 `feature/component-include`。
2. **解析器核心逻辑改造 (`src/layout_parser.cpp`, `src/layout_parser.h`)**：
   - 在解析上下文中维护文件调用栈 `g_includeStack`；
   - 实现路径归一化与相对父目录解析辅助函数；
   - 在 `LayoutParser::CreatePanelElementFromXml` 和 `CreateControlElementFromXml`（以及 JSON 对应函数）中拦截 `<Include>` 节点；
   - 加载目标文件、解析子树并将 `<Include>` 上的属性覆盖到生成组件的根节点。
3. **画廊用例与测试构建**：
   - 在 `resource/layouts/` 下创建可复用的组件片段（如 `layouts/components/reusable_card.xml` 和 `layouts/components/reusable_card.json`）；
   - 创建包含 `<Include>` 实例的测试用例 `resource/layouts/include_component_cases.xml`，并在画廊中增加入口；
4. **编译与 CI 验证**：
   - 运行 `.\build.ps1` 校验编译；
   - 运行 `.\scripts\run_ci_local.ps1 -SkipBuild -SmokeProfile full -Renderer both`，确保 7 项自动化回归测试 100% 通过。
5. **归档与合并**：
   - 合并分支至 `master`；
   - 更新 `DEVELOPMENT_LOG.md` 与 `doc/README.md`。

---

## 4. 后续规划 (Phase 2 Preview)
- 在 Phase 1 `<Include>` 落地后，开展 Phase 2：**组件模板定义与参数插值实例化（`<Template>` / `<UseTemplate>`）**，支持通过 `$prop.xxx` 传递动态插槽与内容。
