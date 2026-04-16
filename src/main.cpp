#include "layout_parser.h"
#include "layout_engine.h"
#include "renderer.h"
#include "resource_provider.h"
#include "text_measurer.h"
#include "ui_context.h"
#include "ui.h"
#include "zip_resource_provider.h"

#include <Windowsx.h>
#include <memory>
#include <string>
#include <vector>

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

        m_hwnd = CreateWindowExW(
            0,
            kClassName,
            L"AI WinUI Renderer (DirectUI-style)",
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT, 1000, 700,
            nullptr,
            nullptr,
            instance,
            this
        );

        if (!m_hwnd) {
            return false;
        }

        m_renderer = CreateRenderer();
        if (!m_renderer || !m_renderer->Initialize(m_hwnd)) {
            return false;
        }
        m_uiContext.renderer = m_renderer.get();

        m_textMeasurer = CreateTextMeasurer();
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
        SetTimer(m_hwnd, 1, 500, nullptr);
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
    std::wstring GetExecutableDirectory() const {
        wchar_t path[MAX_PATH] = {};
        if (GetModuleFileNameW(nullptr, path, ARRAYSIZE(path)) == 0) {
            return L"";
        }
        std::wstring exePath(path);
        const size_t pos = exePath.find_last_of(L"\\/");
        return pos != std::wstring::npos ? exePath.substr(0, pos) : exePath;
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

        const std::vector<std::string> layoutCandidates = {
            "layouts/ui.json",
            "layouts/ui.xml"
        };

        for (const auto& path : layoutCandidates) {
            m_root = LayoutParser::BuildFromFile(m_uiContext, path);
            if (m_root) {
                break;
            }
        }

        if (!m_root) {
            BuildDefaultUI();
        }
        SetFocusedElement(GetFirstFocusable());
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
        const UINT w = static_cast<UINT>(rc.right - rc.left);
        const UINT h = static_cast<UINT>(rc.bottom - rc.top);
        if (m_renderer) {
            m_renderer->Resize(w, h);
        }
        if (m_root) {
            m_root->Measure(static_cast<float>(w), static_cast<float>(h));
            m_root->Arrange(Rect::Make(0, 0, static_cast<float>(w), static_cast<float>(h)));
        }
        InvalidateRect(m_hwnd, nullptr, FALSE);
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
            case WM_SIZE:
                OnResize();
                return 0;
            case WM_MOUSEMOVE:
                OnMouseMove(lParam);
                return 0;
            case WM_MOUSELEAVE:
                OnMouseLeave();
                return 0;
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
                if (m_focusedElement && m_focusedElement->OnTimer(wParam)) {
                    InvalidateRect(m_hwnd, nullptr, FALSE);
                    return 0;
                }
                break;
            case WM_PAINT:
                OnPaint();
                return 0;
            case WM_ERASEBKGND:
                return 1;
            case WM_DESTROY:
                KillTimer(hwnd, 1);
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
    std::unique_ptr<UIElement> m_root;
    UIElement* m_focusedElement = nullptr;
    UIElement* m_mouseCaptureTarget = nullptr;
    bool m_isInitialized = false;
    bool m_isTrackingMouseLeave = false;
};

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int cmdShow) {
    App app;
    if (!app.Initialize(instance, cmdShow)) {
        MessageBoxW(nullptr, L"Failed to initialize app.", L"Error", MB_ICONERROR);
        return -1;
    }
    return app.Run();
}
