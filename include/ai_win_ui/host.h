#pragma once

// Public embed surface (Wave3 H5/H6). Implementation: src/ui_host.cpp.
//
//   #include <ai_win_ui/host.h>
//   auto host = ai_win_ui::Host::Create(hInstance, info);
//   return host->Run();
//
// Child embed (parent HWND):
//   info.parent = shellHwnd;
//   info.ownMessagePump = false;
//   // on WM_SIZE: host->FitToParent();

#include "ai_win_ui/version.h"

#include <Windows.h>
#include <memory>

namespace ai_win_ui {

enum class Backend {
    Direct2D = 0,
    Skia = 1,
};

struct HostCreateInfo {
    // When non-null and valid, create a WS_CHILD host filling the parent client area.
    // Custom/layered chrome is ignored in child mode (borderless child surface).
    HWND parent = nullptr;

    // Resource-relative layout path, e.g. L"layouts/ui.xml". Empty => env / default.
    const wchar_t* layoutPath = nullptr;

    // L"system" | L"custom" | L"layered". Empty => system / env.
    // Ignored when parent is set (child mode forces borderless system surface).
    const wchar_t* chrome = nullptr;

    // L"1000x700". Empty => default / env.
    // When parent is set and size is empty, host fills the parent client rect.
    const wchar_t* size = nullptr;

    Backend renderer = Backend::Direct2D;

    // When true, Host::Run() pumps GetMessage until quit.
    // When false, the embedder owns the message loop (use ProcessMessage or DispatchMessage).
    bool ownMessagePump = true;

    // Optional: SW_* show command for the first ShowWindow (default SW_SHOWNORMAL).
    // Child hosts always show with SW_SHOW.
    int showCommand = SW_SHOWNORMAL;
};

// One retained UI window (top-level or child of HostCreateInfo::parent).
class Host {
public:
    static std::unique_ptr<Host> Create(HINSTANCE instance, const HostCreateInfo& info);
    ~Host();

    Host(const Host&) = delete;
    Host& operator=(const Host&) = delete;

    HWND GetHwnd() const;
    HWND GetParentHwnd() const;
    bool IsClosed() const;
    bool IsEmbeddedChild() const;

    // Pump until WM_QUIT.
    int Run();

    // External pump: Translate+Dispatch when msg targets this host or its children.
    // Returns true if handled.
    bool ProcessMessage(MSG& msg);

    // Resize host client area in pixels (for child hosts this is the window size).
    bool ResizeClient(int width, int height);

    // If created with parent, size/position to fill the parent client rect (0,0)-(cx,cy).
    bool FitToParent();

private:
    struct Impl;
    explicit Host(std::unique_ptr<Impl> impl);
    std::unique_ptr<Impl> m_impl;
};

void InitializeProcessDpiAwareness();

// "0.3.1"
const char* Version();

// Numeric AI_WIN_UI_VERSION
int VersionNumber();

bool OpenSecondaryHost(
    HINSTANCE instance,
    const wchar_t* layoutRelPath,
    const wchar_t* chromeMode = nullptr,
    const wchar_t* sizeWh = nullptr,
    Backend renderer = Backend::Direct2D);

} // namespace ai_win_ui
