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
  - 增强 `Panel`/`Grid` 布局属性解析，支持 JSON/XML 别名与兼容属性
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

## 2026-04-22

- 继续推进 Skia 文本稳定性收尾。
- 调整 `src/skia_text_layout.cpp` 的 `blockHeight` 计算，改为按每一行预留完整 `lineHeight`，减少 Skia 文本测量与实际绘制在垂直居中、控件高度和裁剪边界上的偏差。
- 已重新通过 `cmake --build --preset build-dev-debug-skia-local-sdk`。

## 2026-04-27

- 重构 Skia 字体工具函数，消除代码重复：
  - 新增 `src/skia_font_shared.h`，提取 `TryMatchFamily`、`CreateDefaultTypeface`、`WideToUtf8`、`CreateDefaultFontManager`、`CreateSkiaFont` 为共享内联函数
  - 更新 `src/skia_text_layout.cpp` 和 `src/renderer_skia.cpp` 统一使用共享头文件
- 修复 `WrapWideLine` 连续空白导致的空行问题：
  - 每个换行段开始时跳过前导空白，避免产生空白行
  - 当修剪后段落为空时跳过该空白区域而非回退到单字符发射
- 改进 `blockHeight` 计算以更贴近 DirectWrite 段落布局：
  - 改为 `(行数 - 1) * lineHeight + textHeight`
  - 单行文本 `blockHeight = textHeight`（ascent + descent），对齐 DirectWrite 的 `DWRITE_TEXT_METRICS.height`
  - 多行文本保留完整行间距，但最后一行底部不需要额外 leading
- 修复 `BuildStackLayout` 中 flex 元素主轴尺寸被显式覆盖的问题：
  - Column 方向：`flexGrow/shrink/basis` 激活时不再设置 `height`（主轴），改由 Yoga 从 `flexBasis + flexGrow/flexShrink` 计算
  - Row 方向：`flexGrow/shrink/basis` 激活时不再设置 `width`（主轴），Yoga 自动分配剩余空间
  - 非 flex 元素保持原有显式尺寸设定
  - 交叉轴尺寸（Column 的 width、Row 的 height）不受影响，仍按 stretch/measured 设定
- 构建验证通过：`cmake --build build --config Debug` 0 error

## 2026-04-24（响应式宽度收尾）

- 问题：Row 方向多个固定 `width` 的子节点总宽度相加超过父容器宽度时溢出右边界，窗口缩窄无响应式表现。根因：Yoga 节点默认 `flexShrink=0`，且 `UIElement::m_flexShrink` 默认也是 0，与 CSS flexbox 默认 1 相反。
- 修复：`src/layout_engine.cpp` 在 `BuildStackLayout` 为 Row 方向的子节点注入 **Yoga 层** 的 `flexShrink=1` 兜底。
  - 触发条件：`direction == Row && flexShrink == 0 && flexGrow == 0 && !hasFlexBasis`
  - 只动 Yoga 节点属性，不改 `UIElement::m_flexShrink` 默认值，避免级联影响 `GetPreferredWidth` 等 intrinsic 测量路径
  - 显式写了 `flexShrink`、`flexGrow`、`flex` 或 `flexBasis` 的节点不受影响
  - 窗口足够宽时 Yoga 不压缩（因 totalBasis ≤ containerWidth），不够时按比例压缩
- 副作用说明：
  - 不想被压缩的固定宽元素需在 XML 显式写 `flexShrink="0"`
  - `Spacer` 因带 `flexBasis="0"` 命中 `hasFlexBasis` 分支，行为不变
- 保留未动：
  - `MeasureLeafNode` 的 Undefined 模式不夹紧——兜底 shrink 已覆盖该场景下的溢出
  - Row fallback (`Panel::ArrangeRowFallback`) 是死代码路径（Yoga 引擎强制注入），不改
  - Column 方向不存在同类叠加溢出问题，不改
- 构建验证通过：`cmake --build build --config Debug` 0 error
- 待验证（需要人工跑）：
  - `yoga_measure_cases.xml` 窗口缩到 500px：三张 180 宽 Card 应同比压缩
  - `core_validation.xml` Section 2 Row：`flexGrow` 分配行为保持正确
  - 窗口拉回宽时固定宽元素恢复原尺寸
- 待办：完善 MVP 验收流程文档化

## 2026-04-25（浅色主题还原 & Panel 背景语义）

- 问题：按 `resource/dashboard.jpg`（浅色主题设计稿）还原 dashboard 布局时，程序渲染出大面积黑块，白卡被深色 Row 遮盖，所有卡片带 `#333333` 硬灰边框。
- 根因：
  - `Panel::background` 默认 `#1E1E1E`（近黑），且 `Panel::Render` 无条件 `FillRect`——没显式写 `background=` 的 Panel 全被填成深色
  - `Panel::Render` 还无条件 `DrawRoundedRect` 画 `#333333` 1px 边框——浅色背景上非常突兀
- 修复（`src/ui.h`）：
  - `Panel::background` 默认改为 `alpha=0`（透明）
  - `Panel::Render` 跳过 `alpha<=0` 的背景和边框绘制
  - 移除无条件的 `#333333` 1px 边框（未来需要可通过显式 `borderColor` 属性补回）
- 修复（`src/layout_parser.cpp::ParseHexColor`）：
  - 支持 `background="transparent"` / `"none"` → `alpha=0`
  - 支持 `#RRGGBBAA` 8 位十六进制色
  - 新增 `<algorithm>` / `<cctype>` 头文件 include
- 兼容性论证：grep 过全部 11 个现有 XML，`Panel` 没有任何一个依赖"不写 background 就显示 #1E1E1E"——多数都显式写了 background，少数省略的场景本来就期望透出父色。改默认值安全。
- 新增布局：`resource/layouts/dashboard_responsive.xml`
  - 按 `dashboard.jpg` 还原的响应式版本（不动原 `dashboard_reference.xml`）
  - 所有固定 `width="xxx"` 替换为 `flexGrow + flexBasis + minWidth` 组合，侧栏保留 60 固定宽
  - 窗口 800-1600px 范围内能正确同比压缩/伸展
- 构建验证通过：
  - `cmake --build build --config Debug` 0 error
  - `cmake --build --preset build-dev-debug-skia-local-sdk` 0 error
- 已知未还原项（待后续补齐）：
  - 任务**填充**进度条（目前是空白背景条，设计稿是 90%/50%/10% 紫色填充）
  - Stats 卡**圆形进度环**（目前是方块 + 数字）
  - 小圆头像（Today tasks 旁的 avatar group）
  - 虚线边框（"+" 加号卡片）
  - 卡片阴影（引擎不支持 DropShadow）
  - 胶囊标签（"Motion design" / "Logo" 等小 Tag）
  - 插图资源主题不对（Go premium 误用大脑图，4 功能卡插图与设计稿不一致）
