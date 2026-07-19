#include "application.h"
#include "host_options.h"
#include "ui_host.h"
#include "ai_win_ui/host.h"
#include "ai_win_ui/version.h"
#include "layout_parser.h"
#include "layout_engine.h"
#include "renderer.h"
#include "resource_provider.h"
#include "style_catalog.h"
#include "text_measurer.h"
#include "theme.h"
#include "ui_context.h"
#include "ui.h"
#include "window_chrome.h"
#include "zip_resource_provider.h"

#include <imm.h>
#include <shellscalingapi.h>
#include <Windowsx.h>
#include <oleidl.h>
#include <shellapi.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace {

constexpr float kUnboundedLayoutHeight = 100000.0f;
constexpr UINT_PTR kUiTimerId = 1;
constexpr UINT_PTR kQuitTimerId = 2;
constexpr UINT kUiTimerIntervalMs = 16;
constexpr wchar_t kWindowClassName[] = L"AiWinUiWindowClass";

std::wstring ToLowerAscii(std::wstring value) {
    for (auto& ch : value) {
        if (ch >= L'A' && ch <= L'Z') {
            ch = static_cast<wchar_t>(ch - L'A' + L'a');
        }
    }
    return value;
}

RendererBackend ParseRendererBackend(const std::wstring& value) {
    const std::wstring normalized = ToLowerAscii(value);
    if (normalized == L"skia") {
        return RendererBackend::Skia;
    }
    return RendererBackend::Direct2D;
}

void InitializeDpiAwareness() {
    using SetProcessDpiAwarenessContextFn = BOOL(WINAPI*)(DPI_AWARENESS_CONTEXT);
    using SetProcessDpiAwarenessFn = HRESULT(WINAPI*)(PROCESS_DPI_AWARENESS);

    if (HMODULE user32 = GetModuleHandleW(L"user32.dll")) {
        auto setDpiAwarenessContext = reinterpret_cast<SetProcessDpiAwarenessContextFn>(
            GetProcAddress(user32, "SetProcessDpiAwarenessContext"));
        if (setDpiAwarenessContext &&
            setDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2)) {
            return;
        }
    }

    if (HMODULE shcore = LoadLibraryW(L"shcore.dll")) {
        auto setProcessDpiAwareness = reinterpret_cast<SetProcessDpiAwarenessFn>(
            GetProcAddress(shcore, "SetProcessDpiAwareness"));
        if (setProcessDpiAwareness &&
            SUCCEEDED(setProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE))) {
            FreeLibrary(shcore);
            return;
        }
        FreeLibrary(shcore);
    }

    SetProcessDPIAware();
}

UINT GetWindowDpiWithFallback(HWND hwnd) {
    using GetDpiForWindowFn = UINT(WINAPI*)(HWND);
    if (HMODULE user32 = GetModuleHandleW(L"user32.dll")) {
        auto getDpiForWindow = reinterpret_cast<GetDpiForWindowFn>(
            GetProcAddress(user32, "GetDpiForWindow"));
        if (getDpiForWindow) {
            return getDpiForWindow(hwnd);
        }
    }
    return 96;
}

} // namespace

// Process-wide multi-host lives in ai_win_ui::Application (src/application.*).
class UiHost;
void PurgeClosedSecondaryHosts();
void RegisterHostDropTarget(UiHost* host);
void RevokeHostDropTarget(HWND hwnd);


class UiHost {
    friend class HostDropTarget;
public:
    UiHost() {
        m_uiContext.requestLayout = [this]() {
            if (m_hwnd) {
                UpdateLayout();
                InvalidateRect(m_hwnd, nullptr, FALSE);
            }
        };
        m_uiContext.invalidate = [this]() {
            if (m_hwnd) {
                InvalidateRect(m_hwnd, nullptr, FALSE);
            }
        };
    }

    ~UiHost() {
        RevokeHostDropTarget(m_hwnd);
        // Never leave a live HWND with USERDATA pointing at a destroyed UiHost.
        TeardownWindow();
        // Balance the OleInitialize() from Initialize(). Must run after the
        // renderer (WIC/COM) is released in TeardownWindow().
        if (m_oleInitialized) {
            OleUninitialize();
            m_oleInitialized = false;
        }
    }

    void SetParentHwnd(HWND parent) {
        m_parentHwnd = (parent && IsWindow(parent)) ? parent : nullptr;
        m_isEmbeddedChild = m_parentHwnd != nullptr;
    }

    HWND GetParentHwnd() const { return m_parentHwnd; }
    bool IsEmbeddedChild() const { return m_isEmbeddedChild; }

    bool ResizeClient(int width, int height) {
        if (!m_hwnd || width < 1 || height < 1) {
            return false;
        }
        // Child hosts have no non-client frame: window size == client size.
        UINT flags = SWP_NOZORDER | SWP_NOACTIVATE;
        if (m_isEmbeddedChild) {
            flags |= SWP_NOMOVE;
        }
        if (!SetWindowPos(
                m_hwnd,
                nullptr,
                0,
                0,
                width,
                height,
                flags)) {
            return false;
        }
        OnResize();
        return true;
    }

    bool FitToParent() {
        if (!m_isEmbeddedChild || !m_parentHwnd || !m_hwnd || !IsWindow(m_parentHwnd)) {
            return false;
        }
        RECT rc{};
        if (!GetClientRect(m_parentHwnd, &rc)) {
            return false;
        }
        const int w = std::max(1L, rc.right - rc.left);
        const int h = std::max(1L, rc.bottom - rc.top);
        if (!SetWindowPos(m_hwnd, nullptr, 0, 0, w, h, SWP_NOZORDER | SWP_NOACTIVATE)) {
            return false;
        }
        OnResize();
        return true;
    }

    bool Initialize(HINSTANCE instance, int cmdShow) {
        m_instance = instance;
        m_cmdShow = cmdShow;

        // AI_WIN_UI_IGNORE_ENV=1|true — ignore sticky layout/chrome/size/theme/styles
        // (keeps RENDERER so scripts can still pick backend).
        ApplyIgnoreEnvIfRequested();
        // Legacy window-level vertical scroll (root content taller than client).
        // Prefer ScrollViewer in layouts. Disable: AI_WIN_UI_DISABLE_WINDOW_SCROLL=1
        m_windowScrollEnabled = !EnvTruthy(GetEnvironmentValue(L"AI_WIN_UI_DISABLE_WINDOW_SCROLL"));
        if (m_isEmbeddedChild) {
            // Child hosts should not drive host-level scrollbars.
            m_windowScrollEnabled = false;
        }

        WNDCLASSW wc{};
        wc.lpfnWndProc = &UiHost::WndProcSetup;
        wc.hInstance = instance;
        wc.lpszClassName = kWindowClassName;
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
        wc.style = CS_DBLCLKS;

        if (!RegisterClassW(&wc)) {
            const DWORD err = GetLastError();
            if (err != ERROR_CLASS_ALREADY_EXISTS) {
                return false;
            }
        }

        // Env wins for initial create; layout chrome= can promote system -> custom later.
        // Child embed forces a borderless child surface (no custom/layered chrome).
        m_chromeEnvForced = !GetEnvironmentValue(L"AI_WIN_UI_CHROME").empty();
        if (m_isEmbeddedChild) {
            m_windowChrome.SetMode(WindowChromeMode::System);
            m_chromeEnvForced = true; // ignore layout chrome= promotion for children
        } else {
            m_windowChrome.SetMode(WindowChrome::ParseMode(GetEnvironmentValue(L"AI_WIN_UI_CHROME").c_str()));
        }

        const UINT initialDpi = m_isEmbeddedChild && m_parentHwnd
            ? GetWindowDpiWithFallback(m_parentHwnd)
            : GetDpiForSystem();
        m_windowChrome.SetDpi(initialDpi);
        int clientW = 1000;
        int clientH = 700;
        // Optional: AI_WIN_UI_SIZE=420x480 or 420,480
        const std::wstring sizeEnv = GetEnvironmentValue(L"AI_WIN_UI_SIZE");
        if (!sizeEnv.empty()) {
            int w = 0;
            int h = 0;
            if (swscanf_s(sizeEnv.c_str(), L"%dx%d", &w, &h) == 2 ||
                swscanf_s(sizeEnv.c_str(), L"%d,%d", &w, &h) == 2) {
                if (w >= 50 && w <= 4000 && h >= 50 && h <= 3000) {
                    clientW = w;
                    clientH = h;
                }
            }
        } else if (m_isEmbeddedChild && m_parentHwnd) {
            RECT parentClient{};
            if (GetClientRect(m_parentHwnd, &parentClient)) {
                clientW = std::max(1L, parentClient.right - parentClient.left);
                clientH = std::max(1L, parentClient.bottom - parentClient.top);
            }
        }

        DWORD style = 0;
        DWORD exStyle = 0;
        int windowX = CW_USEDEFAULT;
        int windowY = 0;
        int windowWidth = clientW;
        int windowHeight = clientH;

        if (m_isEmbeddedChild) {
            style = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
            exStyle = 0;
            windowX = 0;
            windowY = 0;
            windowWidth = clientW;
            windowHeight = clientH;
        } else {
            RECT initialRect{0, 0, clientW, clientH};
            style = m_windowChrome.WindowStyle();
            exStyle = m_windowChrome.WindowExStyle();
            AdjustWindowRectExForDpi(&initialRect, style, FALSE, exStyle, initialDpi);
            windowWidth = initialRect.right - initialRect.left;
            windowHeight = initialRect.bottom - initialRect.top;
            // WS_POPUP ignores CW_USEDEFAULT placement and often lands at (0,0).
            // Center on the monitor under the cursor (multi-monitor aware).
            if (m_windowChrome.IsCustom()) {
                POINT cursor{};
                GetCursorPos(&cursor);
                HMONITOR monitor = MonitorFromPoint(cursor, MONITOR_DEFAULTTONEAREST);
                MONITORINFO monitorInfo{};
                monitorInfo.cbSize = sizeof(monitorInfo);
                RECT work{};
                if (GetMonitorInfoW(monitor, &monitorInfo)) {
                    work = monitorInfo.rcWork;
                } else {
                    SystemParametersInfoW(SPI_GETWORKAREA, 0, &work, 0);
                }
                windowX = work.left + std::max(0L, (work.right - work.left - windowWidth) / 2);
                windowY = work.top + std::max(0L, (work.bottom - work.top - windowHeight) / 2);
            }
        }

        m_hwnd = CreateWindowExW(
            exStyle,
            kWindowClassName,
            m_isEmbeddedChild
                ? L"AI WinUI (embedded)"
                : (m_isSecondary ? L"AI WinUI (window)" : L"AI WinUI Renderer (DirectUI-style)"),
            style,
            windowX,
            windowY,
            windowWidth,
            windowHeight,
            m_isEmbeddedChild ? m_parentHwnd : nullptr,
            nullptr,
            instance,
            this
        );

        if (!m_hwnd) {
            return false;
        }

        ai_win_ui::Application::OnHostCreated();

        auto fail = [this]() -> bool {
            TeardownWindow();
            return false;
        };

        UpdateDpiContext(GetWindowDpiWithFallback(m_hwnd));
        m_windowChrome.SetDpi(m_uiContext.dpi);
        if (!m_isEmbeddedChild && m_windowChrome.IsCustom()) {
            m_windowChrome.InitializeDwm(m_hwnd);
        }

        // OLE drag & drop requires the UI thread to be a Single-Threaded
        // Apartment. Establish STA here, BEFORE the renderer initializes COM
        // as MTA (CoInitializeEx(COINIT_MULTITHREADED)) — otherwise OleInitialize
        // fails with RPC_E_CHANGED_MODE and RegisterDragDrop never runs. The
        // renderer already tolerates RPC_E_CHANGED_MODE, so it keeps the STA.
        m_oleInitialized = SUCCEEDED(OleInitialize(nullptr));

        m_requestedRendererBackend = ParseRendererBackend(GetEnvironmentValue(L"AI_WIN_UI_RENDERER"));
        if (!InitializeRenderer()) {
            return fail();
        }
        m_uiContext.renderer = m_renderer.get();

        m_textMeasurer = CreateTextMeasurer(m_activeRendererBackend);
        if (!m_textMeasurer) {
            return fail();
        }
        m_uiContext.textMeasurer = m_textMeasurer.get();

        m_layoutEngine = CreateDefaultLayoutEngine();
        if (!m_layoutEngine) {
            return fail();
        }
        m_uiContext.layoutEngine = m_layoutEngine.get();

        BuildUI();
        if (m_root) {
            m_root->SetContext(&m_uiContext);
        }
        ApplyChromeFromRootIfNeeded();
        m_isInitialized = true;
        OnResize();
        LogEffectiveEnv();
        MaybeDumpMeasureTree();
        MaybeDumpTextMetrics();

        // Layered windows stay invisible until the first UpdateLayeredWindow.
        // Force an initial paint while still hidden so the first ShowWindow has
        // real per-pixel content (otherwise the window only "appears" after
        // taskbar activation triggers a later paint).
        if (m_windowChrome.IsLayered()) {
            RedrawWindow(
                m_hwnd,
                nullptr,
                nullptr,
                RDW_INVALIDATE | RDW_UPDATENOW | RDW_INTERNALPAINT);
        }

        // Normalize shell show commands that would hide the window.
        int showCmd = cmdShow;
        if (m_isEmbeddedChild) {
            showCmd = SW_SHOW;
        } else if (showCmd == SW_HIDE || showCmd == 0) {
            showCmd = SW_SHOWNORMAL;
        }
        ShowWindow(m_hwnd, showCmd);
        UpdateWindow(m_hwnd);
        if (!m_isEmbeddedChild) {
            BringAppWindowToForeground(m_hwnd);
        }

        SetTimer(m_hwnd, kUiTimerId, kUiTimerIntervalMs, nullptr);

        // Headless/smoke: AI_WIN_UI_QUIT_AFTER_MS=1500 exits after layout paints.
        // Do not apply quit timer to secondary/embedded hosts.
        if (!m_isSecondary && !m_isEmbeddedChild) {
            const std::wstring quitAfter = GetEnvironmentValue(L"AI_WIN_UI_QUIT_AFTER_MS");
            if (!quitAfter.empty()) {
                const int ms = _wtoi(quitAfter.c_str());
                if (ms > 0) {
                    SetTimer(m_hwnd, kQuitTimerId, static_cast<UINT>(ms), nullptr);
                }
            }
        }
        RegisterHostDropTarget(this);
        return true;
    }

