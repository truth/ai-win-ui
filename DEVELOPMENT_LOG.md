# 开发日志

## 2026-11-03

- **架构避坑与排错最佳实践知识库沉淀 (Troubleshooting & Pitfall Guide)**
  - 创建正式排坑手册 [`doc/troubleshooting-and-pitfalls.md`](doc/troubleshooting-and-pitfalls.md)，总结 6 大核心陷阱与避坑准则：
    1. Thickness 顺序陷阱（L, T, R, B vs CSS T, R, B, L）；
    2. 容器默认内边距与紧凑单行高度塌陷机制（Padding vs Row Height）；
    3. Direct2D 双缓冲误清屏与局部脏矩形剪裁边界（D2D 全帧 + 几何缓存 vs Skia 局部重绘）；
    4. 高频行情更新与 Yoga 全树重排风暴（Batch Update 机制）；
    5. DPI 动态缩放排障工具（`run_measure_dump.ps1` NDJSON 转储）；
    6. Windows GUI 进程生命周期与文件锁处理。
  - 同步更新 [`.agents/AGENTS.md`](.agents/AGENTS.md) 核心规约与 [`doc/README.md`](doc/README.md) 主文档索引。

- **列表文字高度塌陷修复、Panel 默认内边距安全保护与通用解析扩展 (List Label Height Collapse & Panel Padding Safety Fix)**（分支 `feature/fix-list-labels-height-collapse`）
  - **根本原因排查与定位**：
    - 排查发现 C++ `Panel` 默认赋予 `padding{8, 8, 8, 8}`（上下内边距各 8px，在 1.25x DPI 缩放下为 10px+10px=20px）；
    - 右侧列表中的 28 个行容器（`<Panel direction="row" height="19" alignItems="center">` 及 `height="18"`）在 XML 中均未显式声明 padding，导致 19px 高度（缩放后 23.75px）扣除 20px 默认内边距后，内部可用内容高度被挤压至 3.75px；
    - Yoga 布局引擎在测量阶段执行 `AtMost` 约束，将子元素 `<Label>` 高度硬截断为 `h = 3.0px`，导致 DirectWrite / Skia 无法在 3px 矩形内绘制 15px 高度的文字，产生“文字隐形/完全不显示”的严重 bug。
  - **XML 紧凑行显式内边距定义 (`resource/layouts/stock_quotes.xml`)**：
    - 为盘口指标双列 Grid（10 行）、个股贡献度列表（8 行）、短线精灵推流列表（10 行）共 28 处行容器显式添加 `padding="0,0,0,0"`；
    - 使行内容可用高度达满高 23px，Label 拥有充裕排版空间垂直居中呈现。
  - **C++ 解析器安全兜底与 Thickness 扩展 (`src/layout_parser.cpp`, `src/ui_element.h`)**：
    - 在 XML 和 JSON Panel 解析中增加安全兜底机制：当 `<Panel>` 设置固定高度且 `height <= (padding.top + padding.bottom)` 或 `height <= 24.0f` 时，若未显式指定 padding，自动将内边距置 0，彻底根除类似单行紧凑容器由于默认 padding 挤压导致内容高度下溢的隐患；
    - 放宽 `padding` 与 `margin` 的多参数解析限制，完整支持 1、2、4 个数值语法；
    - 在 `UIElement` 中规范化补充 `FixedWidth()` 和 `FixedHeight()` getter。
  - **度量与 CI 测试验证**：
    - `run_measure_dump.ps1` 转储验证表明，所有列表内 Label 的排版高度由 3.0px 恢复为满高 23.0px；
    - `build.ps1` 构建通过，`run_ci_local.ps1 -SkipBuild` 7/7 项回归测试 **100% PASS**。

- **Direct2D 脏矩形误清屏修复、Thickness 解析增强与同花顺布局内边距校正 (Font Clipping & Direct2D Partial Repaint Fix)**（分支 `feature/fix-font-clipping-and-panel-display`）
  - **Direct2D 脏矩形防清屏保护 (`src/ui_host.cpp`)**：
    - 修复 Direct2D（默认后端）在双缓冲交换链下收到局部脏矩形时，无条件调用 `Clear()` 清屏但跳过非脏区元素（`IntersectsDirtyRect`），导致顶部菜单、右侧盘口指标、贡献度列表大面积黑屏与边缘裁切的致命缺陷；
    - 仅在 Skia 后端（具备离屏持久 Surface）启用局部脏矩形，Direct2D 维持完整帧重绘；结合图表几何缓存，单帧绘制保持在 0.15ms 以内，兼顾完整呈现与 144Hz 丝滑。
  - **Thickness 解析器多参数支持 (`src/layout_parser.cpp`)**：
    - 增强 `ParseThickness`，支持 1 个值（四边等距）、2 个值（水平, 垂直）、4 个值（Left, Top, Right, Bottom），防止单值或双值输入时解析为全零。
  - **同花顺布局 XML 内边距与高度校正 (`resource/layouts/stock_quotes.xml`)**：
    - 纠正所有面板中错将 CSS `0, X, 0, X` 当作水平内边距的写法，统一修正为 Windows Thickness 标准顺序 `X, 0, X, 0`；
    - 消除菜单栏、工具条、状态栏因上下 20px 巨大内边距造成的字体被挤压裁切只显半截的问题；
    - 盘口 10 项指标、8 支权重股贡献度榜单、短线精灵与标的现价全部清晰完整居中显示。
  - **验证**：`build.ps1` 编译 0 Warning / 0 Error，`run_ci_local.ps1 -SkipBuild` 7/7 项回归测试 100% PASS。

