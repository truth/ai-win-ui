#include "ai_win_ui/c_api.h"
#include "ai_win_ui/host.h"

extern "C" {

AI_WIN_UI_API AiHostHandle AIWinUI_CreateHost(HINSTANCE instance, const AiHostCreateInfo* info) {
    if (!info) {
        return nullptr;
    }

    ai_win_ui::HostCreateInfo cppInfo{};
    cppInfo.parent = info->parent;
    cppInfo.layoutPath = info->layoutPath;
    cppInfo.chrome = info->chrome;
    cppInfo.size = info->size;
    cppInfo.renderer = static_cast<ai_win_ui::Backend>(info->renderer);
    cppInfo.ownMessagePump = info->ownMessagePump;
    cppInfo.showCommand = info->showCommand;

    auto host = ai_win_ui::Host::Create(instance, cppInfo);
    return host.release();
}

AI_WIN_UI_API void AIWinUI_DestroyHost(AiHostHandle host) {
    if (host) {
        delete static_cast<ai_win_ui::Host*>(host);
    }
}

AI_WIN_UI_API HWND AIWinUI_GetHwnd(AiHostHandle host) {
    if (!host) return nullptr;
    return static_cast<ai_win_ui::Host*>(host)->GetHwnd();
}

AI_WIN_UI_API HWND AIWinUI_GetParentHwnd(AiHostHandle host) {
    if (!host) return nullptr;
    return static_cast<ai_win_ui::Host*>(host)->GetParentHwnd();
}

AI_WIN_UI_API bool AIWinUI_IsClosed(AiHostHandle host) {
    if (!host) return true;
    return static_cast<ai_win_ui::Host*>(host)->IsClosed();
}

AI_WIN_UI_API bool AIWinUI_IsEmbeddedChild(AiHostHandle host) {
    if (!host) return false;
    return static_cast<ai_win_ui::Host*>(host)->IsEmbeddedChild();
}

AI_WIN_UI_API int AIWinUI_Run(AiHostHandle host) {
    if (!host) return 0;
    return static_cast<ai_win_ui::Host*>(host)->Run();
}

AI_WIN_UI_API bool AIWinUI_ProcessMessage(AiHostHandle host, MSG* msg) {
    if (!host || !msg) return false;
    return static_cast<ai_win_ui::Host*>(host)->ProcessMessage(*msg);
}

AI_WIN_UI_API bool AIWinUI_ResizeClient(AiHostHandle host, int width, int height) {
    if (!host) return false;
    return static_cast<ai_win_ui::Host*>(host)->ResizeClient(width, height);
}

AI_WIN_UI_API bool AIWinUI_FitToParent(AiHostHandle host) {
    if (!host) return false;
    return static_cast<ai_win_ui::Host*>(host)->FitToParent();
}

AI_WIN_UI_API const char* AIWinUI_VersionString(void) {
    return ai_win_ui::Version();
}

AI_WIN_UI_API int AIWinUI_VersionNumber(void) {
    return ai_win_ui::VersionNumber();
}

AI_WIN_UI_API void AIWinUI_InitializeProcessDpiAwareness(void) {
    ai_win_ui::InitializeProcessDpiAwareness();
}

} // extern "C"
