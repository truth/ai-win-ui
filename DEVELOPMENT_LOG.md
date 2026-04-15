# 开发日志

## 2026-04-15

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
  - 成功通过 `cmake --build build --config Debug`
- 待办：
  - 增强文本测量与自动尺寸精度
  - 继续补齐键盘、焦点与更多输入事件