- **同花顺 K 线操盘工作站全景、数据饱满度与 144Hz 极速丝滑度重构 (Stock Kline Workstation & Silky Smoothness Optimization)**（分支 `feature/stock-kline-workstation`）
  - **坐标几何缓存（Geometry Caching）与十字光标 144Hz 丝滑交互 (`src/ui_financial_chart.h`)**：
    - 引入 `m_geometryDirty` 预计算与坐标几何缓存（`m_cachedWhitePts`, `m_cachedCandles`, `m_cachedWicks`, `m_cachedMa5/10/20Pts`, `m_cachedVolMa5/10Pts` 等）；
    - 鼠标在图表上悬停/移动时，直接复用已排版好的几何缓存点集，跳过 240+ 根蜡烛柱、影线、均线坐标重新计算与大量浮点运算，单帧渲染耗时由 4~8ms 暴降至 0.05ms 以内，光标移动达到高刷显示器 144Hz 极致丝滑；
    - 移动十字光标时调用 `InvalidateSelf(0.0f)` 仅对图表矩形区域执行局部重绘。
  - **宿主局部脏矩形提交与鼠标事件穿透优化 (`src/ui_host.cpp`, `include/ai_win_ui/host.h`)**：
    - 修复 `OnMouseMove` 在子控件处理鼠标事件后无条件调用 `InvalidateRect(m_hwnd, nullptr, FALSE)` 导致整屏 1280x820 强制全屏重绘的严重卡顿瓶颈；
    - 改为精确合并 `FlushPendingDirtyInvalidate()`，结合各控件自身的局部脏矩形，使 Direct2D / Skia 双后端只对脏区重绘。
  - **盘口数据批量更新机制 (`BeginBatchUpdate` / `EndBatchUpdate`)**：
    - 针对 500ms 定时器刷新 11+ 个盘口行情 Label，原有逻辑每次 `SetElementText` 均会触发 Yoga `UpdateLayout()` 全树重排与全屏重绘（11 次重复触发）；
    - 引入 `BeginBatchUpdate()` 与 `EndBatchUpdate(bool requestLayout = false)`，批量文本更新期间抑制 Yoga 全树布局重排，仅提交文本区域的局部脏矩形重绘，彻底消除行情跳动时的全屏微卡顿。
  - **同花顺视觉全景对齐与全量 Mock 数据**：
    - 阴线支持同花顺经典电青色（Cyan #00D2D2），阳线亮红（#EE2222）；
    - 均线配色严格对齐：MA5（绿 #00E500）、MA10（紫 #E040FB）、MA20（黄 #FFD700）；
    - 副图 Volume 增加 VolMA5（品红）与 VolMA10（金黄）均量线；
    - 顶部绘制同花顺标志性胶囊标签 日线 ▼（5: 3.02, 10: 3.06, 20: 3.13）与 Vol ▼；
    - 扩充全部 20 项大盘指标与 14 项个股深度属性、16 支权重股贡献度榜单、16+ 条短线精灵逐笔推流。
  - **验证**：Release 版本编译并流畅运行，`run_ci_local.ps1 -SkipBuild` 7/7 项回归测试 100% PASS。


## 2026-11-02

- **同花顺终端 UI 对齐修复、全量 Mock 数据与 K 线图（Candlestick）功能 (FinancialChart K-Line & Layout Align)**（分支 `feature/kline-and-layout-align`）
  - **FinancialChart K 线模式支持 (`src/ui_financial_chart.h`)**：
    - 扩展支持 `FinancialChartMode::Candlestick` 与 `FinancialChartMode::TimeSharing` 双模；
    - 实现蜡烛实体柱（阳线红实心、阴线绿实心）、上下影线、MA5（白）/ MA10（黄）/ MA20（紫）三条均线平滑折线绘制；
    - 副图成交量与量均线适配，K 线十字光标与 OHLCV 详情信息 Header 追踪；
  - **模拟行情与全量 Mock 数据扩充 (src/mock_market_engine.h/cpp)**：
    - `GenerateCandles(90)` 生成 90 根日 K 线模拟行情，计算真实 MA5/MA10/MA20 移动平均线；
    - 盘口核心指标全量补充（委比、委差、委买量、委卖量、外盘、内盘、市盈(动)、市净率、换手率、两市成交额、上涨/平盘/下跌家数）；
    - 扩充 16 支权重股贡献度榜单与 16+ 条短线精灵逐笔推流；
  - **布局重构与字体截断修复 (`resource/layouts/stock_quotes.xml`)**：
    - 顶部菜单行高提升至 28px，字号 12px，消除 DPI 缩放下的文本压扁与裁切；
    - 盘口网格行高调整为安全的 19px，个股贡献度与短线精灵列表行高 18px，填满界面，彻底杜绝黑屏留白；
    - 周期栏与左侧 Dock 接入分时、日K、周K、月K一键无缝切换动作；
  - **独立终端应用与 Host API (samples/stock_terminal/main.cpp, include/ai_win_ui/host.h, src/ui_host.cpp)**：
    - `Host` 暴露 `SetFinancialChartMode`、`ClearFinancialChartCandles`、`AppendFinancialChartCandle`；
    - 响应周期切换 action，实现分时走势与多周期 K 线图无缝交互；
  - **验证**：`build.ps1` 编译 0 Error / 0 Warning，`run_ci_local.ps1 -SkipBuild` 7/7 项回归测试 100% PASS。

