# Implementation Plan: Keyboard-matrix partials

## 目标 (Objective)
增强基础组件的键盘可访问性和导航交互能力。主要涉及：
1. **RadioGroup 箭头键导航**：在单选按钮组中通过方向键（上/下/左/右）切换选中状态，并自动移动焦点。
2. **DataTable 键盘取消选择**：非编辑模式下，按 `Esc` 键清除所有行选择状态。

## 具体修改步骤 (Steps)

1. **引入 UIElement 焦点请求能力**：
   - **`src/ui_context.h`**: 
     - 前置声明 `class UIElement;`。
     - 在 `UIContext` 结构体中新增 `std::function<void(UIElement*)> requestFocus;` 回调。
   - **`src/ui_host.cpp`**: 
     - 在 `UiHost::Initialize()` 中，绑定 `m_uiContext.requestFocus = [this](UIElement* e) { SetFocusedElement(e); };`，允许控件主动申请获得焦点。

2. **RadioButton 箭头切换支持**：
   - **`src/ui_inputs.h` (RadioButton::OnKeyDown)**:
     - 拦截 `VK_UP`, `VK_DOWN`, `VK_LEFT`, `VK_RIGHT`。
     - 从 `s_groups[m_group]` 静态字典中找到当前 `RadioButton`。
     - 按顺序进行前后移动计算（支持首尾循环 wrap-around）。
     - 调用 `SetChecked(true)` 选中目标按钮。
     - 使用 `m_context->requestFocus(nextBtn)` 转移键盘焦点。

3. **DataTable 按 Esc 清除选择**：
   - **`src/ui_panels.h` (DataTable::OnKeyDown)**:
     - 在 `switch (keyCode)` 中加入 `case VK_ESCAPE:` (不在 `m_editing` 分支内)。
     - 将 `m_selectedIndex` 设为 `-1`，并执行 `m_selectedSet.clear()`。
     - 调用 `NotifySelectionChanged()` 并返回 `true` 标记事件已处理。

## 验证 (Validation)
- 运行 `.\scripts\run_ci_local.ps1 -SkipBuild`，确保引入的焦点请求函数未破坏现有核心功能。
- 启动 Demo Gallery：
  - 测试 DataTable（`demo_data_table.xml`），选中行后按 Esc，确认高亮状态取消。
  - 测试 RadioButton（`demo_inputs.xml`），选中单选框后，使用方向键应当能在同组按键中循环切换且焦点同时跟随。
