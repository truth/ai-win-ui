#pragma once

// Pure C-ABI Export API for ai-win-ui (Wave4/Phase 6 Interop).
// Compatible with C# P/Invoke, Rust (ai-win-ui-sys), Python, Electron, Flutter.

#include <Windows.h>
#include <stdbool.h>
#include <stdint.h>

#if defined(_WIN32)
  #if defined(AI_WIN_UI_BUILD_DLL)
    #define AI_WIN_UI_API __declspec(dllexport)
  #else
    #define AI_WIN_UI_API
  #endif
#else
  #define AI_WIN_UI_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef void* AiHostHandle;

typedef enum AiBackend {
    AI_BACKEND_DEFAULT = 0,
    AI_BACKEND_DIRECT2D = 1,
    AI_BACKEND_SKIA = 2
} AiBackend;

typedef struct AiHostCreateInfo {
    HWND parent;
    const wchar_t* layoutPath;
    const wchar_t* chrome;
    const wchar_t* size;
    AiBackend renderer;
    bool ownMessagePump;
    int showCommand;
} AiHostCreateInfo;

AI_WIN_UI_API AiHostHandle AIWinUI_CreateHost(HINSTANCE instance, const AiHostCreateInfo* info);
AI_WIN_UI_API void AIWinUI_DestroyHost(AiHostHandle host);

AI_WIN_UI_API HWND AIWinUI_GetHwnd(AiHostHandle host);
AI_WIN_UI_API HWND AIWinUI_GetParentHwnd(AiHostHandle host);
AI_WIN_UI_API bool AIWinUI_IsClosed(AiHostHandle host);
AI_WIN_UI_API bool AIWinUI_IsEmbeddedChild(AiHostHandle host);

AI_WIN_UI_API int AIWinUI_Run(AiHostHandle host);
AI_WIN_UI_API bool AIWinUI_ProcessMessage(AiHostHandle host, MSG* msg);
AI_WIN_UI_API bool AIWinUI_ResizeClient(AiHostHandle host, int width, int height);
AI_WIN_UI_API bool AIWinUI_FitToParent(AiHostHandle host);

AI_WIN_UI_API const char* AIWinUI_VersionString(void);
AI_WIN_UI_API int AIWinUI_VersionNumber(void);
AI_WIN_UI_API void AIWinUI_InitializeProcessDpiAwareness(void);

#ifdef __cplusplus
}
#endif