- **同花顺风格金融看盘终端与 FinancialChart 复合控件 (Flush Stock Terminal & FinancialChart)**（分支 `feature/stock-terminal`）
  - **金融复合图表控件 (`src/ui_financial_chart.h`, `src/ui.h`)**：
    - 继承自 `ai_win_ui::UIElement`，注册 `<FinancialChart>` XML/JSON 标签；
    - 内部划分 `PricePane`（58% 分时主图走势，含加权白线、领先黄线、昨收虚线中轴与网格分割线）、`VolumePane`（20% 红绿成交量柱）、`IndicatorPane`（18% 大盘涨跌指标线）；
    - 实现双 Y 轴自适应价格算法（以昨收基准为中心对称映射振幅与百分比）及时间 X 轴换算；
    - 实现十字准星 Crosshair 追踪与 Tooltip 浮动数据标签（左轴价格、右轴涨跌幅、底轴时间及顶部状态汇总）；
  - **模拟行情推流引擎与键盘精灵 (`src/mock_market_engine.h/cpp`, `src/keyboard_genie.h/cpp`)**：
    - `MockMarketEngine` 生成 09:30~15:00 真实感分时走势与实时 Tick 行情，驱动 500ms 定时器跳动与短线精灵逐笔异动成交流；
    - `KeyboardGenie` 实现证券代码（如 `600519`）、拼音简拼（如 `GZMT`）极速联想匹配；
  - **同花顺经典布局与双主题 Tokens (`resource/layouts/stock_quotes.xml`, `resource/themes/theme_stock_classic.json`, `resource/themes/theme_stock_modern.json`)**：
    - `stock_quotes.xml` 完整还原顶部双层工具栏、左侧竖直 Dock、主图、资讯列表、右侧指标与逐笔面板、底部跨市场指数条与状态跑马灯；
    - 提供经典同花顺黑底模式与现代质感暗黑模式，支持按 `T` 键平滑热切换；
  - **独立示例工程与宿主控制 API (`samples/stock_terminal/main.cpp`, `include/ai_win_ui/host.h`, `src/ui_host.cpp`, `CMakeLists.txt`)**：
    - 在 `Host` 暴露 `ResetFinancialChart`、`AppendFinancialChartPoint`、`SetFinancialChartTheme` 接口；
    - 新增独立目标 `stock_terminal.exe`；
  - **验证**：`build.ps1` 编译 0 Error / 0 Warning，`run_ci_local.ps1 -SkipBuild` 7/7 项回归测试 100% PASS。

- **P0 + P1 局部脏矩形重绘与二次命中高斯阴影缓存 (Dirty-Rect Partial Repaint & Two-Hit Shadow Cache)**（分支 `feature/dirty-rect-and-shadow-cache`）
  - **真局部脏矩形重绘管线 (`src/ui_context.h`, `src/ui_element.h`, `src/ui_panels.h`, `src/ui_scroll.h`, `src/ui_widgets.h`, `src/renderer_skia.cpp`, `src/ui_host.cpp`)**：
    - `UIElement::InvalidateSelf(float padDip)` 通过 `UIContext::invalidateRect` 向宿主 `UiHost::AccumulateDirtyRect` 提交控件局部包围盒（含阴影外扩边距）；
    - `UiHost::FlushPendingDirtyInvalidate` 将 DIP 脏区转换为物理像素 `RECT` 调用 `::InvalidateRect(m_hwnd, &rc, FALSE)`，`OnPaint` 将 `ps.rcPaint` 传入 `m_renderer->SetDirtyRect(...)`；
    - `SkiaRenderer::BeginFrame` 在局部重绘帧仅裁剪并清除脏矩形，`PresentHwnd` 仅对 `(bx, by, bw, bh)` 脏矩形区域执行局部 `BitBlt`；
    - `Panel::Render`（含卡片阴影外扩边距）、`Label::Render`、`ScrollViewer::Render` 接入 `renderer.IntersectsDirtyRect(...)` 脏区剔除，使 `Switch` 60fps 过渡动画帧在 `Release` 下的 `render_ms` 从 `3.19 ms` 骤降至 **`0.17 ms`（P95 仅 `0.14 ms`，整帧 `paint_ms = 0.27 ms`，比初始基线快 510×）**，即使在 `Debug` 模式下也从 `137.74 ms` 降至 **`1.99 ms`**。
  - **二次命中准入（Two-Hit Admission）高斯软阴影位图缓存 (`src/renderer_skia.cpp`)**：
    - `SkiaRenderer::FillSoftShadow` 新增 `ShadowCacheKey -> {sk_sp<SkImage>, pad, hits}` 缓存（上限 256 项），采用 `hits >= 2` 准入策略：窗口缩放中仅出现 1 次的瞬态尺寸直接绘制到主画布，避免离屏 `SkSurface` 分配开销；静态卡片与固定尺寸控件在第 2 次绘制时自动光栅化为离屏 `SkImage` 位图并复用。
  - **基准测试 DPI 对齐修复 (`scripts/bench_frame_stats.ps1`)**：启用 `SetProcessDPIAware()` 并精确匹配 `AiWinUiWindowClass` 顶层窗口，消除 PowerShell 125% DPI 虚拟化导致的自动化点击偏移。
  - **验证**：`build.ps1` 编译 0 Error / 0 Warning，`run_ci_local.ps1 -SkipBuild` 7/7 项回归测试 100% PASS。

