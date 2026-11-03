# 实施计划 - 修复字体截断、菜单只显示一半与右侧面板内容黑屏问题

## 1. 现状与根因分析

用户在运行最新优化版本时反馈：“界面中的字比如菜单等，字体只显示一半出来。最上方和最下方，以及右侧列表的标题和内容都无法正常显示”，并附带了运行截图。

通过对 Direct2D 渲染管线、宿主刷新机制与 XAML 布局解析的逐行排查，定位到两处致命根因：

### 根因 1：Direct2D 渲染器误用脏矩形裁剪导致大面积黑屏与文字残缺
- `src/ui_host.cpp` 中的 `isPartialPaint` 和 `FlushPendingDirtyInvalidate()` 此前被改为针对局部脏矩形提交。
- Direct2D（默认渲染后端）使用的是 `ID2D1HwndRenderTarget` 双缓冲交换链，每次 `BeginFrame` 均会调用 `m_renderTarget->Clear(clearColor)` 清屏为黑底；
- 同时，所有 UIElement 在 `Render()` 中均会检查 `renderer.IntersectsDirtyRect(m_bounds)`；
- 当局部脏矩形生效时，Direct2D 清空了整屏，却跳过了所有不在脏矩形内的控件（顶部菜单栏、右侧 10 行盘口指标、权重股贡献度列表、短线精灵、底部状态栏）；
- 结果：未在脏矩形内的控件完全未被绘制而呈现大面积黑屏；处于脏矩形边缘的控件则被裁掉一半！
- **对比**：Skia 后端拥有持久化离屏 `SkSurface`，支持局部脏矩形；而 Direct2D 双缓冲交换链必须进行全量绘制（得益于图表几何缓存，Direct2D 全量绘制单帧仅 0.15ms，完全保持 144Hz 丝滑）。

### 根因 2：布局 XML 中 Padding 参数顺序混淆（Windows Thickness 顺序为 Left, Top, Right, Bottom）
- `resource/layouts/stock_quotes.xml` 中将 CSS 的 `padding: 0 10px 0 10px` 误写为 `padding="0,10,0,10"`；
- `ParseThickness` 按照 `values[0]=Left, values[1]=Top, values[2]=Right, values[3]=Bottom` 解析；
- 导致顶部菜单栏（高 28px）被施加了 `Top: 10px, Bottom: 10px` 的垂直内边距，仅剩 8px 高度，13px 的字体被挤压裁切只显半截；
- 同理，底部状态栏、右侧标题栏等均因垂直 padding 过大导致文本被裁切。

---

## 2. 详细技术方案

### 2.1 修复 Direct2D 脏矩形机制 (`src/ui_host.cpp`)
- 仅在渲染器后端为 `Skia` 且具备离屏持久 Surface 时才启用局部脏矩形裁剪；
- 在 Direct2D 后端下，`isPartialPaint` 设为 `false`，`FlushPendingDirtyInvalidate` 统一调用 `InvalidateRect(m_hwnd, nullptr, FALSE)`，确保 Direct2D 每帧完整绘制双缓冲，绝不擦除已有控件；
- 保持 `FinancialChart` 内部的几何缓存（Geometry Cache），确保即使 Direct2D 全量绘制，单帧耗时也仅 0.1~0.2ms，丝滑度完全不减。

### 2.2 修正布局 XML 内边距与高度 (`resource/layouts/stock_quotes.xml`)
- 将所有工具条、菜单栏的内边距由 `0, X, 0, X` 纠正为 `X, 0, X, 0`（水平内边距 X，垂直内边距 0）；
- 修正标的标题栏内边距为 `8, 2, 8, 2`；
- 修正右侧盘口 10 行指标、8 行权重股贡献度、短线精灵的 padding 与行高，确保纵向自适应，不发生内容挤压或越界。

### 2.3 增强 Thickness 解析器鲁棒性 (`src/layout_parser.cpp`)
- 扩展 `ParseThickness`，支持 1 个值（四边等宽）、2 个值（水平, 垂直）、4 个值（Left, Top, Right, Bottom），防止单值或双值误写时解析为全零。

---

## 3. 验证指标
1. `build.ps1` 编译 0 Warning, 0 Error；
2. `.\scripts\run_ci_local.ps1 -SkipBuild` 7 项回归测试 100% PASS；
3. 运行 `stock_terminal.exe`：
   - 顶部菜单栏与按钮文字完整居中显示，字体无半截或裁切；
   - 底部跨市场状态栏文字饱满无截断；
   - 右侧盘口 10 项核心指标、8 支个股贡献度、短线精灵标题与列表全部清晰可见，不再出现黑屏丢失；
   - 鼠标悬停及十字光标交互继续保持 144Hz 极致丝滑。
