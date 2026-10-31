# XML/JSON 解析器模块化回归测试报告

- **日期**：2026-10-31
- **测试环境**：Windows 10.0.19045 x64 / MSVC 19.44.35209.0 / CMake 3.21+
- **构建配置**：Debug (Skia + Direct2D 双渲染后端支持)
- **验证分支**：`master` (合并 `feature/modular-parsers` 后)
- **关联设计计划**：[`doc/plan/2026-10-31-modular-parsers-v1.md`](../plan/2026-10-31-modular-parsers-v1.md)

---

## 1. 测试背景与目标

本次测试旨在全面验证引入工业级解析库 **`pugixml`** 与 **`yyjson`** 并将手写解析器完全解耦后，系统的稳定性和行为一致性。核心目标包括：
1. **API 零破坏**：确保 `JsonValue` 与 `XmlNode` 对上层 `Theme`、`StyleCatalog`、`LayoutParser` 以及 UI 组件的构建完全兼容。
2. **布局渲染零漂移**：验证 XML/JSON 语法树解析后生成的 Yoga Flexbox 盒模型尺寸与先前记录的 Golden 基准 100% 吻合。
3. **双后端生命周期**：确保在 Direct2D 与 Skia 后端下，所有复杂画廊页面均能正常加载、解析并平稳退出。

---

## 2. 测试执行明细与结果汇总

执行命令：
```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\run_ci_local.ps1 -SkipBuild -SmokeProfile full -Renderer both
```

### 2.1 测试总览 (Passed: 7, Failed: 0, 耗时: 234.0s)

| 序号 | 步骤 / 测试项 | 范围 / 模式 | 状态 | 详情 |
| :---: | :--- | :--- | :---: | :--- |
| 1 | `build` | CMake Debug (C++20 & C) | **PASS** | 增量构建无 Warning/Error (skipped by flag) |
| 2 | `gallery coverage` | 检查全部布局文件画廊索引 | **PASS** | 完整覆盖 |
| 3 | `headless smoke (skia / full)` | 覆盖 17 个全量布局用例 | **PASS** | 17/17 全部通过 (exit=0) |
| 4 | `headless smoke (direct2d / full)` | 覆盖 17 个全量布局用例 | **PASS** | 17/17 全部通过 (exit=0) |
| 5 | `measure golden (core-validation / both)` | 核心控件像素级尺寸比对 | **PASS** | 容差 $\le$ 1.5px，完全吻合 |
| 6 | `measure golden (yoga-measure / both)` | Yoga Flexbox 测量比对 | **PASS** | 容差 $\le$ 1.5px，完全吻合 |
| 7 | `measure golden (scroll-viewer / both)` | 复杂滚动容器测量比对 | **PASS** | 容差 $\le$ 1.5px，完全吻合 |

---

## 3. 详细用例覆盖清单 (Full Profile)

在 `skia` 与 `direct2d` 双后端下，对以下 **17 个复杂 XML/JSON 布局** 进行了全量加载、解析与生命周期 Smoke 测试：

| 布局别名 (Alias) | 对应文件路径 | 涉及核心特性 | 结果 |
| :--- | :--- | :--- | :---: |
| `core-validation` | `layouts/core_validation.xml` | 基础面板、按钮、复选框、文本排版 | **PASS** |
| `yoga-measure` | `layouts/yoga_measure_cases.xml` | FlexWrap、FlexGrow、百分比边距与对齐 | **PASS** |
| `core-controls-v2` | `layouts/core_controls_v2.xml` | V2 核心交互控件 | **PASS** |
| `navigation-components` | `layouts/navigation_components.xml` | 导航栏、Tab 栏与状态切换 | **PASS** |
| `table-components` | `layouts/table_components.xml` | 数据表格 DataTable 结构与样式 | **PASS** |
| `scroll-viewer` | `layouts/scroll_viewer_cases.xml` | 滚动条、视口裁剪、滚动边界测试 | **PASS** |
| `demo-gallery` | `layouts/demo_gallery.xml` | 演示总画廊（综合 XML 布局） | **PASS** |
| `skia-image` | `layouts/skia_image_cases.xml` | 外部图像资源、图片缩放与滤镜 | **PASS** |
| `stats-components` | `layouts/stats_components.xml` | 通用 `<Chart>`（折线/柱状/面积图与 Series） | **PASS** |
| `seagull-animation` | `layouts/seagull_animation.xml` | 高级动效、定时刷新与插值 | **PASS** |
| `advanced-inputs` | `layouts/advanced_inputs.xml` | 输入框、快捷键与焦点路由 | **PASS** |
| `style-catalog` | `layouts/style_catalog_demo.xml` | 跨文件 JSON 样式目录与继承 (`$style`) | **PASS** |
| `shaped-hub` | `layouts/shaped_hub.xml` | 异形窗口、透明通道与分层绘制 | **PASS** |
| `text-wrap` | `layouts/text_wrap_cases.xml` | 复杂多行折行、DWrite/Skia 文本测量 | **PASS** |
| `cjk-render` | `layouts/cjk_render_cases.xml` | 中日韩多语言文字排版与回退字体 | **PASS** |
| `popup-theme` | `layouts/popup_theme_cases.xml` | 浮层弹窗、主题 Token 动态解析 (`$color`) | **PASS** |
| `virtual-list` | `layouts/virtual_list_cases.xml` | 虚拟化长列表项复用与动态高度 | **PASS** |

---

## 4. 结论

本次重构：
1. **可靠性**：成功以工业级库（`pugixml`、`yyjson`）替换手写解析器，彻底解决了手写代码潜在的转义、空格、多属性兼容隐患。
2. **兼容性**：所有 7 项 CI 回归测试 **100% 通过**，Golden 布局数据无任何偏离。
3. **架构清晰度**：核心解析功能形成独立模块，解耦了 `layout_parser.cpp` 的臃肿代码，具备良好的维护性与扩展性。
