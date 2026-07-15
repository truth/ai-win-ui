#include "application.h"

#include <algorithm>

namespace ai_win_ui {
namespace {

int g_liveHostCount = 0;
std::vector<void*> g_secondaryHosts;
OpenHostOptions g_pendingOpen{};
bool g_hasPendingOpen = false;

} // namespace

int Application::LiveHostCount() {
    return g_liveHostCount;
}

void Application::OnHostCreated() {
    ++g_liveHostCount;
}

void Application::OnHostDestroyed() {
    if (g_liveHostCount > 0) {
        --g_liveHostCount;
    }
}

void Application::RegisterSecondary(void* host) {
    if (host) {
        g_secondaryHosts.push_back(host);
    }
}

void Application::UnregisterSecondaryIf(void* host) {
    g_secondaryHosts.erase(
        std::remove(g_secondaryHosts.begin(), g_secondaryHosts.end(), host),
        g_secondaryHosts.end());
}

void Application::PurgeClosedSecondaries(bool (*isClosed)(void*), void (*destroyHost)(void*)) {
    if (!isClosed || !destroyHost) {
        return;
    }
    for (auto it = g_secondaryHosts.begin(); it != g_secondaryHosts.end();) {
        void* host = *it;
        if (host && isClosed(host)) {
            destroyHost(host);
            it = g_secondaryHosts.erase(it);
        } else {
            ++it;
        }
    }
}

OpenHostOptions& Application::PendingOpen() {
    return g_pendingOpen;
}

bool& Application::HasPendingOpen() {
    return g_hasPendingOpen;
}

} // namespace ai_win_ui
