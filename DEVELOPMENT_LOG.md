# 开发日志

## 2026-07-15

- 文档：样式/布局规则同步更新
  - 重写 `doc/layout-spec.md`（XML vs JSON、`$style` 三种写法、嵌套 style、ShapePanel、chrome/events）
  - 新增 `doc/style-catalog.md`；更新 `doc/window-chrome.md`、`doc/README.md`、根 `README.md`、`doc/resource-packaging.md`
  - 修正 `style_catalog_demo.json`：`events.onClick`（props 内 onClick 无效）
- 跨文件样式目录 + 异形 layered 多窗口（选项 3）：
  - `StyleCatalog`：`resource/styles/*.json`，`import[]`，命名样式；布局 `style="$style.name"` / `styleRef` / `extend`
  - 启动加载 `styles/default.json`（可 `AI_WIN_UI_STYLES` 追加）；Panel 绘制应用 catalog decoration
  - `IRenderer::FillPolygon`（D2D + Skia）；`ShapePanel`（heart / petal / oval / star）
  - Hub 按钮 `CreateProcess` 打开独立 layered 异形窗（心形/花瓣/椭圆/星形）；`AI_WIN_UI_SIZE=WxH`
  - Demo：`style-catalog`、`shaped-hub`；脚本 `run_shaped_windows_demo.ps1`
- StatCard / SparklineChart 主题化 DefaultStyle（shell/accent/line tokens）；布局显式色保留
- 高级输入主题化：`NumericUpDown` / `DateTimePicker` / `RichTextBox` DefaultStyle + ApplyThemeDefaults；DataTable 壳层 token
- ListView/TreeView：`PaintPillScrollbars` 两端圆角 XY 滚动条；滚轮优先控件（Shift+滚轮水平）
- navigation：右栏 `wrap` 自动换行；MSVC C4819 修复（`/utf-8` + `/wd4819` + UTF-8 BOM）
- 验证布局改为左右结构 + 压缩高度（避免底部被挤出视口）：
  - `navigation_components`：左 shell/ContextMenu，右 ListView|TreeView，取消 wrap
  - `core_controls_v2` / `advanced_inputs` / `table_components` / `stats_components` / `seagull_animation` 同步紧凑左右布局
  - 控件默认更矮：ListView/TreeView 测量高度、Menu/Tool stripHeight、ListBox itemHeight 等
- 壳条主题化 + DataTable 产品增强 + 无头回归：
  - MenuStrip / ToolStrip / StatusStrip / ContextMenu：`DefaultStyle(theme)` + `itemStyle` + `ApplyThemeDefaults`
  - DataTable：`OnSelectionChanged` / `OnCellChanged` 回调；`frozenColumnCount` 冻结列；Ctrl+Left/Right 水平平移
  - dashboard_responsive：任务进度填充条、胶囊 Tag、头像组、环形风格 stat 圆点
  - `AI_WIN_UI_QUIT_AFTER_MS` + `scripts/run_headless_smoke.ps1` 自动加载布局并退出
- ListView / TreeView 接入 `itemStyle` + `DefaultStyle(theme)` / `ApplyThemeDefaults`
  - 行 hover/selected 走嵌套样式；壳层 decoration 同步背景/边框/圆角
  - 布局 `style.itemStyle` 仍由通用 `ApplyCommonXmlAttributes` / JSON `style` 解析
- 嵌套样式 + 主题 DefaultStyle + DataTable v3：
  - `ComponentStyle.itemStyle` / `tabStyle` / `dropdownStyle`（布局解析 + 深拷贝）
  - ListBox 行绘制走 `itemStyle`；ComboBox 下拉/行；TabControl tab 条走 `tabStyle`
  - `DefaultStyle(const Theme*)` + `ApplyThemeDefaults()`：Button / TextInput / Checkbox / Radio / Slider / Progress / List / Combo / Tab
  - DataTable v3：`multiSelect`（Ctrl 点选 / Shift 范围）、`editable`（F2 / 双击单元格）、`resizableColumns`（列边拖拽）
  - 校验：`table_components.xml` / `.json`
