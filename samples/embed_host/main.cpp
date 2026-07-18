// Wave3 embed sample: shell HWND + child ai_win_ui host (parent embedding).
//
// Build: cmake --build build --config Debug --target embed_host
// Run:   build/Debug/embed_host.exe
//
// Demonstrates:
//   - HostCreateInfo::parent  (WS_CHILD fill)
//   - ownMessagePump = false  (shell owns GetMessage loop)
//   - FitToParent on WM_SIZE
//   - ai_win_ui::Version()

#include <ai_win_ui/host.h>

#include <Windows.h>
#include <memory>
#include <string>

namespace {

constexpr wchar_t kShellClass[] = L"AiWinUiEmbedShell";

struct ShellState {
    HINSTANCE instance = nullptr;
    std::unique_ptr<ai_win_ui::Host> host;
};

std::wstring GetModuleDir() {
    wchar_t path[MAX_PATH] = {};
    const DWORD n = GetModuleFileNameW(nullptr, path, MAX_PATH);
    if (n == 0 || n >= MAX_PATH) {
        return L".";
    }
    std::wstring full(path, path + n);
    const auto slash = full.find_last_of(L"\\/");
    if (slash == std::wstring::npos) {
        return L".";
    }
    return full.substr(0, slash);
}

std::wstring VersionTitle() {
    std::wstring title = L"embed_host — ai_win_ui ";
    if (const char* ver = ai_win_ui::Version()) {
        for (const char* p = ver; *p; ++p) {
            title.push_back(static_cast<wchar_t>(static_cast<unsigned char>(*p)));
        }
    }
    title += L" (child embed)";
    return title;
}

LRESULT CALLBACK ShellWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    auto* state = reinterpret_cast<ShellState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    switch (msg) {
        case WM_CREATE: {
            auto* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
            state = static_cast<ShellState*>(cs->lpCreateParams);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(state));
            return 0;
        }
        case WM_SIZE: {
            if (state && state->host && !state->host->IsClosed()) {
                state->host->FitToParent();
            }
            return 0;
        }
        case WM_DESTROY: {
            if (state) {
                state->host.reset();
            }
            PostQuitMessage(0);
            return 0;
        }
        default:
            break;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

bool CreateUiHost(ShellState& state, HWND shell) {
    ai_win_ui::HostCreateInfo info{};
    info.parent = shell;
    info.layoutPath = L"layouts/core_validation.xml";
    // Empty size => fill parent client on create.
    info.renderer = ai_win_ui::Backend::Direct2D;
    info.ownMessagePump = false;
    info.showCommand = SW_SHOW;

    wchar_t rendererEnv[64] = {};
    if (GetEnvironmentVariableW(L"AI_WIN_UI_RENDERER", rendererEnv, 64) > 0) {
        if (_wcsicmp(rendererEnv, L"skia") == 0) {
            info.renderer = ai_win_ui::Backend::Skia;
        }
    }

    state.host = ai_win_ui::Host::Create(state.instance, info);
    return static_cast<bool>(state.host);
}

} // namespace

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int cmdShow) {
    SetCurrentDirectoryW(GetModuleDir().c_str());
    ai_win_ui::InitializeProcessDpiAwareness();

    ShellState state{};
    state.instance = instance;

    WNDCLASSW wc{};
    wc.lpfnWndProc = ShellWndProc;
    wc.hInstance = instance;
    wc.lpszClassName = kShellClass;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wc.style = CS_DBLCLKS;
    if (!RegisterClassW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
        return 1;
    }

    const std::wstring title = VersionTitle();
    HWND shell = CreateWindowExW(
        0,
        kShellClass,
        title.c_str(),
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        1100,
        760,
        nullptr,
        nullptr,
        instance,
        &state);
    if (!shell) {
        return 1;
    }

    ShowWindow(shell, cmdShow == 0 ? SW_SHOWNORMAL : cmdShow);
    UpdateWindow(shell);

    if (!CreateUiHost(state, shell)) {
        MessageBoxW(
            shell,
            L"Host::Create failed (child embed).\n\n"
            L"Ensure resource/ is next to embed_host.exe\n"
            L"and yogacore is built (scripts/setup_ci_deps.ps1).",
            L"embed_host",
            MB_ICONERROR);
        DestroyWindow(shell);
        return 1;
    }

    // External message pump owned by the shell process.
    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        if (state.host && state.host->ProcessMessage(msg)) {
            continue;
        }
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return static_cast<int>(msg.wParam);
}