- **帧耗时剖析器与交互丝滑度优化 (Frame Profiler & Smoothness Optimization)**（分支 `feature/frame-profiler`）
  - **高精度帧耗时统计器 (`src/frame_profiler.h`, `src/ui_host.cpp`, `scripts/bench_frame_stats.ps1`)**：
    - 基于 `QueryPerformanceCounter` 采集每帧 `resize`、`layout`（含调用次数）、`render`、`present`、`paint` 与输入到上屏端到端 `latency` 耗时；
    - 升级 F12 调试面板 (`UiHost::DrawFrameStatsOverlay`) 实时展示最近 120 帧各阶段 `avg / p95 / max`（支持 `F11` 切换持续刷新及 `AI_WIN_UI_DEVTOOLS=1` 启动开启），并支持 `AI_WIN_UI_FRAME_STATS=<csv>` 逐帧导出；
    - 新增 `scripts/bench_frame_stats.ps1` 自动化帧耗时基准测试脚本，量化定位出 `skia-Debug` 下链接未优化 `Debug-x64/skia.lib` 及重复字体查询导致的 28 倍软光栅耗时瓶颈。
  - **五项靶向丝滑度优化**：
    - **默认 Release 交互启动 (`scripts/run_layout_demo.ps1`)**：新增 `-Configuration Auto|Release|Debug` 参数，交互演示默认启用 `Release`，将 `modern_components_v2` 拖拽端到端帧延迟从 `194.14 ms`（~5 FPS）降至 `8.86 ms`（~113 FPS，提速 21.9×）；
    - **Skia 字体与文本排版缓存 (`src/skia_font_shared.h`, `src/skia_text_layout.cpp`, `src/renderer_skia.cpp`)**：缓存 `CreateDefaultTypeface` DirectWrite 字体对象、`CreateSkiaTextLayout` 排版结果（上限 4096 项）及 `DrawSvgPath` 路径解析结果；
    - **按需 ``UpdateLayout()`` (`src/ui_host.cpp`, `src/ui_containers.h`, `src/ui_widgets.h`)**：移除 `UiHost::OnMouseDown` 无条件整页重排，改为仅在 `Expander` 展开/收起与 `AlertBanner` 关闭时调用 `requestLayout()`，使点击 `Switch` 等控件的布局开销从 `27.7 ms` 降至 `0.00 ms`；
    - **`Switch` 与 `SegmentedControl` 缓动动画 (`src/ui_widgets.h`)**：为 `Switch` 圆钮与轨道颜色、以及 `SegmentedControl` 胶囊滑块增加 Smoothstep 平滑过渡动画（`Release` 下单帧重绘仅 `3.19 ms`）；
    - **拖拽 DIB 64px 步进容量复用与同步重绘 (`src/renderer_skia.cpp`, `src/ui_host.cpp`)**：`SkiaRenderer` 按 64px 对齐容量复用 GDI DIB 位图（`resize` 分配耗时从 `1.26 ms` 降至 `0.04 ms`，提速 31.5×），并在 `UiHost::OnResize` 中调用 `RedrawWindow(..., RDW_INVALIDATE | RDW_UPDATENOW)` 同步上屏。
  - **验证**：`build.ps1` 编译 0 Error / 0 Warning，`run_ci_local.ps1 -SkipBuild` 7/7 项回归测试 100% PASS。

## 2026-11-01

- **现代基础组件库扩展 (Modern Components v2)**（分支 `feature/modern-components-v2`）
  - **展示与导航微组件 (`src/ui_widgets.h`)**：
    - `<Avatar>`：支持圆形/圆角方形、首字母缩写 (`initials`)、矢量图标回退 (`icon`) 及右下角在线状态指示点 (`status="online|busy|away|offline"`)；
    - `<Badge>` / `<Tag>`：支持 6 种语义色预设 (`success|warning|danger|info|primary|neutral`)、前置状态圆点 (`dot="true"`) 与前置矢量图标 (`icon`)；
    - `<Divider>`：支持水平/垂直分割线、实线/虚线 (`dashed="true"`) 及居中嵌入文字 (`text="..."`)；
    - `<SegmentedControl>`：支持 `items="Daily|Weekly|Monthly"` 分段胶囊切换，选中项带高斯软阴影高光与 `onChange` 回调。
  - **现代交互与表单控件 (`src/ui_widgets.h`)**：
    - `<Switch>`：现代胶囊拨动开关，支持点击与键盘 Space/Enter 切换及 `UIA::CheckBoxControlType` 无障碍映射；
    - `<SearchBox>`：复合搜索输入框，内置左侧搜索图标、占位符、非空态右侧一键清除按钮 (`clearable="true"`) 及空态快捷键胶囊徽标 (`shortcut="Ctrl+K"`)。
  - **反馈与状态指示组件 (`src/ui_element.h`, `src/ui_widgets.h`)**：
    - 基类级通用 `tooltip="..."` 属性：任意 `UIElement` 悬停时在 `RenderOverlay` 第二遍绘制通道自动弹出带高斯软阴影的深色气泡提示；
    - `<Alert>`：页内嵌状态横幅 (`variant="info|success|warning|error"`)，支持主副标题与点击右侧关闭按钮收起；
    - `<CircularProgress>`：双后端 SVG 圆弧环形进度指示器 (`0..100%`)，支持自定义环宽、颜色与中心百分比文字；
    - `<EmptyState>`：标准空数据占位组件（浅色圆形图标底座 + 矢量图标 + 标题 + 描述 + 操作按钮）。
  - **桌面级容器与环形图扩展 (`src/ui_containers.h`, `src/ui_chart.h`)**：
    - `<Expander>`：可折叠手风琴容器，点击 Header 栏动态展开/收起子容器并触发宿主 ``UpdateLayout()`` 重排；
    - `<SplitView>`：可拖拽双面板分栏容器，支持鼠标按住分隔条实时调整 `splitRatio` 并重排两侧子面板；
    - `<Chart>` 饼图/环形图 (`type="pie|donut"`)：支持 `innerRadiusRatio`、`centerValue`、`centerTitle`、自定义 `colors` 色板、扇区悬停外扩高亮及 Tooltip 联动。
  - **画廊与看板升级验证**：新增 `resource/layouts/modern_components_v2.xml` 挂载至 `demo_gallery.xml`，并升级 `medlio_dashboard.xml` 及子组件；`build.ps1` 编译通过，`run_ci_local.ps1 -SkipBuild` 7/7 项回归测试 100% PASS。