    void TeardownWindow() {
        m_isInitialized = false;
        m_focusedElement = nullptr;
        m_mouseCaptureTarget = nullptr;

        // Drop UI / text (may hold DWrite) before renderer CoUninitialize.
        m_root.reset();
        m_layoutEngine.reset();
        m_textMeasurer.reset();
        m_uiContext.textMeasurer = nullptr;
        m_uiContext.layoutEngine = nullptr;
        m_uiContext.renderer = nullptr;

        // Release HwndRenderTarget / Skia surface while HWND is still valid.
        m_renderer.reset();

        if (m_hwnd) {
            KillTimer(m_hwnd, kUiTimerId);
            KillTimer(m_hwnd, kQuitTimerId);
            SetWindowLongPtrW(m_hwnd, GWLP_USERDATA, 0);
            const HWND hwnd = m_hwnd;
            m_hwnd = nullptr;
            // DestroyWindow may re-enter WM_DESTROY; guard with m_tearingDown.
            if (!m_tearingDown && IsWindow(hwnd)) {
                m_tearingDown = true;
                DestroyWindow(hwnd);
                m_tearingDown = false;
            }
        }
    }

    // Windows blocks SetForegroundWindow when launched from another process
    // (console / PowerShell Start-Process). AttachThreadInput is the usual
    // way to still raise our window so it does not sit only on the taskbar.
    static void BringAppWindowToForeground(HWND hwnd) {
        if (!hwnd) {
            return;
        }

        if (IsIconic(hwnd)) {
            ShowWindow(hwnd, SW_RESTORE);
        } else {
            ShowWindow(hwnd, SW_SHOW);
        }

        SetWindowPos(
            hwnd,
            HWND_TOP,
            0,
            0,
            0,
            0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);

        const HWND foreground = GetForegroundWindow();
        const DWORD thisThread = GetCurrentThreadId();
        DWORD foregroundThread = 0;
        if (foreground) {
            foregroundThread = GetWindowThreadProcessId(foreground, nullptr);
        }

        BOOL attached = FALSE;
        if (foregroundThread != 0 && foregroundThread != thisThread) {
            attached = AttachThreadInput(foregroundThread, thisThread, TRUE);
        }

        BringWindowToTop(hwnd);
        SetForegroundWindow(hwnd);
        SetActiveWindow(hwnd);
        SetFocus(hwnd);

        if (attached) {
            AttachThreadInput(foregroundThread, thisThread, FALSE);
        }

        // Layered: ensure one more present after becoming visible/foreground.
        RedrawWindow(
            hwnd,
            nullptr,
            nullptr,
            RDW_INVALIDATE | RDW_UPDATENOW | RDW_INTERNALPAINT);
    }

    int Run() {
        MSG msg{};
        while (GetMessageW(&msg, nullptr, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
            PurgeClosedSecondaryHosts();
        }
        return static_cast<int>(msg.wParam);
    }

    // For Application::PurgeClosedSecondaries type-erased callbacks.
    static bool HostIsClosed(void* host) {
        return host && static_cast<UiHost*>(host)->IsClosed();
    }
    static void HostDestroy(void* host) {
        delete static_cast<UiHost*>(host);
    }

    // Open a secondary host from structured options (H2). Prefer this over raw env.
    static bool OpenSecondaryHost(HINSTANCE instance, const OpenHostOptions& options) {
        // Snapshot parent env without mutating globals until the last moment.
        wchar_t prevLayoutBuf[512] = {};
        wchar_t prevChromeBuf[512] = {};
        wchar_t prevSizeBuf[512] = {};
        wchar_t prevRendererBuf[512] = {};
        const DWORD nLayout = GetEnvironmentVariableW(L"AI_WIN_UI_LAYOUT", prevLayoutBuf, 512);
        const DWORD nChrome = GetEnvironmentVariableW(L"AI_WIN_UI_CHROME", prevChromeBuf, 512);
        const DWORD nSize = GetEnvironmentVariableW(L"AI_WIN_UI_SIZE", prevSizeBuf, 512);
        const DWORD nRenderer = GetEnvironmentVariableW(L"AI_WIN_UI_RENDERER", prevRendererBuf, 512);

        SetEnvironmentVariableW(L"AI_WIN_UI_LAYOUT", options.layout.c_str());
        // Always set/clear chrome so secondary does not inherit hub chrome env.
        if (!options.chrome.empty() && options.chrome != L"system") {
            SetEnvironmentVariableW(L"AI_WIN_UI_CHROME", options.chrome.c_str());
        } else {
            SetEnvironmentVariableW(L"AI_WIN_UI_CHROME", nullptr);
        }
        if (!options.size.empty()) {
            SetEnvironmentVariableW(L"AI_WIN_UI_SIZE", options.size.c_str());
        } else {
            SetEnvironmentVariableW(L"AI_WIN_UI_SIZE", nullptr);
        }
        SetEnvironmentVariableW(
            L"AI_WIN_UI_RENDERER",
            options.renderer == RendererBackend::Skia ? L"skia" : L"direct2d");
        // Never auto-quit secondary hosts.
        SetEnvironmentVariableW(L"AI_WIN_UI_QUIT_AFTER_MS", nullptr);

        UiHost* host = new UiHost();
        host->m_isSecondary = true;
        const bool ok = host->Initialize(instance, SW_SHOWNORMAL);

        auto restore = [](const wchar_t* name, DWORD n, const wchar_t* buf) {
            if (n == 0 || n >= 512) {
                SetEnvironmentVariableW(name, nullptr);
            } else {
                SetEnvironmentVariableW(name, buf);
            }
        };
        restore(L"AI_WIN_UI_LAYOUT", nLayout, prevLayoutBuf);
        restore(L"AI_WIN_UI_CHROME", nChrome, prevChromeBuf);
        restore(L"AI_WIN_UI_SIZE", nSize, prevSizeBuf);
        restore(L"AI_WIN_UI_RENDERER", nRenderer, prevRendererBuf);

        if (!ok) {
            // Initialize already tore down the HWND on failure.
            delete host;
            return false;
        }
        ai_win_ui::Application::RegisterSecondary(host);
        return true;
    }

    static bool OpenSecondaryHost(
        HINSTANCE instance,
        const wchar_t* layoutRelPath,
        const wchar_t* chromeMode,
        const wchar_t* sizeWh,
        RendererBackend renderer) {
        OpenHostOptions options;
        options.layout = layoutRelPath ? layoutRelPath : L"";
        options.chrome = chromeMode ? chromeMode : L"";
        options.size = sizeWh ? sizeWh : L"";
        options.renderer = renderer;
        return OpenSecondaryHost(instance, options);
    }

    void MarkClosed() { m_closed = true; }
    bool IsClosed() const { return m_closed; }
    bool IsSecondary() const { return m_isSecondary; }
    HWND GetHwnd() const { return m_hwnd; }

    // Apply structured options as process env for the next Initialize (embed/API).
    static void ApplyOptionsToEnvironment(const OpenHostOptions& options) {
        if (!options.layout.empty()) {
            SetEnvironmentVariableW(L"AI_WIN_UI_LAYOUT", options.layout.c_str());
        }
        if (!options.chrome.empty() && options.chrome != L"system") {
            SetEnvironmentVariableW(L"AI_WIN_UI_CHROME", options.chrome.c_str());
        } else if (!options.chrome.empty()) {
            SetEnvironmentVariableW(L"AI_WIN_UI_CHROME", nullptr);
        }
        if (!options.size.empty()) {
            SetEnvironmentVariableW(L"AI_WIN_UI_SIZE", options.size.c_str());
        }
        SetEnvironmentVariableW(
            L"AI_WIN_UI_RENDERER",
            options.renderer == RendererBackend::Skia ? L"skia" : L"direct2d");
    }

private:
    bool InitializeRenderer() {
        // Layered present is supported on both Direct2D and Skia. Prefer the
        // requested backend; fall back to Direct2D if Skia init fails.
        m_renderer = CreateRenderer(m_requestedRendererBackend);
        if (ConfigureAndInitRenderer(m_renderer.get())) {
            m_activeRendererBackend = m_renderer->Backend();
            return true;
        }

        if (m_requestedRendererBackend != RendererBackend::Direct2D) {
            m_renderer = CreateDirect2DRenderer();
            if (ConfigureAndInitRenderer(m_renderer.get())) {
                m_activeRendererBackend = m_renderer->Backend();
                return true;
            }
        }

        m_renderer.reset();
        return false;
    }

    bool ConfigureAndInitRenderer(IRenderer* renderer) {
        if (!renderer) {
            return false;
        }
        renderer->SetPresentMode(
            m_windowChrome.IsLayered() ? PresentMode::Layered : PresentMode::Hwnd);
        return renderer->Initialize(m_hwnd);
    }

    void UpdateDpiContext(UINT dpi) {
        m_uiContext.dpi = dpi > 0 ? dpi : 96;
        m_uiContext.dpiScale = static_cast<float>(m_uiContext.dpi) / 96.0f;
        m_windowChrome.SetDpi(m_uiContext.dpi);
    }

    void ApplyChromeFromRootIfNeeded() {
        if (!m_root || m_chromeEnvForced) {
            return;
        }
        const auto request = m_root->GetWindowChromeRequest();
        if (request == UIElement::WindowChromeRequest::Unspecified) {
            return;
        }
        WindowChromeMode desired = WindowChromeMode::System;
        if (request == UIElement::WindowChromeRequest::Custom) {
            desired = WindowChromeMode::Custom;
        } else if (request == UIElement::WindowChromeRequest::Layered) {
            desired = WindowChromeMode::Layered;
        }
        if (desired == m_windowChrome.Mode()) {
            return;
        }
        m_windowChrome.SetMode(desired);
        m_windowChrome.ApplyWindowStyle(m_hwnd);
        if (m_windowChrome.IsCustom()) {
            m_windowChrome.InitializeDwm(m_hwnd);
        }
        if (m_renderer) {
            m_renderer->SetPresentMode(
                m_windowChrome.IsLayered() ? PresentMode::Layered : PresentMode::Hwnd);
            // Force surface recreate with the new present mode.
            m_renderer->Resize(
                static_cast<UINT>(std::max(1.0f, m_viewportWidth)),
                static_cast<UINT>(std::max(1.0f, m_viewportHeight)));
        }
    }

    static RECT ToGdiRect(const Rect& bounds) {
        return RECT{
            static_cast<LONG>(std::floor(bounds.left)),
            static_cast<LONG>(std::floor(bounds.top)),
            static_cast<LONG>(std::ceil(bounds.right)),
            static_cast<LONG>(std::ceil(bounds.bottom)),
        };
    }

    void RefreshChromeHitRegions() {
        if (!m_windowChrome.IsCustom() || !m_root) {
            m_windowChrome.ClearHitRegions();
            m_windowChrome.ClearContentBounds();
            return;
        }

        std::vector<std::pair<Rect, UIElement::HitTestRole>> captionRegions;
        std::vector<Rect> clientOnlyRegions;
        m_root->CollectChromeHitRegions(captionRegions, clientOnlyRegions);

        std::vector<WindowChromeHitRegion> regions;
        regions.reserve(captionRegions.size() + clientOnlyRegions.size());
        for (const auto& entry : captionRegions) {
            WindowChromeHitRegion region{};
            region.rect = ToGdiRect(entry.first);
            region.caption = true;
            regions.push_back(region);
        }
        for (const auto& bounds : clientOnlyRegions) {
            WindowChromeHitRegion region{};
            region.rect = ToGdiRect(bounds);
            region.clientOnly = true;
            regions.push_back(region);
        }
        m_windowChrome.SetHitRegions(std::move(regions));

        // Layered floating card: resize grips follow the first child card, not
        // the transparent outer padding of the HWND.
        if (m_windowChrome.IsLayered() && !m_root->Children().empty()) {
            m_windowChrome.SetContentBounds(
                ToGdiRect(m_root->Children().front()->Bounds()),
                true);
        } else {
            m_windowChrome.ClearContentBounds();
        }

        // Caption band used for inactive dim overlay.
        float band = static_cast<float>(m_windowChrome.FallbackCaptionHeightPx());
        for (const auto& entry : captionRegions) {
            band = std::max(band, entry.first.bottom);
        }
        // Clamp to a reasonable title-bar height so content is not dimmed.
        m_captionBandHeight = std::min(band, std::max(0.0f, m_viewportHeight * 0.25f));
        if (m_captionBandHeight <= 0.0f) {
            m_captionBandHeight = static_cast<float>(m_windowChrome.FallbackCaptionHeightPx());
        }
    }

    static void CollectCaptionMaximizeButtons(UIElement* element, std::vector<Button*>& out) {
        if (!element) {
            return;
        }
        if (auto* button = dynamic_cast<Button*>(element)) {
            if (button->GetVariant() == Button::Variant::CaptionMaximize) {
                out.push_back(button);
            }
        }
        for (auto& child : element->Children()) {
            CollectCaptionMaximizeButtons(child.get(), out);
        }
    }

    void SyncCaptionMaximizeGlyphs() {
        if (!m_root || !m_hwnd) {
            return;
        }
        const bool maximized = IsZoomed(m_hwnd) != FALSE;
        std::vector<Button*> buttons;
        CollectCaptionMaximizeButtons(m_root.get(), buttons);
        bool changed = false;
        for (Button* button : buttons) {
            if (button->ShowRestoreGlyph() != maximized) {
                button->SetShowRestoreGlyph(maximized);
                changed = true;
            }
        }
        if (changed) {
            InvalidateRect(m_hwnd, nullptr, FALSE);
        }
    }

    std::wstring GetEnvironmentValue(const wchar_t* name) const {
        wchar_t buffer[512] = {};
        const DWORD length = GetEnvironmentVariableW(name, buffer, ARRAYSIZE(buffer));
        if (length == 0 || length >= ARRAYSIZE(buffer)) {
            return L"";
        }
        return std::wstring(buffer, buffer + length);
    }

    std::string Utf16ToUtf8(const std::wstring& value) const {
        if (value.empty()) {
            return {};
        }
        const int size = WideCharToMultiByte(CP_UTF8, 0, value.c_str(), -1, nullptr, 0, nullptr, nullptr);
        if (size <= 1) {
            return {};
        }
        std::string result(size - 1, '\0');
        WideCharToMultiByte(CP_UTF8, 0, value.c_str(), -1, result.data(), size, nullptr, nullptr);
        return result;
    }

    std::wstring GetExecutableDirectory() const {
        wchar_t path[MAX_PATH] = {};
        if (GetModuleFileNameW(nullptr, path, ARRAYSIZE(path)) == 0) {
            return L"";
        }
        std::wstring exePath(path);
        const size_t pos = exePath.find_last_of(L"\\/");
        return pos != std::wstring::npos ? exePath.substr(0, pos) : exePath;
    }

    void LogUtf8Debug(const char* prefix, const std::string& path) const {
        std::wstring msg = prefix ? Utf8ToWide(prefix) : L"";
        if (!path.empty()) {
            msg += Utf8ToWide(path);
        } else {
            msg += L"(none)";
        }
        msg += L"\n";
        OutputDebugStringW(msg.c_str());
    }

    static std::wstring Utf8ToWide(const std::string& utf8) {
        if (utf8.empty()) {
            return {};
        }
        const int n = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
        if (n <= 1) {
            return {};
        }
        std::wstring w(static_cast<size_t>(n - 1), L'\0');
        MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, w.data(), n);
        return w;
    }

