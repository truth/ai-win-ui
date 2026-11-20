#pragma once

#include "react_bridge_iface.h"
#include "react_js_runtime.h"
#include "react_node_registry.h"

#include <atomic>
#include <chrono>
#include <filesystem>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace ai_win_ui::react {

// Registers the React bridge factory with UiHost (call once at app startup).
void RegisterWithHost();

class ReactHostBridge final : public IReactBridge {
public:
    ReactHostBridge(
        IReactHostSurface& surface,
        std::string bundlePath,
        std::string inlineBundleUtf8,
        bool watchMode);
    ~ReactHostBridge() override;

    void OnCommitMessage() override;
    void OnHostDestroyed() override;
    bool ReloadBundle() override;
    void TickHotReload() override;
    bool OnKeyDown(WPARAM keyCode, bool isTextInputFocused) override;

private:
    bool LoadBundleContent(std::string& outScriptUtf8, std::string& outSourceName, bool& outIsFile, std::filesystem::file_time_type& outMtime) const;
    void EnqueueCommitFromWorker(MutationBatch&& batch);

    IReactHostSurface& m_surface;
    std::string m_bundlePath;
    std::string m_inlineBundleUtf8;
    bool m_watchMode = false;
    bool m_isFileBacked = false;
    std::filesystem::file_time_type m_lastWriteTime{};
    std::atomic<bool> m_destroyed{false};

    std::mutex m_commitMutex;
    std::vector<MutationBatch> m_commitQueue;

    std::unique_ptr<ReactNodeRegistry> m_registry;
    std::unique_ptr<ReactJsRuntime> m_jsRuntime;
};

} // namespace ai_win_ui::react

