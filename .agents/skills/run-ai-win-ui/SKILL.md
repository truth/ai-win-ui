---
name: run-ai-win-ui
description: 启动并测试 ai-win-ui 应用程序，配置测试布局文件和环境变量。
---

# 启动与配置 AI Win UI 应用程序

本指南说明了如何使用内置脚本启动应用程序并加载指定的布局，该脚本会自动处理编译、资源同步和环境配置。

## 1. 准备布局文件 (Layout)
`ai-win-ui` 支持使用 XML 定义布局。测试布局文件应该放置在项目的 `resource/layouts/` 目录下。

> [!WARNING]
> 请注意 XML 的根节点必须是有效的 UI 控件（例如 `<Panel>` 或 `<ScrollViewer>`），不能是 `<Window>` 等不支持的标签，否则程序会因为解析失败而安静退出。

## 2. 使用启动脚本运行测试
项目提供了一个极其方便的启动脚本 `scripts\run_layout_demo.ps1`，它会自动执行以下操作：
- 检查源代码是否有更新并自动调用 `cmake --build` 重新编译
- 自动将 `resource/` 目录同步到生成的输出目录（如 `build\Debug\resource`）
- 自动设置 `AI_WIN_UI_LAYOUT` 和 `AI_WIN_UI_RENDERER` 等环境变量
- 解析布局路径的简写（例如直接传入 `drag_drop_cases`，它会自动找到 `resource/layouts/drag_drop_cases.xml`）
- 启动 `ai_win_ui.exe` 测试窗口

**如何运行：**
在 PowerShell 中执行以下命令启动特定布局的测试窗口（例如加载 `drag_drop_cases.xml`）：
```powershell
.\scripts\run_layout_demo.ps1 -Layout "drag_drop_cases"
```
你还可以通过参数配置渲染器：
```powershell
.\scripts\run_layout_demo.ps1 -Layout "drag_drop_cases" -Renderer "skia"
```

> [!NOTE]
> 如果直接启动失败，请检查布局文件是否存在语法错误导致解析失败。该脚本已经完美解决了资源未同步或者路径不对的问题。
