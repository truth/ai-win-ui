# Wave5 响应式数据绑定机制 (2026-09-26)

本文档归档了 Wave5 执行计划中，关于 `Host::BindText` 与 `Observable` 数据绑定机制的设计与实现。

---

## 1. 痛点与目标

在典型的 DirectUI / 声明式 UI 架构中，UI 线程负责渲染视图树，而业务逻辑往往由宿主（Embedder）驱动。我们需要一种机制，能够让 C++ 后端的数据状态改变时，自动将新值同步到 UI 节点（如 `Label`、`Button`），而不需要手动通过查找名称再调用 `SetElementText` 的样板代码。

目标是：
1. **类型安全**：基于 `Observable<std::wstring>`。
2. **内存安全**：保证 UI 窗口销毁时自动清理绑定回调，防止悬空指针崩溃。
3. **极简 API**：宿主仅需调用 `host->BindText("elementName", &observable);`。

---

## 2. 架构改造

### 2.1 Observable 支持 ID 取消订阅
原始的 `Observable<T>` 在 Wave 4 引入时仅支持无限制的追加订阅。本次我们将其内部的监听器存储结构改为 `std::vector<std::pair<size_t, Listener>>`，并在 `Subscribe` 时分配递增的 ID，同时新增了 `Unsubscribe(size_t id)` 以支持精准取消订阅。在 `Notify` 触发时，采用安全拷贝的方式遍历，允许回调内部触发解绑。

### 2.2 UiHost 生命周期管理
`UiHost`（作为窗口生存期的实际承载者）新增了 `m_bindings` 容器。
当调用 `BindText` 时，会执行一次立即同步（获取当前 Observable 值并更新），然后注册一个基于闭包的订阅，闭包内部调用 `SetElementText`。
在 `UiHost` 析构阶段，会主动遍历 `m_bindings` 并对关联的 Observable 调用 `Unsubscribe`，确保闭包不会在此窗口关闭后被触发。

### 2.3 演示应用架构
我们在 `main.cpp` 中拓展了传统的 `HostCreateInfo::onAction` 拦截器。当 Gallery 触发 `action:openBindingDemo` 时，主循环不依赖 `OpenSecondaryHost` 内部助手，而是主动创建 `Host` 对象并持有，从而获得 `Host*` 指针来实施 `BindText` 数据绑定演示，证明了 C++ Embedder 端控制窗口及绑定的完整流程。
