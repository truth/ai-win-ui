#pragma once

#include <Windows.h>
#include <d2d1.h>
#include <dwrite.h>
#include <wrl/client.h>

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

class Renderer {
public:
    bool Initialize(HWND hwnd);
    void Resize(UINT width, UINT height);
    void BeginFrame(const D2D1_COLOR_F& clearColor);
    void EndFrame();

    void FillRect(const D2D1_RECT_F& rect, const D2D1_COLOR_F& color);
    void DrawRect(const D2D1_RECT_F& rect, const D2D1_COLOR_F& color, float strokeWidth = 1.0f);
    void DrawTextW(const wchar_t* text, UINT32 len, const D2D1_RECT_F& rect, const D2D1_COLOR_F& color, float fontSize = 16.0f);

private:
    template <typename T>
    using Com = Microsoft::WRL::ComPtr<T>;

    bool EnsureRenderTarget();
    ID2D1HwndRenderTarget* Target() const { return m_renderTarget.Get(); }

    HWND m_hwnd = nullptr;
    Com<ID2D1Factory> m_d2dFactory;
    Com<IDWriteFactory> m_dwriteFactory;
    Com<ID2D1HwndRenderTarget> m_renderTarget;
    Com<ID2D1SolidColorBrush> m_solidBrush;
    Com<IDWriteTextFormat> m_defaultTextFormat;
};
