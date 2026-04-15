#include "layout_parser.h"
#include "renderer.h"
#include "resource_provider.h"
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

        if (!m_renderer.Initialize(m_hwnd)) {
            return false;
        }

        BuildUI();
        m_isInitialized = true;
        OnResize();
        ShowWindow(m_hwnd, cmdShow);
        UpdateWindow(m_hwnd);
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

        std::unique_ptr<IResourceProvider> resourceProvider;
        auto zipProvider = std::make_unique<ZipResourceProvider>(zipPath);
        if (zipProvider->IsValid()) {
            resourceProvider = std::move(zipProvider);
        } else {
            resourceProvider = std::make_unique<DirectoryResourceProvider>(resourceDir);
        }

        auto eventResolver = [this](const std::string& eventId) -> std::function<void()> {
            if (eventId == "primaryAction") {
                return [this]() { SetWindowTextW(m_hwnd, L"Clicked: Primary Action"); };
            }
            if (eventId == "secondaryAction") {
                return [this]() { SetWindowTextW(m_hwnd, L"Clicked: Secondary Action"); };
            }
            return std::function<void()>();
        };

        const std::vector<std::string> layoutCandidates = {
            "layouts/ui.json",
            "layouts/ui.xml"
        };

        for (const auto& path : layoutCandidates) {
            m_root = LayoutParser::BuildFromFile(*resourceProvider, path, eventResolver);
            if (m_root) {
                break;
            }
        }

        if (!m_root) {
            BuildDefaultUI();
        }
    }

    void BuildDefaultUI() {
        auto panel = std::make_unique<Panel>();
        panel->background = D2D1::ColorF(0x171717);
        panel->padding = {24, 24, 24, 24};
        panel->spacing = 10.0f;

        auto title = std::make_unique<Label>(L"DirectUI-style Retained UI Tree");
        auto desc = std::make_unique<Label>(L"Backend: Win32 + Direct2D + DirectWrite");

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
        panel->AddChild(std::move(btn1));
        panel->AddChild(std::move(btn2));
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
        m_renderer.Resize(w, h);
        if (m_root) {
            m_root->Arrange(D2D1::RectF(0, 0, static_cast<float>(w), static_cast<float>(h)));
        }
        InvalidateRect(m_hwnd, nullptr, FALSE);
    }

    void OnPaint() {
        PAINTSTRUCT ps{};
        BeginPaint(m_hwnd, &ps);

        if (m_isInitialized) {
            m_renderer.BeginFrame(D2D1::ColorF(0x101010));
            if (m_root) {
                m_root->Render(m_renderer);
            }
            m_renderer.EndFrame();
        }

        EndPaint(m_hwnd, &ps);
    }

    void OnMouseMove(LPARAM lParam) {
        if (!m_root) {
            return;
        }
        const float x = static_cast<float>(GET_X_LPARAM(lParam));
        const float y = static_cast<float>(GET_Y_LPARAM(lParam));
        if (m_root->OnMouseMove(x, y)) {
            InvalidateRect(m_hwnd, nullptr, FALSE);
        }
    }

    void OnMouseDown(LPARAM lParam) {
        if (!m_root) {
            return;
        }
        const float x = static_cast<float>(GET_X_LPARAM(lParam));
        const float y = static_cast<float>(GET_Y_LPARAM(lParam));
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
        if (m_root->OnMouseUp(x, y)) {
            InvalidateRect(m_hwnd, nullptr, FALSE);
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
            case WM_LBUTTONDOWN:
                SetCapture(hwnd);
                OnMouseDown(lParam);
                return 0;
            case WM_LBUTTONUP:
                ReleaseCapture();
                OnMouseUp(lParam);
                return 0;
            case WM_PAINT:
                OnPaint();
                return 0;
            case WM_ERASEBKGND:
                return 1;
            case WM_DESTROY:
                PostQuitMessage(0);
                return 0;
            default:
                return DefWindowProcW(hwnd, msg, wParam, lParam);
        }
    }

    HWND m_hwnd = nullptr;
    Renderer m_renderer;
    std::unique_ptr<UIElement> m_root;
    bool m_isInitialized = false;
};

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int cmdShow) {
    App app;
    if (!app.Initialize(instance, cmdShow)) {
        MessageBoxW(nullptr, L"Failed to initialize app.", L"Error", MB_ICONERROR);
        return -1;
    }
    return app.Run();
}
