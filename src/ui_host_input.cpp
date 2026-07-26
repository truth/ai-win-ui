#include "ui_host.h"
#include <Windows.h>

namespace ai_win_ui {

bool ProcessDevToolsInputKey(WPARAM wParam, bool& showDevToolsState) {
    if (wParam == VK_F12) {
        showDevToolsState = !showDevToolsState;
        return true;
    }
    return false;
}

} // namespace ai_win_ui
