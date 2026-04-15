# AI WinUI 项目 MVP 与发展路线

## 目标

为当前项目制定最小可行产品（MVP）和后续发展路线，保障快速落地与可扩展性。

---

## 一、最小可行产品（MVP）

### 1. 基础运行能力

- Win32 窗口与 Direct2D 渲染正常工作。
- UI 树可渲染：`Panel`、`Label`、`Button`。
- 鼠标事件可交互：hover、pressed、click。
- `resource/` 目录加载布局文件与图片。
- `lib/` 目录支持预编译依赖库引入。

### 2. 最低功能范围

- 支持 JSON 或 XML 布局定义文件。
- 运行时从 `resource/layouts/` 加载布局，构建 UI 树。
- `Button` 支持点击回调映射。
- 支持基本布局：垂直排列、padding、spacing、自动宽度。
- 支持 `assets.zip` 作为可选资源包形式。

### 3. MVP 核心交付

- `src/ui.h`：保留当前 UI 元素，并新增布局节点构建逻辑。
- `src/renderer.h/cpp`：保持 Direct2D 渲染实现。
- `src/main.cpp`：增加资源提供器与布局解析入口。
- `resource/`：至少提供一个布局示例文件和一个图片资源。
- `lib/`：支持后续放置预编译库。

---

## 二、阶段性开发路线

### 阶段 0：准备与规范

1. 明确目录结构：
   - `lib/`：依赖包与 native 库
   - `resource/`：布局、图片、字体、样式
2. 完成文档：
   - `doc/README.md`
   - `doc/resource-packaging.md`
   - `doc/mvp-roadmap.md`
3. 确定接口：
   - 资源提供器 `IResourceProvider`
   - 布局解析器 `LayoutParser`
   - 渲染后端接口 `IRenderer`（可选）

### 阶段 1：MVP 实现

1. 实现 `DirectoryResourceProvider`。
2. 实现 `JSON/XML` 布局解析到 `UIElement` 树。
3. 完成 `resource/layouts/` 示例文件及运行验证。
4. 保证 `Button` 点击、hover、pressed 状态正常。
5. 做好 `assets.zip` 加载的可选实现骨架。

### 阶段 2：扩展布局与资源

1. 增强 `Panel` 为更灵活的容器，支持 `align-items`、`justify-content`。
2. 增加 `Image` 组件，支持从 `resource/images/` 加载图片。
3. 增加 `Font` / `Style` 配置支持。
4. 实现 `ZipResourceProvider`，支持从 `assets.zip` 加载资源。

### 阶段 3：引入预编译依赖与后端抽象

1. 在 `lib/` 中加入预编译库示例。
2. 抽象渲染接口 `IRenderer`，保留现有 Direct2D 后端。
3. 设计并验证 `Skia` 后端切换方案（后续实现）。
4. 设计并验证 `Yoga` 布局引擎替换方案（后续实现）。

---

## 三、可选优化与规模化发展

### 3.1 窗口与视觉

- 支持无 title 模式、窗口拖拽区、自定义边框。
- 支持圆角、透明、不规则窗口区域。
- 支持主题与样式切换。

### 3.2 事件与交互

- 增加 `OnMouseEnter`、`OnMouseLeave`、键盘事件、焦点管理。
- 支持拖拽、滑动、长按等手势交互。

### 3.3 资源与运行时

- 支持 `assets.zip` 与 `resource/` 的热加载模式。
- 支持资源版本管理与热更新。
- 支持远程资源加载与动态主题切换。

### 3.4 引擎与后端

- 逐步将布局层替换为 `Yoga`。
- 逐步将渲染层替换为 `Skia` 或通用后端抽象。
- 形成可跨平台 UI 引擎骨架。

---

## 四、按步骤执行计划

### Step 1：整理现有基础

- [ ] 目录约定完成：`lib/`, `resource/`
- [ ] 功能文档完善
- [ ] 基础项目结构确认

### Step 2：实现资源加载与布局解析

- [ ] `DirectoryResourceProvider`
- [ ] `LayoutParser`（JSON/XML）
- [ ] `resource/layouts/` 示例布局
- [ ] `UIElement` 树构建与渲染验证

### Step 3：完善 MVP 行为交互

- [ ] `Button` 点击与状态
- [ ] 运行时资源读取验证
- [ ] `assets.zip` 优雅降级

### Step 4：扩展布局与资源支持

- [ ] `Image` 组件
- [ ] `Style` / `Font` 属性
- [ ] `ZipResourceProvider`

### Step 5：准备后续引擎升级

- [ ] `IRenderer` 抽象接口设计
- [ ] `Yoga` 与 `Skia` 引入方案确认
- [ ] `lib/` 中放置预编译依赖示例

---

## 六、迭代执行计划

建议按周执行，快速交付并逐步验证。每周结束后评估结果并调整下一周计划。

### Week 1：MVP 骨架搭建

- [ ] 确认 `lib/`、`resource/`、`doc/` 目录结构
- [ ] 完成 `DirectoryResourceProvider` 实现
- [ ] 增加 `resource/layouts/` 示例布局文件
- [ ] 基于现有 `UIElement` 构建 JSON/XML 解析器
- [ ] 确保 `Panel/Label/Button` 可以从布局文件实例化并正确渲染

### Week 2：运行时资源加载与交互

- [ ] 实现 `Button` 点击、hover、pressed 行为
- [ ] 完成 `resource/images/` 图片资源加载支持
- [ ] 支持 `assets.zip` 资源包的加载骨架
- [ ] 运行时从 `resource/` 目录加载布局并正常显示
- [ ] 验证 `.gitignore`、`lib/` 与 `resource/` 保持版本控制一致性

### Week 3：布局与资源扩展

- [ ] 增强 `Panel` 布局属性，支持 `padding`、`spacing`、`align` 映射
- [ ] 增加 `Image` 组件与 `Font`/`Style` 基础属性
- [ ] 实现 `ZipResourceProvider` 并验证 `assets.zip` 加载
- [ ] 产出一套可运行的布局示例，覆盖多层嵌套与基础样式

### Week 4：工程化与后续预研

- [ ] 设计 `IRenderer` 接口，抽象现有 Direct2D 实现
- [ ] 准备 `lib/` 中的预编译 Yoga/Skia 引入示例
- [ ] 评估 Yoga 布局引擎与 Skia 渲染后端的最小切换点
- [ ] 更新文档为可复用开发规范与工程模板

---

## 五、最小 MVP 关键指标

- ✅ 可以用 `resource/layouts/ui.json` 或 `ui.xml` 构建界面
- ✅ 可以直接从 `resource/` 加载图片与布局
- ✅ 可以使用 `lib/` 预置二进制依赖
- ✅ UI 元素可以响应鼠标点击与 hover
- ✅ 系统有清晰的扩展路径，不需要先行实现 Skia 与 Yoga