- **UI 视觉渲染与绘制表现力升级 (Visual Rendering v2)**（分支 eature/visual-rendering-v2）
  - **盒模型与高斯软阴影 (ox_decoration.*, 
enderer_skia.cpp, 
enderer.cpp)**：
    - Skia 后端 FillSoftShadow 接入 SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, sigma) 真高斯模糊阴影，Direct2D 后端升级为 12 步高斯衰减阴影，消除浅色卡片阴影阶梯感；
    - 新增四角独立圆角 (FillRRect / DrawRRect，支持 cornerRadius="tl,tr,br,bl") 与四边独立边框绘制（支持 orderBottom="1,#EEF2F7"、order="1,#EAEFF5" 等单边分割线及微边框）；
    - 修复 XML 解析器此前忽略 order、orderTop/Bottom/Left/Right 与 marginLeft/Top/Right/Bottom 简写属性的问题。
  - **字体排版与字重层级 (skia_font_shared.h, skia_text_layout.*, ui_panels.h)**：
    - 扩展 TextRenderOptions 与 <Label>，支持 ontWeight（100..900 及 medium/semibold/old）、ontFamily、lineHeight、lign/erticalAlign；
    - 移除 Skia SkSurfaceProps 中的 kUseDeviceIndependentFonts_Flag，恢复桌面端字形 Hinting 锐度。
  - **内置矢量图标与内联路径 (src/ui_icons.h, src/ui_media.h)**：
    - 新增 <Icon name="search|bell|calendar|..." />（内置 24+ 现代 Lucide 风格矢量图标）与 <Path data="M..." /> 控件；
    - Skia (SkParsePath::FromSVGString) 与 Direct2D（内置轻量 SVG Path M/L/H/V/C/A/Z 解析器）双后端原生渲染矢量图标与任意路径。
  - **图表平滑曲线与渐变精修 (src/ui_chart.h)**：
    - 新增 Catmull-Rom 平滑样条曲线（smooth="true"）、面积图垂直线性渐变填充（FillPolygonLinearGradient）、圆角胶囊柱（arRadius）、背景轨道槽（arTrackColor）、单柱高亮与柱顶悬浮胶囊标签（highlightIndex / highlightColor）以及柔和虚线网格（DrawDashedLine）。
  - **Medlio 看板全面升级与验证**：应用 <Icon> 矢量图标、多级字重、单边分割线、真高斯阴影与平滑渐变图表升级 medlio_dashboard.xml 及子组件；uild.ps1 编译通过， `run_ci_local.ps1 -SkipBuild 7/7 项自动化回归测试 100% PASS。

- **Medlio 医疗看板高保真 UI 对齐测试与视觉重构**（分支 `feature/medlio-dashboard`）
  - **架构层级颠覆与修复**：将旧方案中包裹在白底卡片内的 Sidebar 移至外部深蓝画布（`#2764FF`），确立“左深蓝侧边栏 + 右嵌入式浅灰白工作卡片（`#F6F8FB`，圆角 20px）”的经典高质感层级，解决整体对齐还原度偏低的问题。
  - **色彩材质与状态还原**：
    - 侧边栏：品牌 Logo 白字，激活态 Dashboard 为白底蓝字药丸胶囊，其余为浅蓝白半透明文字（`#BACFFF` / `#D8E5FF`）。
    - 浅灰白工作区：各个功能块均采用纯白底（`#FFFFFF`）+ 极细浅灰微边框（`#EAEFF5`）+ 14-16px 圆角。
  - **组件化全面赋能 (<Include>)**：
    - KPI 统计卡片：抽离为 `components/kpi_stat_card.xml`、`kpi_stat_card_revenue.xml`、`kpi_stat_card_referrals.xml`，首张卡片设为皇家蓝高亮；
    - 预约列表项：抽离为 `components/appointment_row.xml`（Checked_in 绿药丸与操作按钮）与 `components/appointment_row_scheduled.xml`（Scheduled 蓝灰药丸）。
  - **图表与数据拟真精调**：
    - Daily appointment count：周三（Wed）峰值药丸高亮，隐藏默认图例，定制分段器（Today | Monthly | Weekly）；
    - Revenue：金额排版（大字 4,820 + 灰字 .89 + 绿色增长药丸 +12.8%）与面积渐变图表。
  - **验证**：`build.ps1` 编译通过；`run_ci_local.ps1 -SkipBuild` 7 项自动化回归测试 100% PASS。

## 2026-10-31

- **声明式组件嵌套复用系统 (<Include> Phase 1)**（分支 `feature/component-include`）
  - **文件级组件嵌套**：
    - XML 语法：`<Include src="..." [override attributes...] />`
    - JSON 语法：`{ "type": "Include", "props": { "src": "...", ... } }`
  - **属性覆盖机制 (Attribute Override)**：父级在 `<Include>` 节点上声明的属性（如 `background`、`name`、`cornerRadius`、`padding`、尺寸与外边距等）直接覆盖被引入组件根节点的同名属性。
  - **相对路径寻址**：通过 `g_includeStack` 获取当前文件父目录，优先相对路径解析（如 `layouts/components/card.xml`），未命中时回退 Resource 根目录查找。
  - **循环引用与容错机制**：基于包含栈检测循环嵌套，遇到文件缺失或循环引用时不崩溃退出，优雅降级并在界面上呈现显眼的红色错误提示 Panel。
  - **画廊与测试覆盖**：新增 `resource/layouts/components/reusable_card.xml`、`reusable_card.json` 以及 `include_component_cases.xml` 并在主画廊中挂载卡片。
  - **验证**：`build.ps1` 编译通过；`run_ci_local.ps1 -SkipBuild` 7/7 PASS。

