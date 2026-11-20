#pragma once

#include "react_protocol.h"

#include <condition_variable>
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

struct JSRuntime;
struct JSContext;

namespace ai_win_ui::react {

// Owns QuickJS-ng JSRuntime/JSContext exclusively on a dedicated worker thread.
// Emits immutable MutationBatch objects to the CommitSink and receives UiEvents.
class ReactJsRuntime {
public:
    using CommitSink = std::function<void(MutationBatch&&)>;

    explicit ReactJsRuntime(CommitSink commitSink);
    ~ReactJsRuntime();

    ReactJsRuntime(const ReactJsRuntime&) = delete;
    ReactJsRuntime& operator=(const ReactJsRuntime&) = delete;

    // Starts the JS worker thread with initial UTF-8 JS bundle text.
    void Start(std::string initialBundleUtf8, std::string sourceName = "bundle.js");
    void Stop();

    void PostUiEvent(UiEvent&& ev);
    void RequestReload(std::string newBundleUtf8, std::string sourceName = "bundle.js");

    // Synchronous test helper (no worker thread) for deterministic unit tests.
    bool InitSynchronouslyForTest();
    bool EvalSynchronouslyForTest(const std::string& scriptUtf8, const std::string& sourceName = "test.js");
    bool DispatchEventSynchronouslyForTest(const UiEvent& ev);
    bool StepTimersAndJobsSynchronouslyForTest(int64_t advanceMs = 0);
    void ShutdownSynchronouslyForTest();

private:
    struct TimerEntry {
        uint32_t id = 0;
        int64_t dueMs = 0;
        int64_t intervalMs = 0;
        bool isInterval = false;
        bool cancelled = false;
        // Stored as uint64-sized opaque slot or JSValue inside cpp implementation.
        struct Impl;
        Impl* impl = nullptr;
    };

    void WorkerMain(std::string initialBundle, std::string sourceName);
    bool InitQuickJs();
    void DestroyQuickJs();
    void RegisterNativeBindings();
    void RegisterTimersAndGlobals();
    bool EvalScript(const std::string& code, const std::string& filename);
    void DrainMicrotasks();
    void DispatchEventToJs(const UiEvent& ev);
    void FireDueTimers();
    int64_t NextTimerDelayMs() const;
    int64_t NowSteadyMs() const;

    static void LogError(JSContext* ctx, const char* prefix);

    CommitSink m_commitSink;
    MutationBatch m_pendingBatch;
    bool m_inBatch = false;

    JSRuntime* m_rt = nullptr;
    JSContext* m_ctx = nullptr;

    std::vector<TimerEntry> m_timers;
    uint32_t m_nextTimerId = 1;
    int64_t m_testClockOffsetMs = 0;

    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::vector<UiEvent> m_eventQueue;
    bool m_stopRequested = false;
    bool m_hasPendingReload = false;
    std::string m_reloadBundle;
    std::string m_reloadSourceName;

    std::thread m_workerThread;
    bool m_started = false;

public:
    // Native binding callbacks access these on JS thread:
    void NativeBeginBatch();
    void NativePushOp(MutationOp&& op);
    void NativeEndBatch(bool needsLayout);
    uint32_t AddTimerFromJs(JSContext* ctx, void* jsFuncPtr, int64_t delayMs, bool isInterval);
    void CancelTimerFromJs(uint32_t timerId);
    int64_t CurrentClockMs() const { return NowSteadyMs(); }
};

} // namespace ai_win_ui::react