    void LoadThemeFromPath(const std::string& themePath) {
        if (!m_uiContext.resourceProvider) {
            OutputDebugStringW(L"[Theme] skipped (no resource provider)\n");
            return;
        }
        if (themePath.empty() || !m_uiContext.resourceProvider->Exists(themePath)) {
            LogUtf8Debug("[Theme] missing path=", themePath.empty() ? std::string("(empty)") : themePath);
            return;
        }
        const std::string text = m_uiContext.resourceProvider->LoadText(themePath);
        m_theme = Theme::LoadFromJson(text);
        m_uiContext.theme = m_theme.get();
        m_loadedThemePath = themePath;
        {
            std::wstring msg = L"[Theme] loaded=";
            msg += Utf8ToWide(themePath);
            if (m_theme) {
                msg += L" colors=";
                msg += std::to_wstring(static_cast<unsigned long long>(m_theme->colors.size()));
                msg += L" spacings=";
                msg += std::to_wstring(static_cast<unsigned long long>(m_theme->spacings.size()));
            }
            msg += L"\n";
            OutputDebugStringW(msg.c_str());
        }
    }

    void LoadTheme() {
        m_loadedThemePath.clear();
        const std::wstring requestedTheme = GetEnvironmentValue(L"AI_WIN_UI_THEME");
        const std::string themePath = requestedTheme.empty()
            ? std::string("themes/default.json")
            : Utf16ToUtf8(requestedTheme);
        LoadThemeFromPath(themePath);
    }

    // S4 runtime switch: reload theme JSON, re-parse catalog ($color.*), rebind tree.
    void ApplyThemePath(const std::string& themePath) {
        LoadThemeFromPath(themePath);
        // Re-parse style catalog so $color tokens resolve against the new theme.
        LoadStyleCatalog();
        if (m_root) {
            WalkApplyThemeDefaults(m_root.get());
            m_root->SetContext(&m_uiContext);
            OnResize(); // remeasure / rearrange
            InvalidateRect(m_hwnd, nullptr, FALSE);
            std::wstring title = L"Theme: ";
            title += Utf8ToWide(themePath);
            SetWindowTextW(m_hwnd, title.c_str());
        }
    }

    void WalkApplyThemeDefaults(UIElement* element) {
        if (!element) {
            return;
        }
        element->RebindCatalogStyle();
        element->ApplyThemeDefaults();
        for (const auto& child : element->Children()) {
            WalkApplyThemeDefaults(child.get());
        }
    }

    void LoadStyleCatalog() {
        m_loadedStylesPaths.clear();
        m_styleCatalog = std::make_unique<StyleCatalog>();
        m_uiContext.styleCatalog = m_styleCatalog.get();
        if (!m_uiContext.resourceProvider) {
            OutputDebugStringW(L"[Styles] skipped (no resource provider)\n");
            return;
        }

        // Resolve $color.* against the currently loaded theme (needed for S4 reload).
        const Theme* previousTheme = LayoutParser::SetActiveTheme(m_uiContext.theme);

        // Default shared catalog (import-aware).
        if (m_uiContext.resourceProvider->Exists("styles/default.json")) {
            if (m_styleCatalog->LoadFromResource(*m_uiContext.resourceProvider, "styles/default.json")) {
                m_loadedStylesPaths.push_back("styles/default.json");
            }
        }

        // Optional override / extra styles: AI_WIN_UI_STYLES=styles/app.json
        const std::wstring requested = GetEnvironmentValue(L"AI_WIN_UI_STYLES");
        if (!requested.empty()) {
            const std::string path = Utf16ToUtf8(requested);
            if (!path.empty() && m_uiContext.resourceProvider->Exists(path)) {
                if (m_styleCatalog->MergeFile(*m_uiContext.resourceProvider, path)) {
                    m_loadedStylesPaths.push_back(path);
                } else {
                    LogUtf8Debug("[Styles] merge failed path=", path);
                }
            } else {
                LogUtf8Debug("[Styles] AI_WIN_UI_STYLES not found path=", path);
            }
        }

        // S3: print resolved catalog sources + style count.
        {
            std::wstring msg = L"[Styles] files=";
            if (m_loadedStylesPaths.empty()) {
                msg += L"(none)";
            } else {
                for (size_t i = 0; i < m_loadedStylesPaths.size(); ++i) {
                    if (i) {
                        msg += L";";
                    }
                    msg += Utf8ToWide(m_loadedStylesPaths[i]);
                }
            }
            msg += L" styleCount=";
            msg += std::to_wstring(static_cast<unsigned long long>(
                m_styleCatalog ? m_styleCatalog->Count() : 0));
            msg += L"\n";
            OutputDebugStringW(msg.c_str());
        }

        LayoutParser::SetActiveTheme(previousTheme);
    }

    // Open any layout as a secondary host (default in-process).
    // chrome: "" / "system" => system frame; "custom"; "layered"
    // Deferred via PostMessage so we never CreateWindow re-entrantly inside mouse handlers.
    void LaunchDemoHost(const OpenHostOptions& options) {
        OpenHostOptions opts = options;
        if (opts.chrome == L"system") {
            opts.chrome.clear();
        }
        if (EnvTruthy(GetEnvironmentValue(L"AI_WIN_UI_CHILD_PROCESS"))) {
            opts.childProcess = true;
        }

        if (!opts.childProcess) {
            auto& pending = ai_win_ui::Application::PendingOpen();
            pending = opts;
            if (pending.renderer != RendererBackend::Skia &&
                pending.renderer != RendererBackend::Direct2D) {
                pending.renderer = m_activeRendererBackend;
            }
            // Prefer hub's active backend when not explicitly set via process env.
            if (GetEnvironmentValue(L"AI_WIN_UI_RENDERER").empty()) {
                pending.renderer = m_activeRendererBackend;
            }
            ai_win_ui::Application::HasPendingOpen() = true;
            if (m_hwnd) {
                PostMessageW(m_hwnd, ai_win_ui::kMsgOpenDemoHost, 0, 0);
            }
            return;
        }

        LaunchDemoHostProcess(opts);
    }

    void LaunchDemoHost(const wchar_t* layoutRelPath, const wchar_t* chromeMode, const wchar_t* sizeWh) {
        OpenHostOptions options;
        options.layout = layoutRelPath ? layoutRelPath : L"";
        options.chrome = chromeMode ? chromeMode : L"";
        options.size = sizeWh ? sizeWh : L"";
        options.renderer = m_activeRendererBackend;
        LaunchDemoHost(options);
    }