- **XML/JSON 解析器模块化与高性能三方库接入 (pugixml & yyjson)**（分支 `feature/modular-parsers`）
  - **解耦核心解析逻辑**：将原内联于 `layout_parser.cpp` 匿名命名空间中的 400 余行手写递归下降解析器剥离，新增独立的 `src/json_parser.h/.cpp` 与 `src/xml_parser.h/.cpp`。
  - **引入高性能轻量库**：
    - 在 `third_party/yyjson` 引入单源文件 `yyjson`，提供业界顶尖的 SIMD/现代 CPU 深度优化的 JSON 解析吞吐。
    - 在 `third_party/pugixml` 引入单源文件 `pugixml`，提供标准完整的 XML DOM 解析（完整支持实体解码、CDATA、自闭合及多属性规范）。
  - **工程与接口兼容**：
    - 更新 `CMakeLists.txt` 启用 `C` 语言支持并挂载三方源码；
    - 保留并维护原 `JsonValue` 与 `XmlNode` 稳定结构及 `operator[]` 接口，现有 `Theme`、`StyleCatalog`、UI 布局树解析实现零破坏性平滑迁移。
  - **验证**：`build.ps1` 编译通过；`run_ci_local.ps1 -SkipBuild` 7 项自动化回归测试 100% 通过（7/7 PASS）。

## 2026-10-23

- **Chart v2 Phase 1：通用 `<Chart>` 组件**（分支 `feature/chart-v2`）
  - **新增 `src/ui_chart.h`**（header-only，经 `ui.h` 引入）：支持 `line` / `bar` / `area` 多系列，仅使用 `IRenderer` 现有接口（`DrawPolyline`、`FillPolygon`、`FillRect`、`DrawTextW`），Skia 与 Direct2D 共用。
  - **坐标轴**：Y 轴自动 nice-tick（1/2/5×10ⁿ）；X 轴支持分类轴（`categories`）与数值轴（系列 `x` 属性，仅折线/面积；含柱状系列时回退分类轴）；水平网格。
  - **交互**：悬停显示十字准线 + Tooltip（`RenderOverlay` 绘制，避免被兄弟控件遮挡）；点击图例切换系列显隐。
  - **布局解析**：XML `<Chart>` + `<Series name type values x color visible>` 子节点；JSON `type: "Chart"` + `props.series`。分类以 `|` 分隔。支持 `showLegend/showGrid/showTooltip/minValue/maxValue/fontSize` 等属性。
  - **Host API**：`Host::SetChartSeriesData(name, seriesIndex, values, count)`、`Host::SetChartSeriesVisible(name, seriesIndex, visible)`。
  - **UIA**：Chart 自动设置摘要 `uiaHelpText`（系列数/点数）；深度无障碍（逐点朗读）留待后续。
  - **示例**：`resource/layouts/stats_components.xml` 新增折线+面积、柱状、数值 X 轴三个 Chart。
  - **验证**：`build.ps1` 无错误；`run_ci_local.ps1 -SkipBuild` 7/7 PASS。
  - **后续 (Phase 2)**：时间轴、双 Y 轴、对数轴、饼图/环图、`SparklineChart` 复用。

## 2026-10-09


- **Wave5：UIA 无障碍自动化 (UI Automation Basics)**
  - 接入 `uiautomationcore` 库，为 `ai-win-ui` 提供无障碍访问支持。
  - **COM 接口实现**：新增 `UIAElementProvider` 与 `UIAHostProvider`（基于 WRL），分别实现 `IRawElementProviderSimple`、`IRawElementProviderFragment`、`IRawElementProviderFragmentRoot`。
  - **宿主集成**：在 `ui_host.cpp` 的 `WM_GETOBJECT` 中拦截 `UiaRootObjectId`，返回 UIA 根提供程序。
  - **控件角色映射**：`UIElement` 及其派生类（`Button`, `Label` 等）支持重写 `GetUiaControlType` 以返回标准的 `UIA_ButtonControlTypeId` 等角色 ID。
  - **布局解析**：XML/JSON 支持 `uiaName` 与 `uiaHelpText` 属性，供读屏软件读取。
  - **测试与验证**：修复编译重定义错误，统一 `#include <uiautomation.h>`。本地全套 7 项 CI 回归测试（Skia/D2D Smoke、Core/Yoga/ScrollViewer Golden）均通过。

## 2026-09-26

- **Wave5：Flex + Chrome 规范、画廊与 CI 验证**
  - **规范化 (Cursor AI Rules)**
    - 新增 `.cursor/rules/ai_win_ui_layout.mdc` 规则，指导 Cursor AI 生成标准的 XML/JSON 布局，强制规范 Flex 属性、Window Chrome 设定及 `hitTest` 控制。
  - **画廊验证 (Validation Gallery)**
    - 新增 `resource/layouts/flex_chrome_validation.xml`，综合测试 Layered Chrome (透明层叠窗口)、标题栏拖拽以及复杂的 Flex 多行包裹。
    - 修改 `resource/layouts/demo_gallery.xml`，添加 `Flex Chrome Validation` 入口卡片。
  - **CI (Q8)**
    - `scripts/run_ci_local.ps1` 跑通全套 7 项自动化回归测试。

- **Wave5：响应式数据绑定 (Phase 3)**
  - **Observable 生命周期强化**：修改 `observable.h` 支持 `Subscribe` 返回 ID 及 `Unsubscribe` 注销。
  - **Host::BindText API**：扩展 C++ 宿主接口，允许将 `Observable<std::wstring>` 绑定至指定名称的 `Label`/`Button` 控件，且宿主析构时安全解绑。
  - **Embedder Action 拦截**：完善 `main.cpp` 示例，支持从 Gallery 接收自定义 Action (`openBindingDemo`)，展示如何动态创建 Secondary Host 并与全局/静态状态绑定。
- **Wave5：声明式过渡动画 DSL (Q8)**
  - **CSS 风格过渡**：实现 `<Panel transition="background 500ms ease-in, border 300ms linear">` 属性解析，支持指定属性的动画插值。
  - **独立计时与刷新**：将 `OnTimer` 修改为以 `dtMs` (Delta ms) 为参数并沿组件树广播，实现基于实际流逝时间的帧无关（Frame-independent）色彩插值。
  - **动画装饰解析**：在 `ui_element.h` 中新增 `ResolveAnimatedDecoration`，拦截静态渲染过程并注入当前计算的色彩中间态，支持对 `Panel`, `Button`, `TextBox` 等基础控件的热插拔生效。
  - **集成画廊 Demo**：新增 `transition_demo.xml` 展示鼠标悬浮触发的色彩渐变，已集成入主页面 Gallery 测试。
