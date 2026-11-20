#include "react_host_bridge.h"

#include "application.h"
#include "frame_profiler.h"
#include "resource_provider.h"

#include <fstream>
#include <sstream>

namespace ai_win_ui::react {

namespace {

std::unique_ptr<IReactBridge> CreateReactBridge(
    IReactHostSurface& surface,
    const std::string& bundlePath,
    bool watchMode) {
    return std::make_unique<ReactHostBridge>(surface, bundlePath, watchMode);
}

std::string ReadAllFileBytes(const std::filesystem::path& p) {
    std::ifstream ifs(p, std::ios::binary);
    if (!ifs.is_open()) return {};
    std::ostringstream oss;
    oss << ifs.rdbuf();
    return oss.str();
}

} // namespace

void RegisterWithHost() {
    ai_win_ui::RegisterReactBridgeFactory(&CreateReactBridge);
}

ReactHostBridge::ReactHostBridge(IReactHostSurface& surface, std::string bundlePath, bool watchMode)
    : m_surface(surface), m_bundlePath(std::move(bundlePath)), m_watchMode(watchMode) {
    m_registry = std::make_unique<ReactNodeRegistry>(m_surface, [this](UiEvent&& ev) {
        if (m_jsRuntime && !m_destroyed.load(std::memory_order_acquire)) {
            m_jsRuntime->PostUiEvent(std::move(ev));
        }
    });

    m_jsRuntime = std::make_unique<ReactJsRuntime>([this](MutationBatch&& batch) {
        EnqueueCommitFromWorker(std::move(batch));
    });

    std::string script;
    std::string srcName;
    if (!LoadBundleContent(script, srcName, m_isFileBacked, m_lastWriteTime)) {
        OutputDebugStringA(("[ReactHostBridge] Failed to load bundle: " + m_bundlePath + "\n").c_str());
        script = "console.error('Missing React bundle');";
        srcName = m_bundlePath;
    }

    m_jsRuntime->Start(std::move(script), std::move(srcName));
}

ReactHostBridge::~ReactHostBridge() {
    OnHostDestroyed();
}

bool ReactHostBridge::LoadBundleContent(
    std::string& outScriptUtf8,
    std::string& outSourceName,
    bool& outIsFile,
    std::filesystem::file_time_type& outMtime) const {
    outIsFile = false;
    std::error_code ec;
    const std::filesystem::path direct(m_bundlePath);
    if (std::filesystem::exists(direct, ec) && std::filesystem::is_regular_file(direct, ec)) {
        outScriptUtf8 = ReadAllFileBytes(direct);
        if (!outScriptUtf8.empty()) {
            outSourceName = direct.string();
            outIsFile = true;
            outMtime = std::filesystem::last_write_time(direct, ec);
            return true;
        }
    }

    // Try relative to resource/ directory on disk so --watch finds repo resource updates directly
    const std::filesystem::path underResource = std::filesystem::path("resource") / m_bundlePath;
    if (std::filesystem::exists(underResource, ec) && std::filesystem::is_regular_file(underResource, ec)) {
        outScriptUtf8 = ReadAllFileBytes(underResource);
        if (!outScriptUtf8.empty()) {
            outSourceName = underResource.string();
            outIsFile = true;
            outMtime = std::filesystem::last_write_time(underResource, ec);
            return true;
        }
    }

    if (IResourceProvider* provider = m_surface.GetResourceProvider()) {
        if (provider->Exists(m_bundlePath)) {
            outScriptUtf8 = provider->LoadText(m_bundlePath);
            if (!outScriptUtf8.empty()) {
                outSourceName = m_bundlePath;
                return true;
            }
        }
    }
    return false;
}

void ReactHostBridge::EnqueueCommitFromWorker(MutationBatch&& batch) {
    if (m_destroyed.load(std::memory_order_acquire)) {
        return;
    }
    bool needNotify = false;
    {
        std::lock_guard<std::mutex> lock(m_commitMutex);
        needNotify = m_commitQueue.empty();
        m_commitQueue.push_back(std::move(batch));
    }
    if (needNotify) {
        if (HWND hwnd = m_surface.GetHwnd()) {
            PostMessageW(hwnd, ai_win_ui::kMsgReactCommit, 0, 0);
        }
    }
}

void ReactHostBridge::OnCommitMessage() {
    if (m_destroyed.load(std::memory_order_acquire) || !m_registry) {
        return;
    }
    std::vector<MutationBatch> batches;
    {
        std::lock_guard<std::mutex> lock(m_commitMutex);
        if (m_commitQueue.empty()) {
            return;
        }
        batches.swap(m_commitQueue);
    }
    for (auto& batch : batches) {
        const char* trigger = batch.commitTrigger;
        const double jsMs = batch.jsExecMs;
        const double originMs = batch.originEventMs;
        const double applyBegin = ai_win_ui::FrameProfiler::NowMs();
        m_registry->ApplyBatch(std::move(batch));
        const double applyMs = std::max(0.0, ai_win_ui::FrameProfiler::NowMs() - applyBegin);
        m_surface.RecordReactCommitTiming(trigger, jsMs, applyMs, originMs);
    }
}

void ReactHostBridge::OnHostDestroyed() {
    if (m_destroyed.exchange(true, std::memory_order_acq_rel)) {
        return;
    }
    if (m_jsRuntime) {
        m_jsRuntime->Stop();
        m_jsRuntime.reset();
    }
    {
        std::lock_guard<std::mutex> lock(m_commitMutex);
        m_commitQueue.clear();
    }
    if (m_registry) {
        m_registry.reset();
    }
}

bool ReactHostBridge::ReloadBundle() {
    if (m_destroyed.load(std::memory_order_acquire) || !m_jsRuntime) {
        return false;
    }
    std::string script;
    std::string srcName;
    bool isFile = false;
    std::filesystem::file_time_type mtime{};
    if (!LoadBundleContent(script, srcName, isFile, mtime)) {
        return false;
    }
    m_isFileBacked = isFile;
    m_lastWriteTime = mtime;
    m_jsRuntime->RequestReload(std::move(script), std::move(srcName));
    return true;
}

void ReactHostBridge::TickHotReload() {
    if (!m_watchMode || !m_isFileBacked || m_destroyed.load(std::memory_order_acquire)) {
        return;
    }
    std::error_code ec;
    std::filesystem::path target(m_bundlePath);
    if (!std::filesystem::exists(target, ec)) {
        target = std::filesystem::path("resource") / m_bundlePath;
    }
    if (!std::filesystem::exists(target, ec)) {
        return;
    }
    const auto mtime = std::filesystem::last_write_time(target, ec);
    if (!ec && mtime != m_lastWriteTime) {
        OutputDebugStringA(("[ReactHostBridge] Hot-reloading bundle: " + target.string() + "\n").c_str());
        ReloadBundle();
    }
}

bool ReactHostBridge::OnKeyDown(WPARAM keyCode, bool isTextInputFocused) {
    if (m_destroyed.load(std::memory_order_acquire) || !m_jsRuntime) {
        return false;
    }
    if (isTextInputFocused && keyCode != VK_F5 && keyCode != VK_ESCAPE) {
        return false;
    }
    UiEvent ev;
    ev.nodeId = 0;
    ev.type = EventTypeId::KeyDown;
    ev.intVal = static_cast<int32_t>(keyCode);
    ev.boolVal = isTextInputFocused;
    ev.timestampMs = ai_win_ui::FrameProfiler::NowMs();
    m_jsRuntime->PostUiEvent(std::move(ev));
    return false;
}

} // namespace ai_win_ui::react

