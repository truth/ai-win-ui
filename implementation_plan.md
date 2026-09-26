# Implementation Plan: 数据绑定 (Host `BindText` & Observable)

## 1. 目标 (Goals)
实现响应式数据绑定机制，使得 C++ 宿主 (Embedder) 能够将 `Observable<std::wstring>` 绑定到布局文件中的指定元素（如 Label、Button）上。当数据变化时，UI 会自动更新。

## 2. 架构设计与详细步骤

### Step 1: 完善 Observable 生命周期管理
- 修改 `include/ai_win_ui/observable.h`：
  - 将 `Subscribe` 修改为返回一个 `size_t` (subscription ID)。
  - 新增 `void Unsubscribe(size_t id)`，允许按 ID 移除回调。
  - 这确保了当绑定宿主 (UiHost) 销毁时，可以安全地注销订阅，防止 Observable 触发指向已销毁宿主的悬空闭包（Dangling Callback）。

### Step 2: 扩展 Host API 以支持 BindText
- 修改 `include/ai_win_ui/host.h`，在 `Host` 类中新增：
  - `bool BindText(const char* name, Observable<std::wstring>* observable);`
- 修改内部实现 `src/ui_host.cpp` 中的 `UiHost` 类：
  - 新增 `std::vector<std::pair<Observable<std::wstring>*, size_t>> m_bindings;`。
  - 在 `BindText` 方法中，调用 `observable->Subscribe(...)`，回调内调用 `SetElementText(name, value)`。
  - 在 `~UiHost()` 析构函数中遍历 `m_bindings` 并调用 `Unsubscribe`，安全清理绑定。
  - 将 `Host::BindText` 路由到内部 `UiHost::BindText`。

### Step 3: 创建 binding_demo 测试用例
- **资源文件**：
  - 新增 `resource/layouts/binding_demo.xml`，其中包含 `<Label name="lblStatus" />` 和 `<Button name="btnCount" />`。
  - 在 `resource/layouts/demo_gallery.xml` 增加入口，配置 `onClick="action:openBindingDemo"`。
- **宿主逻辑 (main.cpp)**：
  - 在 `wWinMain` 初始化时，配置 `info.onAction` 拦截 `openBindingDemo`。
  - 拦截到动作后，使用 `Host::Create` 创建二级窗口，并将全局/静态的 `Observable<std::wstring>` 绑定到它的子节点上。
  - 启动一个系统 Timer（`SetTimer`），每秒或每点击一次就自增计数器更新 Observable，以此验证 UI 自动渲染机制。

### Step 4: 编译与自动化测试
- 使用 `scripts/run_ci_local.ps1 -SkipBuild` 进行完整的自动化测试。
- 确保新增 API 不影响现有的 7 项 CI 测试，并且新的 `binding_demo` 能正常编译和加载。
