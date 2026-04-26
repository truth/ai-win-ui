#include "layout_parser.h"
#include "layout_engine.h"
#include "renderer.h"
#include "resource_provider.h"
#include "text_measurer.h"
#include "theme.h"
#include "ui_context.h"
#include "ui.h"
#include "zip_resource_provider.h"

#include <shellscalingapi.h>
#include <Windowsx.h>
#include <algorithm>
#include <cmath>
#include <memory>
#include <string>
#include <vector>

namespace {

constexpr float kUnboundedLayoutHeight = 100000.0f;
constexpr UINT_PTR kUiTimerId = 1;
constexpr UINT kUiTimerIntervalMs = 16;

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

class App {
public:
    bool Initialize(HINSTANCE instance, int cmdShow) {
        const wchar_t* kClassName = L"AiWinUiWindowClass";

        WNDCLASSW wc{};
        wc.lpfnWndProc = &App::WndProcSetup;
        wc.hInstance = instance;
        wc.lpszClassName = kClassName;
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);

        if (!RegisterClassW(&wc)) {
            return false;
        }

        const UINT initialDpi = GetDpiForSystem();
        RECT initialRect{0, 0, 1000, 700};
        AdjustWindowRectExForDpi(&initialRect, WS_OVERLAPPEDWINDOW, FALSE, 0, initialDpi);

        m_hwnd = CreateWindowExW(
            0,
            kClassName,
            L"AI WinUI Renderer (DirectUI-style)",
            WS_OVERLAPPEDWINDOW | WS_VSCROLL,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            initialRect.right - initialRect.left,
            initialRect.bottom - initialRect.top,
            nullptr,
            nullptr,
            instance,
            this
        );

        if (!m_hwnd) {
            return false;
        }

        UpdateDpiContext(GetWindowDpiWithFallback(m_hwnd));

        m_requestedRendererBackend = ParseRendererBackend(GetEnvironmentValue(L"AI_WIN_UI_RENDERER"));
        if (!InitializeRenderer()) {
            return false;
        }
        m_uiContext.renderer = m_renderer.get();

        m_textMeasurer = CreateTextMeasurer(m_activeRendererBackend);
        if (!m_textMeasurer) {
            return false;
        }
        m_uiContext.textMeasurer = m_textMeasurer.get();

        m_layoutEngine = CreateDefaultLayoutEngine();
        if (!m_layoutEngine) {
            return false;
        }
        m_uiContext.layoutEngine = m_layoutEngine.get();

        BuildUI();
        if (m_root) {
            m_root->SetContext(&m_uiContext);
        }
        m_isInitialized = true;
        OnResize();
        ShowWindow(m_hwnd, cmdShow);
        UpdateWindow(m_hwnd);
        SetTimer(m_hwnd, kUiTimerId, kUiTimerIntervalMs, nullptr);
        return true;
    }

    int Run() {
        MSG msg{};
        while (GetMessageW(&msg, nullptr, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        return static_cast<int>(msg.wParam);
    }

private:
    bool InitializeRenderer() {
        m_renderer = CreateRenderer(m_requestedRendererBackend);
        if (m_renderer && m_renderer->Initialize(m_hwnd)) {
            m_activeRendererBackend = m_renderer->Backend();
            return true;
        }

        if (m_requestedRendererBackend != RendererBackend::Direct2D) {
            m_renderer = CreateDirect2DRenderer();
            if (m_renderer && m_renderer->Initialize(m_hwnd)) {
                m_activeRendererBackend = m_renderer->Backend();
                return true;
            }
        }

        m_renderer.reset();
        return false;
    }

    void UpdateDpiContext(UINT dpi) {
        m_uiContext.dpi = dpi > 0 ? dpi : 96;
        m_uiContext.dpiScale = static_cast<float>(m_uiContext.dpi) / 96.0f;
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

    void LoadTheme() {
        if (!m_uiContext.resourceProvider) {
            return;
        }
        const std::wstring requestedTheme = GetEnvironmentValue(L"AI_WIN_UI_THEME");
        const std::string themePath = requestedTheme.empty()
            ? std::string("themes/default.json")
            : Utf16ToUtf8(requestedTheme);
        if (themePath.empty() || !m_uiContext.resourceProvider->Exists(themePath)) {
            return;
        }
        const std::string text = m_uiContext.resourceProvider->LoadText(themePath);
        m_theme = Theme::LoadFromJson(text);
        m_uiContext.theme = m_theme.get();
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
            return UIEventHandler();
        };

        LoadTheme();

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
        std::wstring title = L"AI WinUI Renderer [";
        title += RendererBackendDisplayName(m_activeRendererBackend);
        if (m_requestedRendererBackend != m_activeRendererBackend) {
            title += L" fallback from ";
            title += RendererBackendDisplayName(m_requestedRendererBackend);
        }
        title += L"]";
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
        if (m_renderer) {
            m_renderer->Resize(static_cast<UINT>(m_viewportWidth), static_cast<UINT>(m_viewportHeight));
        }
        UpdateLayout();
        InvalidateRect(m_hwnd, nullptr, FALSE);
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
        } else {
            m_contentHeight = m_viewportHeight;
        }
        UpdateScrollBar();
    }

    void OnPaint() {
        PAINTSTRUCT ps{};
        BeginPaint(m_hwnd, &ps);

        if (m_isInitialized && m_renderer) {
            m_renderer->BeginFrame(ColorFromHex(0x101010));
            if (m_root) {
                m_root->Render(*m_renderer);
            }
            m_renderer->EndFrame();
        }

        EndPaint(m_hwnd, &ps);
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

        UIElement* focusTarget = m_root->FindFocusableAt(x, y);
        if (!focusTarget) {
            SetFocusedElement(nullptr);
        } else {
            SetFocusedElement(focusTarget);
        }

        m_mouseCaptureTarget = m_root->FindHitElementAt(x, y);
        if (m_root->OnMouseDown(x, y)) {
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
            InvalidateRect(m_hwnd, nullptr, FALSE);
        }
    }

    void OnMouseLeave() {
        m_isTrackingMouseLeave = false;
        if (m_root && m_root->OnMouseLeave()) {
            InvalidateRect(m_hwnd, nullptr, FALSE);
        }
    }

    void OnMouseWheel(WPARAM wParam) {
        const float wheelStep = std::max(24.0f, 56.0f * m_uiContext.dpiScale);
        const float delta = static_cast<float>(GET_WHEEL_DELTA_WPARAM(wParam)) / static_cast<float>(WHEEL_DELTA);
        if (delta != 0.0f) {
            ScrollBy(-delta * wheelStep);
        }
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
        if (wParam == VK_TAB) {
            MoveFocus((GetKeyState(VK_SHIFT) & 0x8000) != 0);
            return true;
        }
        if (m_focusedElement && m_focusedElement->OnKeyDown(wParam, lParam)) {
            return true;
        }
        return false;
    }

    bool OnKeyUp(WPARAM wParam, LPARAM lParam) {
        if (m_focusedElement && m_focusedElement->OnKeyUp(wParam, lParam)) {
            return true;
        }
        return false;
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
        }
        UpdateScrollBar();
        InvalidateRect(m_hwnd, nullptr, FALSE);
        return true;
    }

