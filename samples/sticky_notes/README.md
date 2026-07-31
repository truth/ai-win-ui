# 便签 (sticky_notes)

一个基于 `ai_win_ui::Host` 内联布局 API 构建的桌面便签示例应用：托盘常驻、多个无边框浮动便签窗口、一个带搜索/悬浮预览的管理器窗口。用来验证 Host API（`inlineLayoutXml`、`onAction`、`GetElementText`/`SetElementText`、窗口定位）在真实多窗口场景下够不够用。

## 功能特性

- 系统托盘图标 + 右键菜单（新建 / 便签管理器 / 显示全部便签 / 退出）
- 浮动便签窗口：`chrome="layered"` 无边框卡片，标题栏可拖拽移动，位置随窗口关闭持久化
- 置顶钉在桌面（📌，每个便签独立开关）
- 7 种背景配色（柠檬黄 / 暖橙 / 樱粉 / 薄荷 / 晴空 / 薰衣草 / 石墨）
- 管理器窗口：便签列表、按标题/正文搜索、悬浮某一行弹出内容预览
- 数据持久化在 `%APPDATA%\ai-win-ui-notes\notes.json`（与具体 exe 副本无关，Debug/Release/发布包共享同一份数据）

## 构建 & 运行

```powershell
cmake --build build --config Debug --target sticky_notes
build\Debug\sticky_notes.exe
```

Release 同理，把 `Debug` 换成 `Release`。CMake 会在 `AI_WIN_UI_BUILD_SAMPLES=ON`（默认开）时把 `sticky_notes` 一并加入构建。

## 发布打包

```powershell
.\scripts\package_sticky_notes.ps1                                   # 默认 Release
.\scripts\package_sticky_notes.ps1 -Configuration Release -OutDir dist\sticky_notes
.\scripts\package_sticky_notes.ps1 -SkipBuild                         # 只重新打包资源，不重新编译
```

产出 `dist\sticky_notes\`，只含两个文件：

- `sticky_notes.exe`
- `assets.zip`（`resource/icons/` + `resource/styles/` 打包，运行时会被 `ZipResourceProvider` 优先加载，缺失的条目再回退到旁边的 `resource/` 目录）

**注意**：`ZipResourceProvider`（`src/zip_resource_provider.cpp`）是一个精简实现，只认识 ZIP 的 `Stored`（不压缩）条目。Windows PowerShell 的 `Compress-Archive`／`System.IO.Compression.ZipArchive` 即使指定 `-CompressionLevel NoCompression` 实际仍写 `Deflate`，会被静默读成空数据（图标消失但不报错）。所以打包脚本没有用这两个 API，而是手写了一个只输出 Stored 条目的最小 ZIP writer（含手算 CRC32）。如果要改資源打包逻辑，保留这个限制或者先给 `ZipResourceProvider` 加上真正的 inflate 支持（`CMakeLists.txt` 里已经为 local-sdk Skia 场景准备了到 `Cr_z_inflate` 的 `/alternatename` 链接，但 CI 的 Direct2D-only 构建没有这个符号，需要额外做条件编译）。

如果改了 `main.cpp` 里 `CreateNoteIcon`/图标画法，用下面脚本重新生成 `.ico`：

```powershell
.\scripts\gen_sticky_notes_icon.ps1
```

会在 16/32/48/256 四个尺寸下重绘同款折角便签图案，写回 `samples/sticky_notes/app.ico`（`app.rc` 把它编译进 exe，Explorer/任务栏/Alt+Tab 显示的图标都来自这里，不再是运行时画的图标）。

## 代码结构

| 文件 | 作用 |
|------|------|
| `main.cpp` | `App` 类：托盘、窗口生命周期、事件分发（`onAction` 回调）、菜单/快捷操作 |
| `layout_gen.h/.cpp` | 生成便签/管理器/预览窗口的内联布局 XML 字符串（`NoteLayoutXml` / `ManagerLayoutXml` / `PreviewLayoutXml`） |
| `note_store.h/.cpp` | `Note` 数据模型、7 色调色板 `Palette`、JSON 读写（`%APPDATA%\ai-win-ui-notes\notes.json`） |
| `resource.h` / `app.rc` / `app.ico` | 编译进 exe 的应用图标资源 |

## 已知限制

- 便签窗口打开期间的实时编辑内容只有在关闭/切换置顶/换色/退出时才会同步回 `NoteStore`；`ShowPreviewFor`/`RefreshManager` 在读取之前会主动同步一次对应的打开窗口，但如果后续新增读取路径，记得同样处理，否则会看到"标题/正文明明有内容，别处却显示空白"的过期数据问题。
- `notes.json` 路径固定为 `%APPDATA%\ai-win-ui-notes\notes.json`，不区分 Debug/Release/打包目录，多份 exe 副本会共享同一份数据。
