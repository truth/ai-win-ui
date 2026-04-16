# 开发日志

## 2026-04-16

- 当前工作分支：`feat/resource-layout-mvp`
- 目标：完善资源驱动 UI 加载与文档支持
- 已完成：
  - 新增 `resource/layouts/ui.xml` 作为 XML 布局示例
  - 支持优先加载 `resource/layouts/ui.json`，并回退至 `ui.xml`
  - 修复 `LayoutParser` 中的资源加载和 JSON/XML 构建逻辑
  - 添加 `ZipResourceProvider`，优先读取 `assets.zip`，回退到 `resource/` 目录
  - 添加 `FallbackResourceProvider`，支持 ZIP 资源缺失时回退到 `resource/` 目录
  - 扩展通用布局属性：支持 `width`、`height`、`margin`
  - 扩展 `Panel` 布局能力：支持 `alignItems`、`justifyContent`
  - 完善鼠标状态处理：补充 `WM_MOUSELEAVE` 与控件级 hover/pressed 状态回收
  - 更新 JSON/XML 示例布局，覆盖新的布局属性与事件表现
  - 新增 `build.ps1` / `build.cmd` 编译脚本，统一 CMake 配置与构建入口
  - 在 CMake 中增加构建后资源复制，确保 `build/<Config>/ai_win_ui.exe` 可直接运行
  - 成功通过 `cmake --build build --config Debug`
  - 增强 `TextInput` 选择、Ctrl+A/C/V/X 剪贴板行为与光标定位
  - 增加 `TextInput` 拖拽选择支持，优化鼠标交互体验
  - 优化 `Panel`/`GridPanel` 测量流程，增加 `UIElement` 子元素测量结果稳定性
  - 在窗口大小变化时先执行根测量再 Arrange，提升自动尺寸计算稳定性
- 待办：
  - 继续补齐键盘、焦点与更多输入事件