- **Wave5：键盘控制与交互增强 (Keyboard-matrix)**
  - **焦点请求接口**：在 `UIContext` 新增 `requestFocus` 允许 `UIElement` 主动申请获得焦点。
  - **RadioGroup 方向键导航**：在单选框分组内支持使用方向键自动切换并移交焦点，具备首尾循环能力。
  - **DataTable 状态控制**：实现在非编辑模式下按下 `Esc` 键清除所有选中行，进一步完善无障碍键盘操作。
## 2026-07-26

- **Wave4：架构模块化重构、渲染性能优化与响应式动效系统**
  - **代码模块解耦 (Phase 1)**
    - 将 191KB 巨型 [layout_parser.cpp](file:///f:/AI-Projects/ai-win-ui/src/layout_parser.cpp) 解耦为 `layout_parser_panels.cpp` 与 `layout_parser_controls.cpp`；
    - 将 [ui_panels.h](file:///f:/AI-Projects/ai-win-ui/src/ui_panels.h) 复杂的容器方法剥离至 [ui_panels.cpp](file:///f:/AI-Projects/ai-win-ui/src/ui_panels.cpp)；
    - 将 `ui_host.cpp` 拆解输入路由 [ui_host_input.cpp](file:///f:/AI-Projects/ai-win-ui/src/ui_host_input.cpp) 与浮层管理 [ui_host_overlay.cpp](file:///f:/AI-Projects/ai-win-ui/src/ui_host_overlay.cpp)。
  - **渲染性能与显存/内存优化 (Phase 2)**
    - **TextLayout 测量缓存**：在 [text_measurer.cpp](file:///f:/AI-Projects/ai-win-ui/src/text_measurer.cpp) 中新增 LRU 文本测量缓存（默认 1024 项），避免滚动与重复测量时的 DWrite/Skia 布局重建开销；
    - **脏区包围盒剔除 (Dirty Rect BBox Culling)**：在 [renderer.h](file:///f:/AI-Projects/ai-win-ui/src/renderer.h) 与 [ui_element.h](file:///f:/AI-Projects/ai-win-ui/src/ui_element.h) 中增加脏区检测相交剪枝，直接剔除非视区的 UI 节点 Render；
    - **SVG DOM 显存 LRU 淘汰**：在 [renderer_skia.cpp](file:///f:/AI-Projects/ai-win-ui/src/renderer_skia.cpp) 中限制 256 项 SVG DOM 的显存淘汰。
  - **响应式绑定与动效 Transition 引擎 (Phase 3/4)**
    - **`Observable<T>` 响应式属性**：导出公开头文件 [include/ai_win_ui/observable.h](file:///f:/AI-Projects/ai-win-ui/include/ai_win_ui/observable.h)，实现变量改变自动更新 UI；
    - **Animation Controller**：新增 [src/animation_controller.h](file:///f:/AI-Projects/ai-win-ui/src/animation_controller.h)，支持 Linear, EaseIn, EaseOut, CubicBezier, Spring 缓动插值算法与 ColorLerp / FloatLerp 计算。
  - **平铺纯 C-API 导出 (Phase 5/6)**
    - 导出公开头文件 [include/ai_win_ui/c_api.h](file:///f:/AI-Projects/ai-win-ui/include/ai_win_ui/c_api.h) 与 [src/c_api.cpp](file:///f:/AI-Projects/ai-win-ui/src/c_api.cpp)；
    - 导出 C 句柄 `AiHostHandle` 与 `AIWinUI_CreateHost`、`AIWinUI_DestroyHost`、`AIWinUI_Run`、`AIWinUI_ProcessMessage` 平铺 API，为 C# / Rust / Python 绑定提供支持。
  - **DevTools & 性能监控浮层 (Phase 7)**
    - 支持按键 `F12` 唤起/隐藏 DevTools 调试条；
    - 在右上角实时展示渲染 Backend (Direct2D / Skia)、FPS 帧率与视口 Resolution 尺寸。
  - **自动化 CI 全过 (Q8)**
    - `scripts/run_ci_local.ps1` 跑通全套 7 项自动化测试（Skia/D2D Smoke、Core/Yoga/ScrollViewer Golden），回归通过。


- **Wave3：可嵌入 + GitHub 绿勾**
  - H5b：`UiHost` → `src/ui_host.cpp`；`main.cpp` 薄入口
  - H6：`ai_win_ui_lib` 静态库 + `samples/embed_host`；公开 `include/ai_win_ui/host.h`
  - H7：`version.h` + `CHANGELOG.md`（**0.3.1** child embed）
  - **parent HWND**：`HostCreateInfo::parent` → `WS_CHILD`；`FitToParent` / `ResizeClient`；embed_host shell 示例
  - CI：`setup_ci_deps.ps1`（Yoga）；GHA **D2D-only**（Skia 不在 git）；`run_ci_local` 无 Skia 时自动 d2d
- **Wave2 续：Q8 本地 CI**
  - `scripts/run_ci_local.ps1`：build + gallery coverage + headless smoke + measure golden
  - `.github/workflows/ci.yml`：Windows Actions 调用同一脚本
  - 文档：README / mvp-acceptance / doc/README
- **Wave2 续：R8 Svg tint + cache**
  - `CreateSvgFromBytes(..., cacheKey)` path/hash 缓存；`DrawSvg` 可选 tint（Skia SrcIn）
  - `SvgIcon tint=` / `color=`；XML 解析补齐；demo `svg_tint_demo.xml`（alias `svg-tint`）
- **Wave2 续：R7 渐变/阴影**
  - `IRenderer::FillLinearGradient` / `FillSoftShadow`（D2D 真渐变 + 双后端 multi-pass 软阴影）
  - `BoxDecoration.gradient` / `.shadow`；`DrawBoxDecoration` 先阴影后填充
  - Panel 属性：`gradientStart/End/Angle`、`shadowBlur/OffsetX/OffsetY/Color`；style JSON 同字段
  - Demo：`gradient_shadow_demo.xml`（alias `gradient-shadow`）+ gallery 卡
- **Wave2 续：文档同步 + R6**
  - **R6** `TextRenderOptions.ellipsis`；Skia 前缀+U+2026；D2D `SetTrimming`；`Label ellipsis="true"`；demo `text_ellipsis_cases.xml`
  - 文档：`mvp-roadmap` / `mvp-execution-plan` / `doc/README` / 根 README / `layout-spec` / `skia-integration` / `mvp-acceptance` / `keyboard-matrix` / personas Wave2 进度
- **Wave2 开跑（产品能力）**
  - **C5** `VirtualListBox`：`itemCount` / `itemPrefix` / binder；demo `virtual_list_demo.xml`（1000 行）；gallery 卡 + alias `virtual-list`
  - **C4** `OverlayFlyout` 共用几何；Combo + Popup 同路径；popup-theme 加 Combo 对照
  - **S4** `$color.*` 运行时 rebind（Panel/Label token）；catalog 名 `SetStyleFromCatalog`；`applyTheme` 重载 StyleCatalog + `SetActiveTheme`；`buttons.json` 改 token
  - **H5** 嵌入 API 设计草案：`doc/plan/2026-07-16-embed-api-v1.md`
- **Wave1 收口（L1 Exit 勾选）**
  - L2：`scroll_viewer_cases` A/B/C（嵌套 ListView/ListBox + 双轴）；`ListBox`/`DataTable` `OnMouseWheel`；layout-spec 滚轮消费矩阵
  - L4：scroll-viewer / core-validation / yoga-measure 双后端 golden 对齐
  - C3：IME composition font 跟 caret 高度；`mvp-acceptance` IME 表
  - H4 / R5：退出稳定性步骤 + `skia-integration` 已知差表
  - C4/S4 标为 Wave2 续（Popup 共用基类 / 硬编码色换肤）
- Wave1 续 8：C4 `Popup` 控件 + demo；S4 `applyTheme:` + `themes/light.json`；gallery 卡 Popup+Theme
- Wave1 续 7：H1 `App`→`UiHost` + `ui_host.h`/`OpenSecondaryUiHost`；C2 `DismissAllOverlays` + Esc；overlay 契约文档；Q4 gallery 验收表

## 2026-07-15

- Wave1 续 6：R2 Skia CJK soft-break 对齐 DWrite wrapH；修复 compare/golden 脚本 PowerShell `>` 误用为重定向
- Wave1 续 5：Q3 headless smoke 扩页 + 最新 exe 回退；L4 `yoga-measure` skia/d2d golden
- Wave1 续 4：S2 Theme/Catalog 边界文档；S3 启动日志 theme/styles 路径；L3 yoga space-between + min/max 段
- Wave1 续 3：`application.h/cpp` 多 host 注册；IME 候选 exclude + composition font；`text_wrap_cases` R2 页
- Wave1 续 2：`host_options.h`；Combo 点外关闭 `DismissOverlaysAt`；TextInput caret 最近边界；`token-migration-strategy.md`
- Wave1 续：`OpenHostOptions`；`AI_WIN_UI_DISABLE_WINDOW_SCROLL`；`compare_text_dumps.ps1`（core-validation skia/d2d **PASS ≤2px**）；`doc/keyboard-matrix.md`
- **GridPanel → Yoga flex-wrap grid**：`ILayoutEngine::MeasureGrid` / `ArrangeGrid`；等分列宽 + `rowHeight` + gap；手写 fallback 保留
- Wave1 **分任务执行**落地：
  - H3：`LogEffectiveEnv` + 标题 chrome + `AI_WIN_UI_IGNORE_ENV`
  - Q1：`check_gallery_coverage.ps1`；Q2：`core-validation` skia/d2d golden
  - R/A6：`AI_WIN_UI_TEXT_DUMP` + `run_text_dump.ps1`；`wave1-text-parity-notes.md`
- DirectUI **产品化路线 · 分人物**：`doc/plan/2026-07-15-directui-productization-personas.md`
  - 六角色 R 文本 / L 布局 / C 控件 / S 样式 / H 宿主 / Q 质量
  - 三 Wave（L1 引擎可信 → L2 产品能力 → L3 交付形态）+ 认领表 + 文件边界
  - 辅助：`scripts/clear_ai_win_env.ps1`
- P0 硬化第二批：
  - ScrollViewer：**拖拽内容滑动** + 滚动条 thumb/track
  - 进程内多根窗口（关子窗不杀 hub）；`AI_WIN_UI_CHILD_PROCESS=1` 仍可多进程
  - measure golden：`scripts/run_measure_golden.ps1` + `tests/golden/scroll-viewer.skia.ndjson`
- P0 硬化首批代码（A/B/D）：
  - 文本：`CreateSkiaTextLayout` 规则表 + bold/italic 与 draw 共用；DWrite NoWrap 首行
  - `ScrollViewer` + `scroll_viewer_cases`（别名 `scroll-viewer`）
  - `name`/`id` + `AI_WIN_UI_MEASURE_DUMP` NDJSON + `scripts/run_measure_dump.ps1`
- DirectUI 差距评估 → P0 硬化计划：`doc/plan/2026-07-15-directui-p0-hardening.md`
  - Track A 文本 parity / B ScrollViewer / C 进程内多根 / D measure golden
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
