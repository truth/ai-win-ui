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
#include <string>

namespace ai_win_ui {

enum class Backend {
    Default = 0,
    Direct2D = 1,
    Skia = 2,
};

// Invoked when a layout element with a custom onClick id (one the framework
// does not recognise) is activated, and on hover changes ("hover:<name>" /
// "hover:" on leave). actionId is UTF-8. userData is HostCreateInfo::actionUser.
using HostActionCallback = void (*)(const char* actionId, void* userData);

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

    Backend renderer = Backend::Default;

    // When true, Host::Run() pumps GetMessage until quit.
    // When false, the embedder owns the message loop (use ProcessMessage or DispatchMessage).
    bool ownMessagePump = true;

    // Optional: SW_* show command for the first ShowWindow (default SW_SHOWNORMAL).
    // Child hosts always show with SW_SHOW.
    int showCommand = SW_SHOWNORMAL;

    // When non-null, the UI is built from this in-memory UTF-8 layout string
    // instead of loading layoutPath / env / default files. XML unless
    // inlineLayoutIsJson is set.
    const char* inlineLayoutXml = nullptr;
    bool inlineLayoutIsJson = false;

    // Optional explicit top-level window position (screen pixels). Ignored for
    // child hosts. Use for restoring saved window placement.
    bool hasPosition = false;
    int posX = 0;
    int posY = 0;

    // Optional app action sink for custom onClick / hover ids (see above).
    HostActionCallback onAction = nullptr;
    void* actionUser = nullptr;
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

    // Read the current text of a named element (TextInput / RichTextBox / Label /
    // Button). Returns empty string if not found. Used to read edited form state.
    std::wstring GetElementText(const char* name) const;

    // Set the text of a named element and re-layout. Returns false if not found.
    bool SetElementText(const char* name, const wchar_t* text);

    // Request a repaint of the host window.
    void Invalidate();

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