    void LaunchDemoHostProcess(const OpenHostOptions& options) {
        const wchar_t* layoutRelPath = options.layout.c_str();
        const wchar_t* sizeWh = options.size.empty() ? nullptr : options.size.c_str();
        const std::wstring& chrome = options.chrome;

        wchar_t exePath[MAX_PATH] = {};
        if (GetModuleFileNameW(nullptr, exePath, ARRAYSIZE(exePath)) == 0) {
            return;
        }

        const std::wstring prevLayout = GetEnvironmentValue(L"AI_WIN_UI_LAYOUT");
        const std::wstring prevChrome = GetEnvironmentValue(L"AI_WIN_UI_CHROME");
        const std::wstring prevSize = GetEnvironmentValue(L"AI_WIN_UI_SIZE");
        const std::wstring prevRenderer = GetEnvironmentValue(L"AI_WIN_UI_RENDERER");

        SetEnvironmentVariableW(L"AI_WIN_UI_LAYOUT", layoutRelPath);
        if (chrome.empty()) {
            SetEnvironmentVariableW(L"AI_WIN_UI_CHROME", nullptr);
        } else {
            SetEnvironmentVariableW(L"AI_WIN_UI_CHROME", chrome.c_str());
        }
        if (sizeWh && sizeWh[0] != L'\0') {
            SetEnvironmentVariableW(L"AI_WIN_UI_SIZE", sizeWh);
        } else {
            SetEnvironmentVariableW(L"AI_WIN_UI_SIZE", nullptr);
        }
        SetEnvironmentVariableW(
            L"AI_WIN_UI_RENDERER",
            options.renderer == RendererBackend::Skia ? L"skia" : L"direct2d");

        std::wstring cmdLine = L"\"";
        cmdLine += exePath;
        cmdLine += L"\"";
        std::vector<wchar_t> mutableCmd(cmdLine.begin(), cmdLine.end());
        mutableCmd.push_back(L'\0');

        STARTUPINFOW si{};
        si.cb = sizeof(si);
        PROCESS_INFORMATION pi{};
        const BOOL ok = CreateProcessW(
            exePath,
            mutableCmd.data(),
            nullptr,
            nullptr,
            FALSE,
            0,
            nullptr,
            nullptr,
            &si,
            &pi);
        if (ok) {
            if (pi.hThread) {
                CloseHandle(pi.hThread);
            }
            if (pi.hProcess) {
                CloseHandle(pi.hProcess);
            }
            SetWindowTextW(m_hwnd, L"Opened demo child process");
        } else {
            SetWindowTextW(m_hwnd, L"Failed to launch demo child process");
        }

        auto restore = [](const wchar_t* name, const std::wstring& value) {
            if (value.empty()) {
                SetEnvironmentVariableW(name, nullptr);
            } else {
                SetEnvironmentVariableW(name, value.c_str());
            }
        };
        restore(L"AI_WIN_UI_LAYOUT", prevLayout);
        restore(L"AI_WIN_UI_CHROME", prevChrome);
        restore(L"AI_WIN_UI_SIZE", prevSize);
        restore(L"AI_WIN_UI_RENDERER", prevRenderer);
    }

    void BuildUI() {
        const std::wstring baseDir = GetExecutableDirectory();
        const std::wstring zipPath = baseDir.empty() ? L"assets.zip" : baseDir + L"\\assets.zip";
        const std::wstring resourceDir = baseDir.empty() ? L"resource" : baseDir + L"\\resource";

        auto zipProvider = std::make_unique<ZipResourceProvider>(zipPath);
        auto dirProvider = std::make_unique<DirectoryResourceProvider>(resourceDir);
        if (zipProvider->IsValid()) {
            m_resourceProvider = std::make_unique<FallbackResourceProvider>(std::move(zipProvider), std::move(dirProvider));
        } else {
            m_resourceProvider = std::move(dirProvider);
        }
        m_uiContext.resourceProvider = m_resourceProvider.get();

        m_uiContext.eventResolver = [this](const std::string& eventId) -> UIEventHandler {
            if (eventId == "primaryAction") {
                return [this]() { SetWindowTextW(m_hwnd, L"Clicked: Primary Action"); };
            }
            if (eventId == "secondaryAction") {
                return [this]() { SetWindowTextW(m_hwnd, L"Clicked: Secondary Action"); };
            }
            // S4: applyTheme:themes/default.json | themes/light.json
            if (eventId.rfind("applyTheme:", 0) == 0) {
                const std::string path = eventId.substr(11);
                return [this, path]() { ApplyThemePath(path); };
            }
            // C4: togglePopup:elementName
            if (eventId.rfind("togglePopup:", 0) == 0) {
                const std::string name = eventId.substr(12);
                return [this, name]() {
                    if (!m_root) {
                        return;
                    }
                    if (UIElement* el = m_root->FindElementByName(name)) {
                        if (auto* popup = dynamic_cast<Popup*>(el)) {
                            popup->ToggleOpen();
                            InvalidateRect(m_hwnd, nullptr, FALSE);
                        }
                    }
                };
            }
            // C7: openModal:elementName
            if (eventId.rfind("openModal:", 0) == 0) {
                const std::string name = eventId.substr(10);
                return [this, name]() {
                    if (!m_root) return;
                    if (UIElement* el = m_root->FindElementByName(name)) {
                        if (auto* modal = dynamic_cast<Modal*>(el)) {
                            modal->Open();
                            InvalidateRect(m_hwnd, nullptr, FALSE);
                        }
                    }
                };
            }
            if (eventId.rfind("closeModal:", 0) == 0) {
                const std::string name = eventId.substr(11);
                return [this, name]() {
                    if (!m_root) return;
                    if (UIElement* el = m_root->FindElementByName(name)) {
                        if (auto* modal = dynamic_cast<Modal*>(el)) {
                            modal->Close();
                            InvalidateRect(m_hwnd, nullptr, FALSE);
                        }
                    }
                };
            }
            if (eventId == "windowMinimize") {
                return [this]() { ShowWindow(m_hwnd, SW_MINIMIZE); };
            }
            if (eventId == "windowMaximize") {
                return [this]() {
                    ShowWindow(m_hwnd, IsZoomed(m_hwnd) ? SW_RESTORE : SW_MAXIMIZE);
                };
            }
            if (eventId == "windowClose") {
                return [this]() { PostMessageW(m_hwnd, WM_CLOSE, 0, 0); };
            }
            // Shaped demos (layered).
            if (eventId == "openHeartWindow") {
                return [this]() {
                    LaunchDemoHost(L"layouts/shaped_heart_window.xml", L"layered", L"420x460");
                };
            }
            if (eventId == "openPetalWindow") {
                return [this]() {
                    LaunchDemoHost(L"layouts/shaped_petal_window.xml", L"layered", L"440x440");
                };
            }
            if (eventId == "openOvalWindow") {
                return [this]() {
                    LaunchDemoHost(L"layouts/shaped_oval_window.xml", L"layered", L"400x320");
                };
            }
            if (eventId == "openStarWindow") {
                return [this]() {
                    LaunchDemoHost(L"layouts/shaped_star_window.xml", L"layered", L"400x400");
                };
            }
            // Gallery: openDemo:layoutPath[|chrome][|size]
            // chrome: system|custom|layered (default system). size: e.g. 1100x750
            if (eventId.rfind("openDemo:", 0) == 0) {
                const std::string payload = eventId.substr(9);
                return [this, payload]() {
                    std::string layout = payload;
                    std::string chrome;
                    std::string size;
                    const auto p1 = payload.find('|');
                    if (p1 == std::string::npos) {
                        layout = payload;
                    } else {
                        layout = payload.substr(0, p1);
                        const auto rest = payload.substr(p1 + 1);
                        const auto p2 = rest.find('|');
                        if (p2 == std::string::npos) {
                            chrome = rest;
                        } else {
                            chrome = rest.substr(0, p2);
                            size = rest.substr(p2 + 1);
                        }
                    }
                    if (layout.empty()) {
                        return;
                    }
                    // Utf8 -> wide for env APIs.
                    auto toWide = [](const std::string& utf8) -> std::wstring {
                        if (utf8.empty()) {
                            return {};
                        }
                        const int n = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
                        if (n <= 1) {
                            return {};
                        }
                        std::wstring w(static_cast<size_t>(n - 1), L'\0');
                        MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, w.data(), n);
                        return w;
                    };
                    const std::wstring wLayout = toWide(layout);
                    const std::wstring wChrome = toWide(chrome);
                    const std::wstring wSize = toWide(size);
                    LaunchDemoHost(
                        wLayout.c_str(),
                        wChrome.empty() ? L"" : wChrome.c_str(),
                        wSize.empty() ? nullptr : wSize.c_str());
                };
            }
            return UIEventHandler();
        };

        LoadTheme();
        LoadStyleCatalog();

        std::vector<std::string> layoutCandidates;
        const std::wstring requestedLayout = GetEnvironmentValue(L"AI_WIN_UI_LAYOUT");
        if (!requestedLayout.empty()) {
            const std::string requestedUtf8 = Utf16ToUtf8(requestedLayout);
            if (!requestedUtf8.empty()) {
                layoutCandidates.push_back(requestedUtf8);

                const bool endsWithJson =
                    requestedUtf8.size() >= 5 && requestedUtf8.substr(requestedUtf8.size() - 5) == ".json";
                const bool endsWithXml =
                    requestedUtf8.size() >= 4 && requestedUtf8.substr(requestedUtf8.size() - 4) == ".xml";
                if (endsWithJson) {
                    layoutCandidates.push_back(requestedUtf8.substr(0, requestedUtf8.size() - 5) + ".xml");
                } else if (endsWithXml) {
                    layoutCandidates.push_back(requestedUtf8.substr(0, requestedUtf8.size() - 4) + ".json");
                }
            }
        }

        layoutCandidates.push_back("layouts/ui.json");
        layoutCandidates.push_back("layouts/ui.xml");

        for (const auto& path : layoutCandidates) {
            m_root = LayoutParser::BuildFromFile(m_uiContext, path);
            if (m_root) {
                m_activeLayoutPath = path;
                break;
            }
        }

        if (!m_root) {
            BuildDefaultUI();
            m_activeLayoutPath = "default-ui";
        }