- ProgressBar 接入 `ComponentStyle`（`track` / `fill`，hover/pressed fill）；`fillColor`/`trackColor` 同步样式
- ComboBox：布局高度仅标题行；下拉为 `RenderOverlay` 浮层；`FindOverlayHitAt` 优先命中
- ComboBox：底部空间不足时列表向上展开（`viewportHeight` 写入 `UIContext`）
- advanced_inputs：左 Numeric/DateTime，右 RichTextBox（取消 wrap，避免被挤到窗口底）
- core_controls_v2：左栏 Progress/List/Combo，右栏 TabControl（取消 wrap，始终并排）
- 修复 TabControl 不显示：
  - `IsLayoutLeaf()` 改为 virtual；TabControl 返回 true，Yoga 按叶子测量并尊重 `height`
  - 切换 tab 时 `LayoutTabPages()` 重新 Measure/Arrange 当前页（否则页面停在屏外 -10000）
  - 高度自适应内容；demo 卡片 `alignItems=start`
- Slider 接入 `ComponentStyle`：`StyleSpec.track` / `thumb` / `fill` 子装饰
  - hover/pressed thumb 态；进度填充；`highlightColor` 属性同步 fill/thumb
  - 解析支持 `style.track|thumb|fill` 与 `style.decoration.track|thumb|fill`
- DataTable v2：行选择、表头排序（数字感知）、hover、选中滚动可见
  - 属性：`selectable` / `sortable` / `selectedIndex` / `selectedRowBackground` / `rowHoverBackground`
  - 校验页：`table_components.xml` / `.json` 扩充更多行与静态对照表
- `run_layout_demo.ps1`：源码新于 exe 时自动重建（修复旧 Debug 二进制导致新控件无响应）
- Phase 8 完成：高级输入控件
  - `NumericUpDown`：`label` / `min` / `max` / `value` / `step` / `decimalPlaces`；chevron + 方向键
  - `DateTimePicker`：`mode=date|time|dateTime`、`value`、`showSeconds`；分段高亮、左右切字段、闰年/月末钳制
  - `RichTextBox`：多行编辑、Ctrl+B/I、加载时 `**bold**` / `*italic*`；`TextRenderOptions.bold/italic` 贯通 D2D/Skia
  - 校验页：`resource/layouts/advanced_inputs.xml` + `.json`；别名 `advanced-inputs`
  - 文档：`doc/advanced-rendering-workplan.md` Phase 8 进度；chrome 过期说明已修正
- 分支 `feature/skia-layered-multimon-perf`：
  - Skia layered present（DIB 零拷贝 + `UpdateLayeredWindow` / HWND `BitBlt`）
  - 多显示器：光标所在显示器居中；`WM_DISPLAYCHANGE` 刷新；DPI 跨屏沿用 `WM_DPICHANGED`
  - 性能：同尺寸跳过 resize、最小化跳过 present、脏帧门控
  - layered 不再强制 Direct2D；脚本默认 Skia
- Layered 真异形 v2：`PresentMode::Layered`（Direct2D DC RT + DIB + `UpdateLayeredWindow`）
  - `AI_WIN_UI_CHROME=layered` / 根布局 `chrome="layered"`
  - 透明 clear + 圆角卡片 demo：`layouts/layered_chrome_demo.xml`
  - 透明像素 `HTTRANSPARENT`（缩放边框仍可用）
  - 软阴影、最大化时收起浮层 padding/圆角
  - 计划：`doc/plan/2026-07-15-layered-irregular-window-v2.md`
- 分支：`feature/custom-window-chrome-v1`（已 push 到 origin）
- 落地自定义窗口 chrome（计划见 `doc/plan/2026-07-15-custom-chrome-irregular-window-v1.md`）
- 新增 `src/window_chrome.*`：`WM_NCCALCSIZE` / `WM_NCHITTEST` / `WM_GETMINMAXINFO`、DPI 边框、DWM 圆角
- `main.cpp`：opt-in custom 样式、命中区域刷新、min/max/close 事件
- 布局属性：`chrome`、`hitTest`、`role=titleBar`（XML/JSON common props）
- Caption 按钮几何图标 + 关闭红色 hover；最大化切换还原图标；custom 窗口居中启动
- 标题栏右上角固定窗口按钮；custom 模式绘制 1px 外框
- Demo：`resource/layouts/custom_chrome_demo.xml` + `scripts/run_custom_chrome_demo.ps1`
- 文档：`doc/window-chrome.md`、README 快速入口
- 默认仍为系统标题栏；`AI_WIN_UI_CHROME=custom` 或根布局 `chrome="custom"` 启用
- 附带修复：Skia Release 链接按配置收集 `.lib`（避免 Debug 独有的 `spvtools` 在 Release 失败）
- 验证：`.\build.ps1 -Configuration Release` 通过

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
