# XML/JSON 解析模块化重构 (pugixml & yyjson) 设计归档 (2026-10-31)

分支：`feature/modular-parsers`

## 背景
原有 XML/JSON 解析代码为手写递归下降解析（Ad-hoc parser），约 300+ 行直接内联在 `layout_parser.cpp` 匿名命名空间中，存在如下问题：
1. **性能与内存瓶颈**：原手写代码使用 `std::map<std::string, JsonValue>` 与递归动态构建，吞吐在数十 MB/s 级别，内存分配零散；
2. **规范兼容性缺陷**：XML 缺少 CDATA、实体转义、多属性与标准结构校验；JSON 缺少完整的 Unicode 与科学计数法转义；
3. **架构强耦合**：UI 布局构建代码与底层文本 Lexer/Parser 混合在 4200+ 行的单文件中，职责不清晰。

## 目标
1. 剥离底层手写解析器，建立独立子模块；
2. 引入经过工业级生产验证的单源文件高性能轻量库（`pugixml` 与 `yyjson`）；
3. 严格保持原有 `JsonValue` 和 `XmlNode` API 兼容性，确保业务无感迁移。

## 设计决策与架构

| 决策项 | 方案 | 优势 |
| :--- | :--- | :--- |
| **JSON 解析器** | 引入 `yyjson` (`yyjson.h` + `yyjson.c`) | 现代 C 语言 SIMD 深度优化，吞吐可达数 GB/s，轻量且零外部依赖 |
| **XML 解析器** | 引入 `pugixml` (`pugixml.hpp` + `pugixml.cpp`) | 业界标准 DOM 解析库，完整 XML 规范支持（实体/CDATA/注释），吞吐数百 MB/s |
| **模块封装** | `src/json_parser.h/.cpp` 与 `src/xml_parser.h/.cpp` | 解析逻辑独立封装，与 UI 布局逻辑彻底解耦 |
| **API 兼容策略** | `layout_parser.h` 包含 `json_parser.h` 与 `xml_parser.h` | 维持原有 `JsonValue` / `XmlNode` 接口与操作符重载完全不变 |

## 实施步骤
1. **代码库引入**：将 `pugixml` 与 `yyjson` 单源文件放置于 `third_party/pugixml/` 与 `third_party/yyjson/`。
2. **CMake 构建工程适配**：
   - `CMakeLists.txt` 中启用 `C` 语言支持（`LANGUAGES C CXX`）；
   - 将 `yyjson.c` 与 `pugixml.cpp` 添加至核心静态库 `ai_win_ui_lib`。
3. **独立模块实现**：
   - `JsonParser::Parse`：基于 `yyjson_read_opts` 快速构建 `JsonValue`；
   - `XmlParser::Parse`：基于 `pugi::xml_document` 解析完整 DOM 并构建 `XmlNode`。
4. **主解析器瘦身**：
   - 彻底删除 `layout_parser.cpp` 中 400 余行手写解析函数（`SkipWhitespace`、`ParseJsonObject`、`ParseXml` 等）；
   - `LayoutParser::ParseJson` 和 `LayoutParser::ParseXml` 干净委托至新子模块。

## 验证
- `.\build.ps1`：MSVC C++20 + C 编译完全通过，0 Error / 0 Warning。
- `.\scripts\run_ci_local.ps1 -SkipBuild`：本地 7 项 CI 回归测试 **100% PASS**（包含 Gallery 覆盖率、Skia 与 Direct2D 无头 Smoke 测试、Core/Yoga/ScrollViewer Golden 测量）。
