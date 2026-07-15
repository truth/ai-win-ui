#pragma once

#include "host_options.h"

#include <Windows.h>
#include <vector>

// Process-wide multi-host registry (Wave1 H1).
// Each top-level UI window is one UiHost (see ui_host.h / main.cpp);
// last destroy posts quit. This module owns shared process state and message ids.
namespace ai_win_ui {

constexpr UINT kMsgOpenDemoHost = WM_APP + 40;

class Application {
public:
    static int LiveHostCount();
    static void OnHostCreated();
    static void OnHostDestroyed(); // returns whether process should quit via PostQuitMessage

    static void RegisterSecondary(void* host);
    static void UnregisterSecondaryIf(void* host);
    static void PurgeClosedSecondaries(bool (*isClosed)(void*), void (*destroyHost)(void*));

    // Deferred open payload (filled by hub, consumed on kMsgOpenDemoHost).
    static OpenHostOptions& PendingOpen();
    static bool& HasPendingOpen();
};

} // namespace ai_win_ui