        UpdateWindowTitle();
        SetFocusedElement(GetFirstFocusable());
    }

    void UpdateWindowTitle() {
        std::wstring title = L"AI WinUI [";
        title += RendererBackendDisplayName(m_activeRendererBackend);
        if (m_requestedRendererBackend != m_activeRendererBackend) {
            title += L" fallback from ";
            title += RendererBackendDisplayName(m_requestedRendererBackend);
        }
        title += L"]";
        if (m_windowChrome.IsLayered()) {
            title += L" chrome=layered";
        } else if (m_windowChrome.IsCustom()) {
            title += L" chrome=custom";
        }
        if (!m_activeLayoutPath.empty()) {
            const int size = MultiByteToWideChar(CP_UTF8, 0, m_activeLayoutPath.c_str(), -1, nullptr, 0);
            if (size > 1) {
                std::wstring layoutPath(size - 1, L'\0');
                MultiByteToWideChar(CP_UTF8, 0, m_activeLayoutPath.c_str(), -1, layoutPath.data(), size);
                title += L" - ";
                title += layoutPath;
            }
        }
        SetWindowTextW(m_hwnd, title.c_str());
    }

    static bool EnvTruthy(const std::wstring& value) {
        return value == L"1" || value == L"true" || value == L"TRUE" || value == L"yes" || value == L"YES";
    }

    void ApplyIgnoreEnvIfRequested() {
        if (!EnvTruthy(GetEnvironmentValue(L"AI_WIN_UI_IGNORE_ENV"))) {
            return;
        }
        // Clear sticky demo vars that commonly break "just launch the app".
        SetEnvironmentVariableW(L"AI_WIN_UI_LAYOUT", nullptr);
        SetEnvironmentVariableW(L"AI_WIN_UI_CHROME", nullptr);
        SetEnvironmentVariableW(L"AI_WIN_UI_SIZE", nullptr);
        SetEnvironmentVariableW(L"AI_WIN_UI_THEME", nullptr);
        SetEnvironmentVariableW(L"AI_WIN_UI_STYLES", nullptr);
        SetEnvironmentVariableW(L"AI_WIN_UI_MEASURE_DUMP", nullptr);
        SetEnvironmentVariableW(L"AI_WIN_UI_TEXT_DUMP", nullptr);
        OutputDebugStringW(L"[Env] AI_WIN_UI_IGNORE_ENV: cleared sticky layout/chrome/size/theme/styles/dumps\n");
    }

    void LogEffectiveEnv() const {
        auto line = [](const wchar_t* key, const std::wstring& value) {
            std::wstring msg = L"[Env] ";
            msg += key;
            msg += L"=";
            msg += value.empty() ? L"(default)" : value;
            msg += L"\n";
            OutputDebugStringW(msg.c_str());
        };
        line(L"AI_WIN_UI_LAYOUT", GetEnvironmentValue(L"AI_WIN_UI_LAYOUT"));
        line(L"AI_WIN_UI_CHROME", GetEnvironmentValue(L"AI_WIN_UI_CHROME"));
        line(L"AI_WIN_UI_RENDERER", GetEnvironmentValue(L"AI_WIN_UI_RENDERER"));
        line(L"AI_WIN_UI_SIZE", GetEnvironmentValue(L"AI_WIN_UI_SIZE"));
        line(L"AI_WIN_UI_THEME", GetEnvironmentValue(L"AI_WIN_UI_THEME"));
        line(L"AI_WIN_UI_STYLES", GetEnvironmentValue(L"AI_WIN_UI_STYLES"));
        line(L"AI_WIN_UI_QUIT_AFTER_MS", GetEnvironmentValue(L"AI_WIN_UI_QUIT_AFTER_MS"));
        line(L"AI_WIN_UI_DISABLE_WINDOW_SCROLL",
             m_windowScrollEnabled ? L"0 (legacy window scroll ON)" : L"1 (window scroll OFF)");
        {
            std::wstring msg = L"[Env] activeLayout=";
            if (m_activeLayoutPath.empty()) {
                msg += L"(none)";
            } else {
                msg += Utf8ToWide(m_activeLayoutPath);
            }
            msg += L" chromeMode=";
            msg += m_windowChrome.IsLayered() ? L"layered"
                : (m_windowChrome.IsCustom() ? L"custom" : L"system");
            msg += L" backend=";
            msg += RendererBackendDisplayName(m_activeRendererBackend);
            msg += L" themeFile=";
            msg += m_loadedThemePath.empty() ? L"(none)" : Utf8ToWide(m_loadedThemePath);
            msg += L" stylesFiles=";
            if (m_loadedStylesPaths.empty()) {
                msg += L"(none)";
            } else {
                for (size_t i = 0; i < m_loadedStylesPaths.size(); ++i) {
                    if (i) {
                        msg += L";";
                    }
                    msg += Utf8ToWide(m_loadedStylesPaths[i]);
                }
            }
            msg += L"\n";
            OutputDebugStringW(msg.c_str());
        }
    }

    UIElement* GetFirstFocusable() const {
        const auto focusables = CollectFocusableElements();
        return focusables.empty() ? nullptr : focusables.front();
    }

    void BuildDefaultUI() {
        auto panel = std::make_unique<Panel>();
        panel->background = ColorFromHex(0x171717);
        panel->padding = {24, 24, 24, 24};
        panel->spacing = 10.0f;

        auto title = std::make_unique<Label>(L"DirectUI-style Retained UI Tree");
        auto desc = std::make_unique<Label>(L"Backend: Win32 + abstract renderer/text services");

        auto input = std::make_unique<TextInput>(L"Hello, world!");
        input->SetFixedWidth(360.0f);

        auto btn1 = std::make_unique<Button>(L"Primary Action");
        btn1->SetOnClick([this]() {
            SetWindowTextW(m_hwnd, L"Clicked: Primary Action");
        });

        auto btn2 = std::make_unique<Button>(L"Secondary Action");
        btn2->SetOnClick([this]() {
            SetWindowTextW(m_hwnd, L"Clicked: Secondary Action");
        });

        panel->AddChild(std::move(title));
        panel->AddChild(std::move(desc));
        panel->AddChild(std::move(input));
        panel->AddChild(std::move(btn1));
        panel->AddChild(std::move(btn2));
        panel->SetContext(&m_uiContext);
        m_root = std::move(panel);
    }

    void OnResize() {
        if (!m_isInitialized) {
            return;
        }
        RECT rc{};
        GetClientRect(m_hwnd, &rc);
        m_viewportWidth = static_cast<float>(std::max<LONG>(0, rc.right - rc.left));
        m_viewportHeight = static_cast<float>(std::max<LONG>(0, rc.bottom - rc.top));
        m_uiContext.viewportWidth = m_viewportWidth;
        m_uiContext.viewportHeight = m_viewportHeight;
        if (m_renderer) {
            m_renderer->Resize(static_cast<UINT>(m_viewportWidth), static_cast<UINT>(m_viewportHeight));
        }
        SyncLayeredShellForWindowState();
        UpdateLayout();
        InvalidateRect(m_hwnd, nullptr, FALSE);
    }

    // When layered is maximized, collapse floating margins / radius so the card
    // fills the work area cleanly; restore the floating shell when restored.
    void SyncLayeredShellForWindowState() {
        if (!m_windowChrome.IsLayered() || !m_root) {
            return;
        }
        auto* rootPanel = dynamic_cast<Panel*>(m_root.get());
        if (!rootPanel) {
            return;
        }
        if (!m_layeredShellCaptured) {
            m_layeredRootPadding = rootPanel->padding;
            if (!rootPanel->Children().empty()) {
                if (auto* card = dynamic_cast<Panel*>(rootPanel->Children().front().get())) {
                    m_layeredCardRadius = card->cornerRadius;
                }
            }
            m_layeredShellCaptured = true;
        }

        const bool maximized = m_hwnd && IsZoomed(m_hwnd);
        if (maximized) {
            rootPanel->padding = Thickness{};
            if (!rootPanel->Children().empty()) {
                if (auto* card = dynamic_cast<Panel*>(rootPanel->Children().front().get())) {
                    card->cornerRadius = 0.0f;
                }
            }
        } else {
            rootPanel->padding = m_layeredRootPadding;
            if (!rootPanel->Children().empty()) {
                if (auto* card = dynamic_cast<Panel*>(rootPanel->Children().front().get())) {
                    card->cornerRadius = m_layeredCardRadius;
                }
            }
        }
    }

    static Rect ExpandRect(const Rect& rect, float amount) {
        return Rect::Make(
            rect.left - amount,
            rect.top - amount,
            rect.right + amount,
            rect.bottom + amount);
    }

    // Soft multi-pass shadow under the floating card (layered present only).
    void DrawLayeredCardShadow(IRenderer& renderer) const {
        if (!m_root || m_root->Children().empty()) {
            return;
        }
        const UIElement* card = m_root->Children().front().get();
        if (!card) {
            return;
        }
        const Rect bounds = card->Bounds();
        if (bounds.Width() <= 1.0f || bounds.Height() <= 1.0f) {
            return;
        }

        float radius = 18.0f * m_uiContext.dpiScale;
        if (const auto* cardPanel = dynamic_cast<const Panel*>(card)) {
            radius = cardPanel->cornerRadius * m_uiContext.dpiScale;
        }
        // Skip shadow when maximized (full-bleed card).
        if (radius < 0.5f) {
            return;
        }

        for (int i = 5; i >= 1; --i) {
            const float expand = static_cast<float>(i) * 3.0f * m_uiContext.dpiScale;
            const float drop = static_cast<float>(i) * 1.2f * m_uiContext.dpiScale;
            const float alpha = 0.035f * static_cast<float>(6 - i);
            Rect shadow = ExpandRect(bounds, expand);
            shadow.top += drop;
            shadow.bottom += drop;
            renderer.FillRoundedRect(shadow, Color{0.0f, 0.0f, 0.0f, alpha}, radius + expand * 0.35f);
        }
    }

    void UpdateLayout() {
        if (m_root) {
            m_root->Measure(m_viewportWidth, kUnboundedLayoutHeight);
            m_contentHeight = std::max(m_viewportHeight, m_root->DesiredSize().height);
            ClampScrollOffset();
            m_root->Arrange(Rect::Make(
                0.0f,
                -m_scrollOffset,
                m_viewportWidth,
                m_contentHeight - m_scrollOffset));
            UpdateImeWindow();
            RefreshChromeHitRegions();
        } else {
            m_contentHeight = m_viewportHeight;
            m_windowChrome.ClearHitRegions();
        }
        UpdateScrollBar();
    }

    // AI_WIN_UI_MEASURE_DUMP=1 �� measure_dump.ndjson next to exe
    // AI_WIN_UI_MEASURE_DUMP=path\to\out.ndjson �� explicit path
    void MaybeDumpMeasureTree() {
        const std::wstring dumpEnv = GetEnvironmentValue(L"AI_WIN_UI_MEASURE_DUMP");
        if (dumpEnv.empty() || !m_root) {
            return;
        }

        std::wstring outPathW = dumpEnv;
        if (dumpEnv == L"1" || dumpEnv == L"true" || dumpEnv == L"TRUE") {
            const std::wstring dir = GetExecutableDirectory();
            outPathW = dir.empty() ? L"measure_dump.ndjson" : (dir + L"\\measure_dump.ndjson");
        }

        // Prefer narrow path for ofstream portability (ACP on Windows is fine for ASCII paths).
        const std::string outPath = Utf16ToUtf8(outPathW);
        std::ofstream out(outPath.empty() ? "measure_dump.ndjson" : outPath, std::ios::binary | std::ios::trunc);
        if (!out) {
            OutputDebugStringW((L"[MeasureDump] failed to open " + outPathW + L"\n").c_str());
            return;
        }

        // One JSON object per line (NDJSON), stable path + desired + arranged bounds.
        std::function<void(const UIElement*, const std::string&, int)> walk;
        walk = [&](const UIElement* el, const std::string& parentPath, int index) {
            if (!el) {
                return;
            }
            const std::string segment = el->Name().empty()
                ? ("n" + std::to_string(index))
                : el->Name();
            const std::string path = parentPath.empty() ? segment : (parentPath + "/" + segment);
            const Size desired = el->DesiredSize();
            const Rect bounds = el->Bounds();
            out << "{\"path\":\"" << path
                << "\",\"dw\":" << desired.width
                << ",\"dh\":" << desired.height
                << ",\"x\":" << bounds.left
                << ",\"y\":" << bounds.top
                << ",\"w\":" << bounds.Width()
                << ",\"h\":" << bounds.Height()
                << "}\n";
            int childIndex = 0;
            for (const auto& child : el->Children()) {
                walk(child.get(), path, childIndex++);
            }
        };
        walk(m_root.get(), "", 0);
        out.flush();
        OutputDebugStringW((L"[MeasureDump] wrote " + outPathW + L"\n").c_str());
    }

    // AI_WIN_UI_TEXT_DUMP=1|path �� NDJSON of Label measure sizes (Wave1 R/A6).
    void MaybeDumpTextMetrics() {
        const std::wstring dumpEnv = GetEnvironmentValue(L"AI_WIN_UI_TEXT_DUMP");
        if (dumpEnv.empty() || !m_root || !m_uiContext.textMeasurer) {
            return;
        }

        std::wstring outPathW = dumpEnv;
        if (EnvTruthy(dumpEnv)) {
            const std::wstring dir = GetExecutableDirectory();
            outPathW = dir.empty() ? L"text_dump.ndjson" : (dir + L"\\text_dump.ndjson");
        }
        const std::string outPath = Utf16ToUtf8(outPathW);
        std::ofstream out(outPath.empty() ? "text_dump.ndjson" : outPath, std::ios::binary | std::ios::trunc);
        if (!out) {
            OutputDebugStringW((L"[TextDump] failed to open " + outPathW + L"\n").c_str());
            return;
        }

        std::function<void(UIElement*, const std::string&, int)> walk;
        walk = [&](UIElement* el, const std::string& parentPath, int index) {
            if (!el) {
                return;
            }
            const std::string segment = el->Name().empty()
                ? ("n" + std::to_string(index))
                : el->Name();
            const std::string path = parentPath.empty() ? segment : (parentPath + "/" + segment);

            if (auto* label = dynamic_cast<Label*>(el)) {
                const float fontSize = label->ScaleValue(label->FontSize());
                const float maxW = std::max(1.0f, label->Bounds().Width());
                const Size wrap = m_uiContext.textMeasurer->MeasureText(
                    label->Text().c_str(),
                    static_cast<uint32_t>(label->Text().size()),
                    fontSize,
                    maxW,
                    TextWrapMode::Wrap);
                const Size nowrap = m_uiContext.textMeasurer->MeasureText(
                    label->Text().c_str(),
                    static_cast<uint32_t>(label->Text().size()),
                    fontSize,
                    maxW,
                    TextWrapMode::NoWrap);
                out << "{\"path\":\"" << path
                    << "\",\"font\":" << fontSize
                    << ",\"maxW\":" << maxW
                    << ",\"wrapW\":" << wrap.width
                    << ",\"wrapH\":" << wrap.height
                    << ",\"nowrapW\":" << nowrap.width
                    << ",\"nowrapH\":" << nowrap.height
                    << ",\"boxW\":" << label->Bounds().Width()
                    << ",\"boxH\":" << label->Bounds().Height()
                    << "}\n";
            }

            int childIndex = 0;
            for (const auto& child : el->Children()) {
                walk(child.get(), path, childIndex++);
            }
        };
        walk(m_root.get(), "", 0);
        out.flush();
        OutputDebugStringW((L"[TextDump] wrote " + outPathW + L"\n").c_str());
    }

    void OnPaint() {
        PAINTSTRUCT ps{};
        BeginPaint(m_hwnd, &ps);

        if (m_isInitialized && m_renderer) {
            // Layered present clears to transparent so desktop shows in empty regions.
            const Color clearColor = m_windowChrome.IsLayered()
                ? Color{0.0f, 0.0f, 0.0f, 0.0f}
                : ColorFromHex(0x101010);
            m_renderer->BeginFrame(clearColor);
            if (m_windowChrome.IsLayered()) {
                DrawLayeredCardShadow(*m_renderer);
            }
            if (m_root) {
                m_root->Render(*m_renderer);
                // Popups (ComboBox dropdown, etc.) above the rest of the tree.
                m_root->RenderOverlay(*m_renderer);
            }
            if (m_windowChrome.IsCustom() && m_viewportWidth > 1.0f && m_viewportHeight > 1.0f) {
                // Dim caption on inactive windows. Prefer the card title strip when layered
                // so transparent margins stay clear.
                if (!m_uiContext.windowActive && m_captionBandHeight > 0.0f) {
                    Rect dimRect = Rect::Make(0.0f, 0.0f, m_viewportWidth, m_captionBandHeight);
                    if (m_windowChrome.IsLayered() && m_root && !m_root->Children().empty()) {
                        const Rect card = m_root->Children().front()->Bounds();
                        dimRect = Rect::Make(
                            card.left,
                            card.top,
                            card.right,
                            std::min(card.bottom, card.top + m_captionBandHeight));
                    }
                    m_renderer->FillRect(dimRect, Color{0.0f, 0.0f, 0.0f, 0.28f});
                }
                // Opaque custom chrome needs an edge; layered shapes define their own outline in UI.
                if (!m_windowChrome.IsLayered()) {
                    const Rect edge = Rect::Make(0.5f, 0.5f, m_viewportWidth - 0.5f, m_viewportHeight - 0.5f);
                    const Color edgeColor = m_uiContext.windowActive
                        ? ColorFromHex(0x2A3A4C)
                        : ColorFromHex(0x1A2430);
                    m_renderer->DrawRect(edge, edgeColor, 1.0f);
                }
            }
            m_renderer->EndFrame();
        }

        EndPaint(m_hwnd, &ps);
    }

    void SetWindowActive(bool active) {
        if (m_uiContext.windowActive == active) {
            return;
        }
        m_uiContext.windowActive = active;
        if (m_hwnd) {
            InvalidateRect(m_hwnd, nullptr, FALSE);
        }
    }

    void OnMouseMove(LPARAM lParam) {
        if (!m_root) {
            return;
        }
        EnsureMouseLeaveTracking();
        const float x = static_cast<float>(GET_X_LPARAM(lParam));
        const float y = static_cast<float>(GET_Y_LPARAM(lParam));
        bool handled = false;
        if (m_mouseCaptureTarget) {
            handled = m_mouseCaptureTarget->OnMouseMove(x, y);
        } else if (UIElement* overlay = m_root->FindOverlayHitAt(x, y)) {
            handled = overlay->OnMouseMove(x, y);
        } else {
            handled = m_root->OnMouseMove(x, y);
        }
        if (handled) {
            InvalidateRect(m_hwnd, nullptr, FALSE);
        }
    }

    void OnMouseDown(LPARAM lParam) {
        if (!m_root) {
            return;
        }
        const float x = static_cast<float>(GET_X_LPARAM(lParam));
        const float y = static_cast<float>(GET_Y_LPARAM(lParam));

        // Expanded popups (ComboBox list) win over normal tree z-order.
        UIElement* overlayHit = m_root->FindOverlayHitAt(x, y);

        // Click outside any open overlay �� dismiss (keep header clicks for toggle).
        if (!overlayHit) {
            if (m_root->DismissOverlaysAt(x, y)) {
                InvalidateRect(m_hwnd, nullptr, FALSE);
            }
        }
        UIElement* focusTarget = overlayHit
            ? (overlayHit->IsFocusable() ? overlayHit : overlayHit->FindFocusableAt(x, y))
            : m_root->FindFocusableAt(x, y);
        if (!focusTarget && overlayHit && overlayHit->IsFocusable()) {
            focusTarget = overlayHit;
        }
        if (!focusTarget) {
            SetFocusedElement(nullptr);
        } else {
            SetFocusedElement(focusTarget);
        }

        if (overlayHit) {
            m_mouseCaptureTarget = overlayHit;
            if (overlayHit->OnMouseDown(x, y)) {
                UpdateImeWindow();
                InvalidateRect(m_hwnd, nullptr, FALSE);
            }
            return;
        }

        m_mouseCaptureTarget = m_root->FindHitElementAt(x, y);
        if (m_root->OnMouseDown(x, y)) {
            UpdateImeWindow();
            InvalidateRect(m_hwnd, nullptr, FALSE);
        }
    }

    void OnMouseUp(LPARAM lParam) {
        if (!m_root) {
            return;
        }
        const float x = static_cast<float>(GET_X_LPARAM(lParam));
        const float y = static_cast<float>(GET_Y_LPARAM(lParam));

        bool handled = false;
        if (m_mouseCaptureTarget) {
            handled = m_mouseCaptureTarget->OnMouseUp(x, y);
            m_mouseCaptureTarget = nullptr;
        } else {
            handled = m_root->OnMouseUp(x, y);
        }

        if (handled) {
            UpdateImeWindow();
            InvalidateRect(m_hwnd, nullptr, FALSE);
        }
    }

    void OnMouseLeave() {
        m_isTrackingMouseLeave = false;
        if (m_root && m_root->OnMouseLeave()) {
            InvalidateRect(m_hwnd, nullptr, FALSE);
        }
    }

    void OnMouseWheel(WPARAM wParam, LPARAM lParam) {
        const float delta = static_cast<float>(GET_WHEEL_DELTA_WPARAM(wParam)) / static_cast<float>(WHEEL_DELTA);
        if (delta == 0.0f) {
            return;
        }

        // Prefer scrolling the control under the cursor (ListView/TreeView pill bars).
        POINT screen{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
        POINT client = screen;
        ScreenToClient(m_hwnd, &client);
        const float x = static_cast<float>(client.x);
        const float y = static_cast<float>(client.y);
        const bool shiftHeld = (GetKeyState(VK_SHIFT) & 0x8000) != 0;

        if (m_root && m_root->OnMouseWheel(delta, x, y, shiftHeld)) {
            InvalidateRect(m_hwnd, nullptr, FALSE);
            return;
        }

        // Legacy host scroll �� opt out with AI_WIN_UI_DISABLE_WINDOW_SCROLL=1
        if (!m_windowScrollEnabled) {
            return;
        }
        const float wheelStep = std::max(24.0f, 56.0f * m_uiContext.dpiScale);
        ScrollBy(-delta * wheelStep);
    }

    void EnsureElementVisible(UIElement* element) {
        if (!element) {
            return;
        }

        const Rect bounds = element->Bounds();
        float targetOffset = m_scrollOffset;

        if (bounds.top < 0.0f) {
            targetOffset += bounds.top;
        } else if (bounds.bottom > m_viewportHeight) {
            targetOffset += bounds.bottom - m_viewportHeight;
        }

        SetScrollOffset(targetOffset);
    }

    void SetFocusedElement(UIElement* element) {
        if (m_focusedElement == element) {
            return;
        }
        if (m_focusedElement) {
            m_focusedElement->OnBlur();
        }
        m_focusedElement = element;
        if (m_focusedElement) {
            m_focusedElement->OnFocus();
            EnsureElementVisible(m_focusedElement);
            UpdateImeWindow();
        }
        InvalidateRect(m_hwnd, nullptr, FALSE);
    }

    std::vector<UIElement*> CollectFocusableElements() const {
        std::vector<UIElement*> focusables;
        if (m_root) {
            m_root->CollectFocusable(focusables);
        }
        return focusables;
    }

    UIElement* FindFocusableAt(float x, float y) const {
        return m_root ? m_root->FindFocusableAt(x, y) : nullptr;
    }

    void MoveFocus(bool reverse) {
        const auto focusables = CollectFocusableElements();
        if (focusables.empty()) {
            return;
        }

        auto it = std::find(focusables.begin(), focusables.end(), m_focusedElement);
        size_t nextIndex = 0;
        if (it != focusables.end()) {
            if (reverse) {
                nextIndex = (it == focusables.begin()) ? focusables.size() - 1 : static_cast<size_t>(std::distance(focusables.begin(), it) - 1);
            } else {
                nextIndex = (static_cast<size_t>(std::distance(focusables.begin(), it)) + 1) % focusables.size();
            }
        }

        SetFocusedElement(focusables[nextIndex]);
    }

    bool OnKeyDown(WPARAM wParam, LPARAM lParam) {
        if (wParam == VK_TAB) {
            MoveFocus((GetKeyState(VK_SHIFT) & 0x8000) != 0);
            UpdateImeWindow();
            return true;
        }

        if (m_focusedElement && m_focusedElement->OnKeyDown(wParam, lParam)) {
            UpdateImeWindow();
            return true;
        }

        // Host-level Esc: collapse any remaining open overlays (C2).
        if (wParam == VK_ESCAPE && m_root && m_root->DismissAllOverlays()) {
            InvalidateRect(m_hwnd, nullptr, FALSE);
            return true;
        }

        switch (wParam) {
            case VK_PRIOR:
                return ScrollBy(-std::max(48.0f, m_viewportHeight * 0.85f));
            case VK_NEXT:
                return ScrollBy(std::max(48.0f, m_viewportHeight * 0.85f));
            case VK_HOME:
                return SetScrollOffset(0.0f);
            case VK_END:
                return SetScrollOffset(MaxScrollOffset());
            default:
                break;
        }
        return false;
    }

    bool OnKeyUp(WPARAM wParam, LPARAM lParam) {
        if (m_focusedElement && m_focusedElement->OnKeyUp(wParam, lParam)) {
            return true;
        }
        return false;
    }

    bool OnTextChar(wchar_t ch) {
        if (!m_focusedElement || !m_focusedElement->OnChar(ch)) {
            return false;
        }
        UpdateImeWindow();
        return true;
    }

    bool OnUnicodeChar(WPARAM wParam) {
        if (wParam == UNICODE_NOCHAR) {
            return true;
        }
        if (wParam == 0 || wParam > 0x10FFFF) {
            return false;
        }

        if (wParam <= 0xFFFF) {
            return OnTextChar(static_cast<wchar_t>(wParam));
        }

        const uint32_t codepoint = static_cast<uint32_t>(wParam - 0x10000);
        const wchar_t high = static_cast<wchar_t>(0xD800 + (codepoint >> 10));
        const wchar_t low = static_cast<wchar_t>(0xDC00 + (codepoint & 0x3FF));
        const bool insertedHigh = OnTextChar(high);
        const bool insertedLow = OnTextChar(low);
        return insertedHigh || insertedLow;
    }

    bool ScrollBy(float delta) {
        return SetScrollOffset(m_scrollOffset + delta);
    }

    bool SetScrollOffset(float offset) {
        const float previousOffset = m_scrollOffset;
        m_scrollOffset = std::clamp(offset, 0.0f, MaxScrollOffset());
        if (std::abs(m_scrollOffset - previousOffset) < 0.5f) {
            return false;
        }

        if (m_root) {
            m_root->Arrange(Rect::Make(
                0.0f,
                -m_scrollOffset,
                m_viewportWidth,
                m_contentHeight - m_scrollOffset));
            UpdateImeWindow();
        }
        UpdateScrollBar();
        InvalidateRect(m_hwnd, nullptr, FALSE);
        return true;
    }

    void UpdateImeWindow() {
        if (!m_hwnd || !m_focusedElement) {
            return;
        }

        Rect caret{};
        if (!m_focusedElement->GetTextInputCaretRect(caret)) {
            return;
        }

        HIMC imc = ImmGetContext(m_hwnd);
        if (!imc) {
            return;
        }

        // Caret top-left is the usual composition anchor (CJK IME).
        const POINT caretPoint{
            static_cast<LONG>(std::lround(caret.left)),
            static_cast<LONG>(std::lround(caret.top))
        };

        COMPOSITIONFORM composition{};
        composition.dwStyle = CFS_POINT;
        composition.ptCurrentPos = caretPoint;
        ImmSetCompositionWindow(imc, &composition);

        // Candidate list near caret; also exclude the focused control rect so the
        // list does not cover the text being edited (C3).
        CANDIDATEFORM candidatePos{};
        candidatePos.dwIndex = 0;
        candidatePos.dwStyle = CFS_CANDIDATEPOS;
        candidatePos.ptCurrentPos = POINT{
            static_cast<LONG>(std::lround(caret.left)),
            static_cast<LONG>(std::lround(caret.bottom + 2.0f))
        };
        ImmSetCandidateWindow(imc, &candidatePos);

        const Rect field = m_focusedElement->Bounds();
        CANDIDATEFORM candidateExclude{};
        candidateExclude.dwIndex = 0;
        candidateExclude.dwStyle = CFS_EXCLUDE;
        candidateExclude.ptCurrentPos = caretPoint;
        candidateExclude.rcArea = RECT{
            static_cast<LONG>(std::floor(field.left)),
            static_cast<LONG>(std::floor(field.top)),
            static_cast<LONG>(std::ceil(field.right)),
            static_cast<LONG>(std::ceil(field.bottom))
        };
        ImmSetCandidateWindow(imc, &candidateExclude);

        // Prefer a known UI face; size tracks caret height when available (C3).
        LOGFONTW lf{};
        const float caretH = std::max(0.0f, caret.Height());
        const float fontPx = caretH > 2.0f
            ? std::clamp(caretH * 0.85f, 12.0f, 28.0f)
            : std::max(12.0f, 16.0f * m_uiContext.dpiScale);
        lf.lfHeight = -static_cast<LONG>(std::lround(fontPx));
        lf.lfCharSet = DEFAULT_CHARSET;
        lf.lfQuality = CLEARTYPE_QUALITY;
        wcscpy_s(lf.lfFaceName, L"Segoe UI");
        ImmSetCompositionFontW(imc, &lf);

        ImmReleaseContext(m_hwnd, imc);
    }

    float MaxScrollOffset() const {
        return std::max(0.0f, m_contentHeight - m_viewportHeight);
    }

    void ClampScrollOffset() {
        m_scrollOffset = std::clamp(m_scrollOffset, 0.0f, MaxScrollOffset());
    }

    void UpdateScrollBar() const {
        if (!m_hwnd) {
            return;
        }
        if (!m_windowScrollEnabled) {
            ShowScrollBar(m_hwnd, SB_VERT, FALSE);
            return;
        }
        SCROLLINFO si{};
        si.cbSize = sizeof(si);
        si.fMask = SIF_PAGE | SIF_POS | SIF_RANGE;
        si.nMin = 0;
        si.nMax = std::max(0, static_cast<int>(std::ceil(m_contentHeight)) - 1);
        si.nPage = static_cast<UINT>(std::max(0.0f, m_viewportHeight));
        si.nPos = static_cast<int>(std::lround(m_scrollOffset));
        SetScrollInfo(m_hwnd, SB_VERT, &si, TRUE);
        ShowScrollBar(m_hwnd, SB_VERT, MaxScrollOffset() > 0.5f ? TRUE : FALSE);
    }

    bool OnVerticalScroll(WPARAM wParam) {
        SCROLLINFO si{};
        si.cbSize = sizeof(si);
        si.fMask = SIF_ALL;
        GetScrollInfo(m_hwnd, SB_VERT, &si);

        float targetOffset = m_scrollOffset;
        const float lineStep = std::max(16.0f, 40.0f * m_uiContext.dpiScale);
        const float pageStep = std::max(lineStep * 3.0f, m_viewportHeight * 0.85f);

        switch (LOWORD(wParam)) {
            case SB_LINEUP:
                targetOffset -= lineStep;
                break;
            case SB_LINEDOWN:
                targetOffset += lineStep;
                break;
            case SB_PAGEUP:
                targetOffset -= pageStep;
                break;
            case SB_PAGEDOWN:
                targetOffset += pageStep;
                break;
            case SB_THUMBTRACK:
            case SB_THUMBPOSITION:
                targetOffset = static_cast<float>(si.nTrackPos);
                break;
            case SB_TOP:
                targetOffset = 0.0f;
                break;
            case SB_BOTTOM:
                targetOffset = MaxScrollOffset();
                break;
            default:
                return false;
        }

        return SetScrollOffset(targetOffset);
    }

    void EnsureMouseLeaveTracking() {
        if (m_isTrackingMouseLeave) {
            return;
        }

        TRACKMOUSEEVENT tme{};
        tme.cbSize = sizeof(tme);
        tme.dwFlags = TME_LEAVE;
        tme.hwndTrack = m_hwnd;
        if (TrackMouseEvent(&tme)) {
            m_isTrackingMouseLeave = true;
        }
    }

    static LRESULT CALLBACK WndProcSetup(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        if (msg == WM_NCCREATE) {
            const auto* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
            auto* app = static_cast<UiHost*>(cs->lpCreateParams);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
            SetWindowLongPtrW(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&UiHost::WndProcThunk));
            return app->HandleMessage(hwnd, msg, wParam, lParam);
        }
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    static LRESULT CALLBACK WndProcThunk(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        auto* app = reinterpret_cast<UiHost*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        return app ? app->HandleMessage(hwnd, msg, wParam, lParam) : DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    LRESULT HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        switch (msg) {
            case WM_NCCALCSIZE: {
                bool handled = false;
                const LRESULT result = m_windowChrome.HandleNcCalcSize(hwnd, wParam, lParam, handled);
                if (handled) {
                    return result;
                }
                break;
            }
            case WM_NCHITTEST: {
                bool handled = false;
                const LRESULT result = m_windowChrome.HandleNcHitTest(hwnd, lParam, handled);
                if (handled) {
                    // Resize grips always win. Caption / system buttons are interactive
                    // even when drawn with transparent fills (glyph-only caption buttons).
                    // Only empty transparent pixels outside chrome regions pass through.
                    const bool isResizeBorder =
                        result == HTLEFT || result == HTRIGHT || result == HTTOP || result == HTBOTTOM ||
                        result == HTTOPLEFT || result == HTTOPRIGHT || result == HTBOTTOMLEFT ||
                        result == HTBOTTOMRIGHT;
                    if (!isResizeBorder && m_windowChrome.IsLayered() && m_renderer) {
                        POINT screenPt{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
                        POINT clientPt = screenPt;
                        ScreenToClient(hwnd, &clientPt);
                        if (!m_windowChrome.IsChromeInteractiveHit(clientPt) &&
                            !m_renderer->SampleOpaque(clientPt.x, clientPt.y, 16)) {
                            return HTTRANSPARENT;
                        }
                    }
                    return result;
                }
                break;
            }
            case WM_GETMINMAXINFO: {
                bool handled = false;
                const LRESULT result = m_windowChrome.HandleGetMinMaxInfo(hwnd, lParam, handled);
                if (handled) {
                    return result;
                }
                break;
            }
            case WM_ACTIVATE: {
                const bool active = (LOWORD(wParam) != WA_INACTIVE);
                SetWindowActive(active);
                break;
            }
            case WM_NCACTIVATE: {
                // Keep custom chrome in sync even when only non-client activation changes.
                if (m_windowChrome.IsCustom()) {
                    SetWindowActive(wParam != FALSE);
                    // Still let DefWindowProc paint default NC (none in custom) semantics.
                }
                break;
            }
            case WM_DPICHANGED: {
                // Fired when the window moves across monitors with different DPI.
                UpdateDpiContext(HIWORD(wParam));
                const auto* suggestedRect = reinterpret_cast<RECT*>(lParam);
                if (suggestedRect) {
                    SetWindowPos(
                        hwnd,
                        nullptr,
                        suggestedRect->left,
                        suggestedRect->top,
                        suggestedRect->right - suggestedRect->left,
                        suggestedRect->bottom - suggestedRect->top,
                        SWP_NOZORDER | SWP_NOACTIVATE);
                }
                OnResize();
                return 0;
            }
            case WM_DISPLAYCHANGE:
                // Display topology / resolution change: refresh layout against
                // the monitor that currently hosts the window.
                if (m_hwnd) {
                    UpdateDpiContext(GetWindowDpiWithFallback(m_hwnd));
                    OnResize();
                }
                return 0;
            case WM_SIZE:
                OnResize();
                SyncCaptionMaximizeGlyphs();
                return 0;
            case WM_SETCURSOR: {
                if (LOWORD(lParam) == HTCLIENT) {
                    POINT pt;
                    GetCursorPos(&pt);
                    ScreenToClient(hwnd, &pt);
                    // Handle scrolling offset if needed
                    float x = static_cast<float>(pt.x) / m_uiContext.dpiScale;
                    float y = static_cast<float>(pt.y) / m_uiContext.dpiScale;
                    if (m_windowScrollEnabled) {
                        y += m_scrollOffset;
                    }
                    UIElement* hit = m_root ? m_root->FindOverlayHitAt(x, y) : nullptr;
                    if (!hit && m_root) {
                        hit = m_root->FindHitElementAt(x, y);
                    }
                    if (hit) {
                        CursorType cursor = hit->GetCursor();
                        if (cursor != CursorType::Auto) {
                            LPCWSTR sysCursor = IDC_ARROW;
                            switch (cursor) {
                                case CursorType::Hand: sysCursor = IDC_HAND; break;
                                case CursorType::IBeam: sysCursor = IDC_IBEAM; break;
                                case CursorType::Cross: sysCursor = IDC_CROSS; break;
                                case CursorType::Wait: sysCursor = IDC_WAIT; break;
                                case CursorType::SizeWE: sysCursor = IDC_SIZEWE; break;
                                case CursorType::SizeNS: sysCursor = IDC_SIZENS; break;
                                case CursorType::SizeNWSE: sysCursor = IDC_SIZENWSE; break;
                                case CursorType::SizeNESW: sysCursor = IDC_SIZENESW; break;
                                case CursorType::SizeAll: sysCursor = IDC_SIZEALL; break;
                                case CursorType::Forbidden: sysCursor = IDC_NO; break;
                                default: break;
                            }
                            SetCursor(LoadCursorW(nullptr, sysCursor));
                            return TRUE;
                        }
                    }
                }
                break; // Fallback to DefWindowProc
            }
            case WM_MOUSEMOVE:
                OnMouseMove(lParam);
                return 0;
            case WM_MOUSELEAVE:
                OnMouseLeave();
                return 0;
            case WM_MOUSEWHEEL:
                OnMouseWheel(wParam, lParam);
                return 0;
            case WM_VSCROLL:
                if (OnVerticalScroll(wParam)) {
                    return 0;
                }
                break;
            case WM_LBUTTONDOWN:
                SetCapture(hwnd);
                OnMouseDown(lParam);
                return 0;
            case WM_LBUTTONUP:
                ReleaseCapture();
                OnMouseUp(lParam);
                return 0;
            case WM_SETFOCUS:
                if (!m_focusedElement) {
                    SetFocusedElement(GetFirstFocusable());
                }
                return 0;
            case WM_KILLFOCUS:
                SetFocusedElement(nullptr);
                return 0;
            case WM_KEYDOWN:
                if (OnKeyDown(wParam, lParam)) {
                    InvalidateRect(m_hwnd, nullptr, FALSE);
                    return 0;
                }
                break;
            case WM_CHAR:
            case WM_IME_CHAR:
                if (OnTextChar(static_cast<wchar_t>(wParam))) {
                    InvalidateRect(m_hwnd, nullptr, FALSE);
                    return 0;
                }
                break;
            case WM_UNICHAR:
                if (OnUnicodeChar(wParam)) {
                    InvalidateRect(m_hwnd, nullptr, FALSE);
                    return 0;
                }
                break;
            case WM_IME_STARTCOMPOSITION:
            case WM_IME_COMPOSITION:
                UpdateImeWindow();
                break;
            case WM_KEYUP:
                if (OnKeyUp(wParam, lParam)) {
                    UpdateImeWindow();
                    InvalidateRect(m_hwnd, nullptr, FALSE);
                    return 0;
                }
                break;
            case WM_TIMER:
                if (wParam == kUiTimerId) {
                    if (m_root && m_root->OnTimer(wParam)) {
                        InvalidateRect(m_hwnd, nullptr, FALSE);
                    }
                    return 0;
                }
                if (wParam == kQuitTimerId) {
                    KillTimer(hwnd, kQuitTimerId);
                    PostMessageW(hwnd, WM_CLOSE, 0, 0);
                    return 0;
                }
                break;
            case WM_PAINT:
                OnPaint();
                return 0;
            case WM_ERASEBKGND:
                return 1;
            case ai_win_ui::kMsgOpenDemoHost:
                if (ai_win_ui::Application::HasPendingOpen()) {
                    ai_win_ui::Application::HasPendingOpen() = false;
                    OpenHostOptions pending = ai_win_ui::Application::PendingOpen();
                    const bool ok = OpenSecondaryHost(m_instance, pending);
                    if (m_hwnd) {
                        std::wstring title = ok ? L"Opened: " : L"Failed: ";
                        title += pending.layout;
                        SetWindowTextW(m_hwnd, title.c_str());
                    }
                }
                return 0;
            case WM_DESTROY:
                if (!m_tearingDown) {
                    KillTimer(hwnd, kUiTimerId);
                    KillTimer(hwnd, kQuitTimerId);
                }
                SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
                m_hwnd = nullptr;
                m_isInitialized = false;
                m_closed = true;
                ai_win_ui::Application::OnHostDestroyed();
                // Only quit the process when the last host closes.
                if (ai_win_ui::Application::LiveHostCount() <= 0) {
                    PostQuitMessage(0);
                }
                return 0;
            default:
                return DefWindowProcW(hwnd, msg, wParam, lParam);
        }
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    HINSTANCE m_instance = nullptr;
    int m_cmdShow = SW_SHOWNORMAL;
    bool m_isSecondary = false;
    bool m_isEmbeddedChild = false;
    bool m_closed = false;
    bool m_tearingDown = false;
    HWND m_hwnd = nullptr;
    HWND m_parentHwnd = nullptr;
    UIContext m_uiContext{};
    WindowChrome m_windowChrome;
    bool m_chromeEnvForced = false;
    float m_captionBandHeight = 44.0f;
    bool m_layeredShellCaptured = false;
    Thickness m_layeredRootPadding{28.0f, 28.0f, 28.0f, 28.0f};
    float m_layeredCardRadius = 18.0f;
    std::unique_ptr<IRenderer> m_renderer;
    std::unique_ptr<ITextMeasurer> m_textMeasurer;
    std::unique_ptr<ILayoutEngine> m_layoutEngine;
    std::unique_ptr<IResourceProvider> m_resourceProvider;
    std::unique_ptr<Theme> m_theme;
    std::unique_ptr<StyleCatalog> m_styleCatalog;
    std::string m_loadedThemePath;
    std::vector<std::string> m_loadedStylesPaths;
    std::string m_activeLayoutPath;
    RendererBackend m_requestedRendererBackend = RendererBackend::Direct2D;
    RendererBackend m_activeRendererBackend = RendererBackend::Direct2D;
    std::unique_ptr<UIElement> m_root;
    UIElement* m_focusedElement = nullptr;
    UIElement* m_mouseCaptureTarget = nullptr;
    float m_viewportWidth = 0.0f;
    float m_viewportHeight = 0.0f;
    float m_contentHeight = 0.0f;
    float m_scrollOffset = 0.0f;
    // Legacy: translate entire root when content taller than client (prefer ScrollViewer).
    bool m_windowScrollEnabled = true;
    bool m_isInitialized = false;
    bool m_isTrackingMouseLeave = false;
    bool m_oleInitialized = false;
};

class HostDropTarget : public IDropTarget {
public:
    HostDropTarget(UiHost* host) : m_host(host) {}
    
    virtual ~HostDropTarget() = default;

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) override {
        if (riid == IID_IUnknown || riid == IID_IDropTarget) {
            *ppvObject = static_cast<IDropTarget*>(this);
            AddRef();
            return S_OK;
        }
        *ppvObject = nullptr;
        return E_NOINTERFACE;
    }
    
    ULONG STDMETHODCALLTYPE AddRef() override { return ++m_refCount; }
    
    ULONG STDMETHODCALLTYPE Release() override {
        ULONG count = --m_refCount;
        if (count == 0) delete this;
        return count;
    }
    
    HRESULT STDMETHODCALLTYPE DragEnter(IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect) override {
        m_files = ExtractFiles(pDataObj);
        return DragOver(grfKeyState, pt, pdwEffect);
    }
    
    HRESULT STDMETHODCALLTYPE DragOver(DWORD /*grfKeyState*/, POINTL pt, DWORD* pdwEffect) override {
        if (m_files.empty()) {
            *pdwEffect = DROPEFFECT_NONE;
            return S_OK;
        }
        
        POINT p{pt.x, pt.y};
        ScreenToClient(m_host->m_hwnd, &p);
        float x = static_cast<float>(p.x) / m_host->m_uiContext.dpiScale;
        float y = static_cast<float>(p.y) / m_host->m_uiContext.dpiScale;
        if (m_host->m_windowScrollEnabled) y += m_host->m_scrollOffset;
        
        UIElement* hit = m_host->m_root ? m_host->m_root->FindDropTargetAt(x, y) : nullptr;

        if (hit != m_currentDragElement) {
            if (m_currentDragElement) m_currentDragElement->OnDragLeave();
            m_currentDragElement = hit;
            if (m_currentDragElement) m_currentDragElement->OnDragEnter(m_files);
        }

        if (m_currentDragElement && m_currentDragElement->OnDragOver(x, y)) {
            *pdwEffect = DROPEFFECT_COPY;
        } else {
            *pdwEffect = DROPEFFECT_NONE;
        }
        return S_OK;
    }
    
    HRESULT STDMETHODCALLTYPE DragLeave() override {
        if (m_currentDragElement) {
            m_currentDragElement->OnDragLeave();
            m_currentDragElement = nullptr;
        }
        m_files.clear();
        return S_OK;
    }
    
    HRESULT STDMETHODCALLTYPE Drop(IDataObject* pDataObj, DWORD /*grfKeyState*/, POINTL pt, DWORD* pdwEffect) override {
        if (m_files.empty()) {
            *pdwEffect = DROPEFFECT_NONE;
            return S_OK;
        }
        POINT p{pt.x, pt.y};
        ScreenToClient(m_host->m_hwnd, &p);
        float x = static_cast<float>(p.x) / m_host->m_uiContext.dpiScale;
        float y = static_cast<float>(p.y) / m_host->m_uiContext.dpiScale;
        if (m_host->m_windowScrollEnabled) y += m_host->m_scrollOffset;
        
        if (m_currentDragElement) {
            if (m_currentDragElement->OnDrop(m_files, x, y)) {
                *pdwEffect = DROPEFFECT_COPY;
            } else {
                *pdwEffect = DROPEFFECT_NONE;
            }
            m_currentDragElement->OnDragLeave();
            m_currentDragElement = nullptr;
        } else {
            *pdwEffect = DROPEFFECT_NONE;
        }
        m_files.clear();
        return S_OK;
    }

private:
    std::vector<std::wstring> ExtractFiles(IDataObject* pDataObj) {
        std::vector<std::wstring> files;
        FORMATETC fmt = { CF_HDROP, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
        STGMEDIUM stg = { 0 };
        if (SUCCEEDED(pDataObj->GetData(&fmt, &stg))) {
            HDROP hDrop = static_cast<HDROP>(stg.hGlobal);
            UINT count = DragQueryFileW(hDrop, 0xFFFFFFFF, nullptr, 0);
            for (UINT i = 0; i < count; ++i) {
                UINT length = DragQueryFileW(hDrop, i, nullptr, 0);
                if (length > 0) {
                    std::wstring path(length, L'\0');
                    DragQueryFileW(hDrop, i, &path[0], length + 1);
                    files.push_back(path);
                }
            }
            ReleaseStgMedium(&stg);
        }
        return files;
    }

    UiHost* m_host;
    ULONG m_refCount = 1;
    UIElement* m_currentDragElement = nullptr;
    std::vector<std::wstring> m_files;
};

void RegisterHostDropTarget(UiHost* host) {
    // OLE STA is established in UiHost::Initialize(); here we only register the
    // target. RegisterDragDrop takes its own reference, so we release ours.
    if (!host || !host->GetHwnd()) {
        return;
    }
    IDropTarget* target = new HostDropTarget(host);
    RegisterDragDrop(host->GetHwnd(), target);
    target->Release();
}

void RevokeHostDropTarget(HWND hwnd) {
    if (hwnd) {
        RevokeDragDrop(hwnd);
    }
}

void PurgeClosedSecondaryHosts() {
    ai_win_ui::Application::PurgeClosedSecondaries(&UiHost::HostIsClosed, &UiHost::HostDestroy);
}

namespace ai_win_ui {

bool OpenSecondaryUiHost(HINSTANCE instance, const OpenHostOptions& options) {
    return UiHost::OpenSecondaryHost(instance, options);
}

bool OpenSecondaryUiHost(
    HINSTANCE instance,
    const wchar_t* layoutRelPath,
    const wchar_t* chromeMode,
    const wchar_t* sizeWh,
    RendererBackend renderer) {
    return UiHost::OpenSecondaryHost(instance, layoutRelPath, chromeMode, sizeWh, renderer);
}

void InitializeProcessDpiAwareness() {
    InitializeDpiAwareness();
}

const char* Version() {
    return AI_WIN_UI_VERSION_STRING;
}

int VersionNumber() {
    return AI_WIN_UI_VERSION;
}

bool OpenSecondaryHost(
    HINSTANCE instance,
    const wchar_t* layoutRelPath,
    const wchar_t* chromeMode,
    const wchar_t* sizeWh,
    Backend renderer) {
    return OpenSecondaryUiHost(
        instance,
        layoutRelPath,
        chromeMode,
        sizeWh,
        renderer == Backend::Skia ? RendererBackend::Skia : RendererBackend::Direct2D);
}

struct Host::Impl {
    std::unique_ptr<UiHost> host;
    bool ownPump = true;
};

std::unique_ptr<Host> Host::Create(HINSTANCE instance, const HostCreateInfo& info) {
    InitializeProcessDpiAwareness();

    OpenHostOptions options;
    if (info.layoutPath && info.layoutPath[0]) {
        options.layout = info.layoutPath;
    }
    if (info.chrome && info.chrome[0]) {
        options.chrome = info.chrome;
    }
    if (info.size && info.size[0]) {
        options.size = info.size;
    }
    options.renderer =
        info.renderer == Backend::Skia ? RendererBackend::Skia : RendererBackend::Direct2D;
    options.parent = info.parent;

    // Map structured options onto env consumed by UiHost::Initialize / BuildUI.
    UiHost::ApplyOptionsToEnvironment(options);
    if (info.parent && IsWindow(info.parent)) {
        // Child hosts fill parent unless an explicit size was requested.
        if (!info.size || !info.size[0]) {
            SetEnvironmentVariableW(L"AI_WIN_UI_SIZE", nullptr);
        }
        // Never inherit custom/layered chrome into a WS_CHILD surface.
        SetEnvironmentVariableW(L"AI_WIN_UI_CHROME", nullptr);
    }

    auto impl = std::make_unique<Impl>();
    impl->host = std::make_unique<UiHost>();
    impl->ownPump = info.ownMessagePump;
    impl->host->SetParentHwnd(info.parent);
    const int showCmd = info.showCommand != 0 ? info.showCommand : SW_SHOWNORMAL;
    if (!impl->host->Initialize(instance, showCmd)) {
        return nullptr;
    }
    return std::unique_ptr<Host>(new Host(std::move(impl)));
}

Host::Host(std::unique_ptr<Impl> impl) : m_impl(std::move(impl)) {}

Host::~Host() = default;

HWND Host::GetHwnd() const {
    return m_impl && m_impl->host ? m_impl->host->GetHwnd() : nullptr;
}

HWND Host::GetParentHwnd() const {
    return m_impl && m_impl->host ? m_impl->host->GetParentHwnd() : nullptr;
}

bool Host::IsClosed() const {
    return !m_impl || !m_impl->host || m_impl->host->IsClosed();
}

bool Host::IsEmbeddedChild() const {
    return m_impl && m_impl->host && m_impl->host->IsEmbeddedChild();
}

int Host::Run() {
    if (!m_impl || !m_impl->host) {
        return -1;
    }
    return m_impl->host->Run();
}

bool Host::ProcessMessage(MSG& msg) {
    if (!m_impl || !m_impl->host) {
        return false;
    }
    const HWND hwnd = m_impl->host->GetHwnd();
    if (!hwnd) {
        return false;
    }
    if (msg.hwnd != hwnd && !IsChild(hwnd, msg.hwnd)) {
        return false;
    }
    TranslateMessage(&msg);
    DispatchMessageW(&msg);
    return true;
}

bool Host::ResizeClient(int width, int height) {
    return m_impl && m_impl->host && m_impl->host->ResizeClient(width, height);
}

bool Host::FitToParent() {
    return m_impl && m_impl->host && m_impl->host->FitToParent();
}

} // namespace ai_win_ui
 