    float MaxScrollOffset() const {
        return std::max(0.0f, m_contentHeight - m_viewportHeight);
    }

    void ClampScrollOffset() {
        m_scrollOffset = std::clamp(m_scrollOffset, 0.0f, MaxScrollOffset());
    }

    void UpdateScrollBar() const {
        SCROLLINFO si{};
        si.cbSize = sizeof(si);
        si.fMask = SIF_PAGE | SIF_POS | SIF_RANGE;
        si.nMin = 0;
        si.nMax = std::max(0, static_cast<int>(std::ceil(m_contentHeight)) - 1);
        si.nPage = static_cast<UINT>(std::max(0.0f, m_viewportHeight));
        si.nPos = static_cast<int>(std::lround(m_scrollOffset));
        SetScrollInfo(m_hwnd, SB_VERT, &si, TRUE);
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
            auto* app = static_cast<App*>(cs->lpCreateParams);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
            SetWindowLongPtrW(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&App::WndProcThunk));
            return app->HandleMessage(hwnd, msg, wParam, lParam);
        }
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    static LRESULT CALLBACK WndProcThunk(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        auto* app = reinterpret_cast<App*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        return app ? app->HandleMessage(hwnd, msg, wParam, lParam) : DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    LRESULT HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        switch (msg) {
            case WM_DPICHANGED: {
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
            case WM_SIZE:
                OnResize();
                return 0;
            case WM_MOUSEMOVE:
                OnMouseMove(lParam);
                return 0;
            case WM_MOUSELEAVE:
                OnMouseLeave();
                return 0;
            case WM_MOUSEWHEEL:
                OnMouseWheel(wParam);
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
                    return 0;
                }
                break;
            case WM_CHAR:
                if (m_focusedElement && m_focusedElement->OnChar(static_cast<wchar_t>(wParam))) {
                    InvalidateRect(m_hwnd, nullptr, FALSE);
                    return 0;
                }
                break;
            case WM_KEYUP:
                if (OnKeyUp(wParam, lParam)) {
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
                break;
            case WM_PAINT:
                OnPaint();
                return 0;
            case WM_ERASEBKGND:
                return 1;
            case WM_DESTROY:
                KillTimer(hwnd, kUiTimerId);
                PostQuitMessage(0);
                return 0;
            default:
                return DefWindowProcW(hwnd, msg, wParam, lParam);
        }
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    HWND m_hwnd = nullptr;
    UIContext m_uiContext{};
    std::unique_ptr<IRenderer> m_renderer;
    std::unique_ptr<ITextMeasurer> m_textMeasurer;
    std::unique_ptr<ILayoutEngine> m_layoutEngine;
    std::unique_ptr<IResourceProvider> m_resourceProvider;
    std::unique_ptr<Theme> m_theme;
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
    bool m_isInitialized = false;
    bool m_isTrackingMouseLeave = false;
};

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int cmdShow) {
    InitializeDpiAwareness();

    App app;
    if (!app.Initialize(instance, cmdShow)) {
        MessageBoxW(nullptr, L"Failed to initialize app.", L"Error", MB_ICONERROR);
        return -1;
    }
    return app.Run();
}
