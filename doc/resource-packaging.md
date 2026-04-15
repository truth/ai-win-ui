# 资源与布局定义方案

本方案说明如何使用 XML/JSON 定义 UI 布局，并将资源打包成 ZIP 供程序使用，同时保留 `resource` 目录模式的兼容性。

## 1. 目标

- 使用数据驱动方式定义 UI 布局。
- 支持 XML 或 JSON 两种布局描述格式。
- 支持将布局文件、图像、字体、样式、文案等资源打包到 ZIP。
- 支持运行时从 `resource/` 目录加载资源，便于开发调试。
- 依赖包存放在 `lib/` 目录，程序运行时资源存放在 `resource/` 目录。

## 2. 布局定义格式

### 2.1 JSON 示例

```json
{
  "type": "Panel",
  "props": {
    "padding": [24, 24, 24, 24],
    "spacing": 10,
    "background": "#171717"
  },
  "children": [
    {
      "type": "Label",
      "props": {
        "text": "DirectUI-style Retained UI Tree",
        "fontSize": 18,
        "color": "#EDEDED"
      }
    },
    {
      "type": "Button",
      "props": {
        "text": "Primary Action"
      },
      "events": {
        "onClick": "primaryAction"
      }
    }
  ]
}
```

### 2.2 XML 示例

```xml
<Panel padding="24,24,24,24" spacing="10" background="#171717">
  <Label text="DirectUI-style Retained UI Tree" fontSize="18" color="#EDEDED" />
  <Button text="Primary Action" onClick="primaryAction" />
</Panel>
```

## 3. 资源打包与加载策略

### 3.1 ZIP 打包模式

- 程序发布时，将布局描述文件和静态资源打包到单个 `assets.zip` 中。
- ZIP 内容示例：
  - `layouts/ui_layout.json`
  - `layouts/ui_layout.xml`
  - `images/icon.png`
  - `fonts/SegoeUI.ttf`
  - `styles/theme.json`

### 3.2 运行时加载逻辑

- 首先尝试从程序目录下的 `assets.zip` 加载资源。
- 如果 `assets.zip` 不存在，则退回到 `resource/` 目录模式：
  - `resource/layouts/`
  - `resource/images/`
  - `resource/fonts/`

### 3.3 资源提供器抽象

建议定义统一资源访问接口：

```cpp
class IResourceProvider {
public:
    virtual ~IResourceProvider() = default;
    virtual bool Exists(const std::string& path) = 0;
    virtual std::vector<uint8_t> LoadBytes(const std::string& path) = 0;
    virtual std::string LoadText(const std::string& path) = 0;
};
```

实现类型：

- `ZipResourceProvider`
- `DirectoryResourceProvider`

## 4. 布局解析器

### 4.1 通用解析流程

1. 从 `IResourceProvider` 读取文件内容。
2. 识别文件类型（JSON / XML）。
3. 解析成通用布局节点模型。
4. 将布局节点转换为 `UIElement` 树。

### 4.2 解析器建议

- JSON：使用 `RapidJSON`、`nlohmann/json` 或 `simdjson`。
- XML：使用 `tinyxml2`。

### 4.3 布局节点模型

推荐节点结构：

- `type`：组件类型（Panel、Label、Button、Image 等）
- `props`：布局与外观属性
- `events`：事件绑定名称
- `children`：子节点列表

## 5. 与现有 UI 系统整合

### 5.1 执行流程

- 程序启动时初始化资源提供器：
  - 如果 `assets.zip` 可用，则使用 `ZipResourceProvider`。
  - 否则使用 `DirectoryResourceProvider("resource")`。
- 依赖库仍从 `lib/` 目录加载，保证运行时可直接引用预编译库。
- 读取布局文件并创建 UI 树。
- 事件回调映射到注册函数，例如 `primaryAction`。

### 5.2 兼容现有代码

现有 `src/ui.h` 的 `UIElement`、`Panel`、`Label`、`Button` 结构可以保持不变。
只需新增：

- `LayoutParser` 或 `UIBuilder`
- 资源路径解析与加载层
- 布局属性映射到 `UIElement` 成员

## 6. ZIP 资源打包工具

### 6.1 开发阶段

- 在工程目录下创建 `resource/` 子目录。
- 运行打包脚本将目录内容压缩到 `assets.zip`。
- 发布版使用 `assets.zip`，调试版可直接使用目录。

### 6.2 可选实现

- 使用 `miniz`、`libzip` 或 `zlib` 等库。
- 也可借助外部构建工具（Python、PowerShell）生成 ZIP 文件。

## 7. 运行时资源路径优先级

1. `assets.zip` 中的资源
2. `resource/` 目录中的资源
3. 打包后嵌入到可执行文件的资源（可选扩展）

## 8. 可扩展性

- 未来可支持 `remote://` 资源提供器，方便从网络加载布局与素材。
- 资源版本控制可通过 `manifest.json` 扩展，使 ZIP 内资源按版本管理。
- 布局定义可进一步支持主题变量、样式继承、动态绑定等高级特性。
