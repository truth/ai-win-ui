# 开发日志

## 2026-04-15

- 当前工作分支：`feat/resource-layout-mvp`
- 目标：完善资源驱动 UI 加载与文档支持
- 已完成：
  - 新增 `resource/layouts/ui.xml` 作为 XML 布局示例
  - 支持优先加载 `resource/layouts/ui.json`，并回退至 `ui.xml`
  - 修复 `LayoutParser` 中的资源加载和 JSON/XML 构建逻辑
  - 成功通过 `cmake --build build --config Debug`
- 待办：
  - 完善根目录 `README.md`
  - 添加资源打包（ZIP）加载支持
  - 扩展布局属性与事件处理能力
