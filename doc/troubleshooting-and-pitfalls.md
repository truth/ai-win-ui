# ai-win-ui 关键排坑与架构避坑指南 (Troubleshooting & Pitfall Guide)

本文档总结了在 `ai-win-ui` 高性能 C++ 桌面渲染、Yoga 弹性排版、Direct2D/Skia 双渲染后端开发过程中遇到的核心“暗坑”与设计陷阱，为后续开发、重构与排障提供直接的避坑准则。

---

## 目录
1. [排版与内边距顺序陷阱 (Thickness Order)](#1-排版与内边距顺序陷阱-thickness-order)
2. [容器默认内边距与紧凑单行高度塌陷 (Padding vs Row Height)](#2-容器默认内边距与紧凑单行高度塌陷-padding-vs-row-height)
3. [Direct2D 双缓冲误清屏与局部脏矩形剪裁 (D2D vs Skia Dirty Rect)](#3-direct2d-双缓冲误清屏与局部脏矩形剪裁-d2d-vs-skia-dirty-rect)
4. [高频数据更新与 Yoga 全树重排风暴 (Batch Update)](#4-高频数据更新与-yoga-全树重排风暴-batch-update)
5. [DPI 缩放与几何排版转储排查技巧 (Measure Dump)](#5-dpi-缩放与几何排版转储排查技巧-measure-dump)
6. [Windows GUI 进程生命周期与文件锁 (Process Management)](#6-windows-gui-进程生命周期与文件锁-process-management)

---

## 1. 排版与内边距顺序陷阱 (Thickness Order)

### 踩坑表现
设置菜单栏、状态栏或面板的内边距时，文字被上下压扁，只显示上半截或下半截。

### 根本原因
- **CSS 顺序**：`padding: Top, Right, Bottom, Left`（顺时针，从上开始）。
- **Windows / WPF / ai-win-ui 顺序**：`Thickness` 的标准顺序是 **`Left, Top, Right, Bottom`**！
- 若在 XML 中误写为 `padding="0, 10, 0, 10"`（本意为左右 10px、上下 0px），解析器会将其解释为：
  - `Left = 0`, `Top = 10`, `Right = 0`, `Bottom = 10`。
  - 导致上下被加上合计 20px 巨大内边距！一个 28px 高度的菜单栏减去 20px 仅剩 8px，12px 字体被严重截断。

### 避坑守则
1. **牢记标准顺序**：`padding="L, T, R, B"`。水平 10px、垂直 0px **必须写为 `10, 0, 10, 0`**。
2. **善用简化写法**：解析器已支持：
   - 1 个值：`padding="8"`（四边均为 8px）
   - 2 个值：`padding="10, 0"`（水平 10px, 垂直 0px）
   - 4 个值：`padding="10, 2, 10, 2"`（左, 上, 右, 下）

---

## 2. 容器默认内边距与紧凑单行高度塌陷 (Padding vs Row Height)

### 踩坑表现
在定义紧凑表格行、指标网格（如 `height="18"` 或 `height="19"`）时，行背景色正常绘制，但里面的 `<Label>` 文本**完全不可见/隐形**。

### 根本原因
1. C++ 中 `Panel` 构造函数拥有历史默认属性：`padding{8, 8, 8, 8}`。
2. 在标准 1.25x DPI 缩放下，上下内边距缩放为 `10px + 10px = 20px`。
3. 若行容器 `<Panel direction="row" height="19" alignItems="center">` 未显式写 `padding`，它将继承这 20px 上下内边距：
   $$\text{可用内容高度} = 23.75\text{px} - 20\text{px} = 3.75\text{px}$$
4. Yoga 布局引擎在测量叶子节点时施加 `AtMost` 约束，把 `Label` 的高度硬截断为 `h = 3.0px`。
5. DirectWrite / Skia 在 3px 矩形内无法绘制 15px 高度的文字，导致文字彻底被裁切消失。

### 避坑守则
1. **XML 规范**：所有定义了单行高度（`height <= 24px`）的 Panel，**必须显式指定 `padding="0,0,0,0"` 或 `padding="0"`**。
2. **引擎防护**：`layout_parser.cpp` 中已内置安全兜底规则：当 Panel 指定了固定高度且 `height <= (padding.top + padding.bottom)` 或 `height <= 24px` 且未显式指定 padding 时，自动清零 padding。自定义新控件时切忌在构造函数中赋予过大默认内边距。

---

## 3. Direct2D 双缓冲误清屏与局部脏矩形剪裁 (D2D vs Skia Dirty Rect)

### 踩坑表现
开启局部脏矩形剪裁（`IntersectsDirtyRect`）优化后，界面部分区域（如顶部、右侧）出现大面积黑屏或闪烁，鼠标移动时光标扫过的区域才会偶尔露出界面。

### 根本原因
- **Direct2D**：使用的是基于 DXGI 交换链（SwapChain）的呈现模式，后台缓冲不具备持久内容（每次 Present 后内容失效或交换）。若在绘制前调用 `d2dContext->Clear()` 清屏，但在重绘树时跳过了非脏区元素（`IntersectsDirtyRect` 为 false），这些区域将直接保留清屏后的黑底！
- **Skia**：使用内存持久的离屏 Surface/Bitmap，非脏区保留着上一帧的完整像素，仅擦除并重绘脏矩形即可。

### 避坑守则
1. **无持久离屏缓冲的后端不得使用跨帧跳过绘制**：在 `src/ui_host.cpp` 中，仅对 Skia 启用 `IntersectsDirtyRect`，Direct2D 保持全量提交。
2. **144Hz 高刷性能靠几何缓存**：对于图表等重型绘制组件（K 线、分时线），必须引入**几何点集缓存（Geometry Cache）**。图表重绘时只绘制已排版好的几何缓存点集（耗时 < 0.1ms），即便 Direct2D 帧帧全绘，CPU/GPU 占用依然极低，完全可以轻松跑满 144Hz。

---

## 4. 高频数据更新与 Yoga 全树重排风暴 (Batch Update)

### 踩坑表现
金融终端定时器每 500ms 刷新 10~20 项行情指标 Label，窗口在鼠标悬停或行情跳动时产生微卡顿，CPU 偶尔飙升。

### 根本原因
- 默认情况下，每次调用 `host->SetElementText("id", text)`，如果文本长度变化，UIElement 会请求父容器重新布局，触发 Yoga 的 `UpdateLayout()` 遍历全树，并调用 `InvalidateRect(m_hwnd)` 请求重绘。
- 如果一次性连续更新 15 个 Label，会造成 15 次全树重排与重绘请求风暴。

### 避坑守则
1. **使用批处理包装**：在高频或定时批量更新文本时，必须使用批处理接口：
   ```cpp
   host->BeginBatchUpdate();
   host->SetElementText("label_a", valA);
   host->SetElementText("label_b", valB);
   // ... 批量更新完毕
   host->EndBatchUpdate(false); // false 表示不需要全树重新排版
   ```
2. **区分布局改变与内容绘制**：仅修改同等位宽的数值文本（如现价、涨跌幅）时，不需要触发 Yoga 全树排版，仅提交 Label 自身的局部脏矩形重绘即可。

---

## 5. DPI 缩放与几何排版转储排查技巧 (Measure Dump)

### 踩坑表现
界面在开发者机器上正常，在用户的 125% 或 150% DPI 屏幕上出现错位、折行或挤压。肉眼观察很难知道到底是哪个节点被压缩。

### 最佳排查手段：转储几何树 (Measure Dump)
不要靠肉眼猜尺寸，使用系统内置的排版度量转储工具：
```powershell
$out = (Resolve-Path "scratch").Path + "\stock_dump.ndjson"
.\scripts\run_measure_dump.ps1 -Layout "layouts/stock_quotes.xml" -Renderer "skia" -OutFile $out -QuitAfterMs 1000
```
该工具会输出全树所有节点的 NDJSON 几何数据：
```json
{"path":"n0/n3/n2/n1/n0/grid_weibi","dw":77.5,"dh":26.28,"x":656,"y":146,"w":78,"h":23}
```
- `dw, dh`：控件期望尺寸（Desired Width / Height）。
- `w, h`：Yoga 最终分配给控件的几何包围盒宽高。
- **一旦发现 `dh` 正常（如 26px）但 `h` 异常（如 3px），即可 100% 确定是其父容器的 Cross-Axis 或 Padding 产生了严重挤压约束！**

---

## 6. Windows GUI 进程生命周期与文件锁 (Process Management)

### 踩坑表现
1. 运行 `build.ps1` 报错：`fatal error LNK1104: 无法打开文件“stock_terminal.exe”`。
2. PowerShell 后台调用 `Start-Process` 时窗口不显示或立即退出。

### 避坑守则
1. **构建前先杀旧进程**：
   ```powershell
   Stop-Process -Name "stock_terminal" -Force -ErrorAction SilentlyContinue
   Stop-Process -Name "ai_win_ui" -Force -ErrorAction SilentlyContinue
   ```
2. **GUI 程序非阻塞启动**：
   - Windows GUI 子系统（`wWinMain`）程序在控制台中运行时不会阻塞控制台。
   - 使用带工作目录的 `Start-Process` 明确其资源路径：
     ```powershell
     Start-Process -FilePath "build\Debug\stock_terminal.exe" -WorkingDirectory "build\Debug"
     ```
3. **文件路径使用绝对路径**：
   - 环境变量（如 `AI_WIN_UI_MEASURE_DUMP`）在子进程读取时，其当前工作目录为 exe 所在目录，相对路径会导致文件生成到 `build/` 内部而非项目根目录。必须始终使用 `(Resolve-Path ...).Path` 解析为绝对路径。
