#include "ai_win_ui/host.h"
#include "ai_win_ui/observable.h"

#include <Windows.h>
#include <string>
#include <vector>
#include <memory>

static ai_win_ui::Observable<std::wstring> s_statusText(L"Waiting for binding...");
static ai_win_ui::Observable<std::wstring> s_countText(L"Clicks: 0");
static int s_clickCount = 0;

struct AppState {
    HINSTANCE instance = nullptr;
    std::vector<std::unique_ptr<ai_win_ui::Host>> secondaryHosts;
};

static void OnAction(const char* actionId, void* userData) {
    auto state = static_cast<AppState*>(userData);
    if (!actionId || !state) return;

    if (strcmp(actionId, "openBindingDemo") == 0) {
        s_clickCount = 0;
        s_statusText.Set(L"Binding established. Timer running...");
        s_countText.Set(L"Clicks: 0");

        ai_win_ui::HostCreateInfo secInfo{};
        secInfo.layoutPath = L"layouts/binding_demo.xml";
        secInfo.size = L"600x400";
        secInfo.ownMessagePump = false; // Use primary host's pump
        
        auto secHost = ai_win_ui::Host::Create(state->instance, secInfo);
        if (secHost) {
            secHost->BindText("lblStatus", &s_statusText);
            secHost->BindText("btnCount", &s_countText);
            state->secondaryHosts.push_back(std::move(secHost));
        }
    } else if (strcmp(actionId, "incrementCount") == 0) {
        s_clickCount++;
        s_countText.Set(L"Clicks: " + std::to_wstring(s_clickCount));
        s_statusText.Set(L"Button clicked at tick " + std::to_wstring(GetTickCount()));
    } else if (strcmp(actionId, "closeWindow") == 0) {
        // Find the host whose action triggered this, but since we can't easily 
        // identify it from just actionId, we'll just let the user close it via the 'X' button.
        // Or we could send a WM_CLOSE to the active window.
        HWND hwnd = GetForegroundWindow();
        if (hwnd) {
            PostMessageW(hwnd, WM_CLOSE, 0, 0);
        }
    }
}

// Thin executable entry — library body lives in ui_host.cpp (Wave3 H5b/H6).
int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int cmdShow) {
    AppState state;
    state.instance = instance;

    ai_win_ui::HostCreateInfo info{};
    info.showCommand = cmdShow;
    info.onAction = OnAction;
    info.actionUser = &state;

    // layout / chrome / size / renderer still come from AI_WIN_UI_* env (scripts, demos).
    auto host = ai_win_ui::Host::Create(instance, info);
    if (!host) {
        MessageBoxW(nullptr, L"Failed to initialize UiHost.", L"Error", MB_ICONERROR);
        return -1;
    }

    // Set a timer to periodically update the status if any secondary host is open.
    SetTimer(nullptr, 0, 1000, [](HWND, UINT, UINT_PTR, DWORD) {
        if (s_clickCount > 0) {
            s_statusText.Set(L"Tick " + std::to_wstring(GetTickCount()) + L", Clicks=" + std::to_wstring(s_clickCount));
        }
    });

    int result = host->Run();

    // Clean up
    state.secondaryHosts.clear();
    return result;
}
