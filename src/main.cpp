#include "ai_win_ui/host.h"

#include <Windows.h>

// Thin executable entry — library body lives in ui_host.cpp (Wave3 H5b/H6).
int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int cmdShow) {
    ai_win_ui::HostCreateInfo info{};
    info.showCommand = cmdShow;
    // layout / chrome / size / renderer still come from AI_WIN_UI_* env (scripts, demos).
    auto host = ai_win_ui::Host::Create(instance, info);
    if (!host) {
        MessageBoxW(nullptr, L"Failed to initialize UiHost.", L"Error", MB_ICONERROR);
        return -1;
    }
    return host->Run();
}
