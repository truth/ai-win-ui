#pragma once

#include <Windows.h>
#include <cstdint>
#include <d2d1.h>
#include <dwrite.h>
#include <wincodec.h>
#include <wrl/client.h>

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "windowscodecs.lib")

class IRenderer {
public:
    virtual ~IRenderer() = default;
    virtual bool Initialize(HWND hwnd) = 0;
    virtual void Resize(UINT width, UINT height) = 0;
    virtual void BeginFrame(const D2D1_COLOR_F& clearColor) = 0;
    virtual void EndFrame() = 0;

    virtual void FillRect(const D2D1_RECT_F& rect, const D2D1_COLOR_F& color) = 0;
    virtual void FillRoundedRect(const D2D1_RECT_F& rect, const D2D1_COLOR_F& color, float radius) = 0;
    virtual void DrawRect(const D2D1_RECT_F& rect, const D2D1_COLOR_F& color, float strokeWidth = 1.0f) = 0;
    virtual void DrawRoundedRect(const D2D1_RECT_F& rect, const D2D1_COLOR_F& color, float strokeWidth, float radius) = 0;
    virtual void PushRoundedClip(const D2D1_RECT_F& rect, float radius) = 0;
    virtual void PopLayer() = 0;
    virtual void DrawTextW(const wchar_t* text, UINT32 len, const D2D1_RECT_F& rect, const D2D1_COLOR_F& color, float fontSize = 16.0f) = 0;
    virtual Microsoft::WRL::ComPtr<ID2D1Bitmap> CreateBitmapFromBytes(const uint8_t* data, size_t size) = 0;
    virtual void DrawBitmap(ID2D1Bitmap* bitmap, const D2D1_RECT_F& rect) = 0;
};

class Renderer : public IRenderer {
public:
    bool Initialize(HWND hwnd);
    void Resize(UINT width, UINT height);
    void BeginFrame(const D2D1_COLOR_F& clearColor);
    void EndFrame();

    void FillRect(const D2D1_RECT_F& rect, const D2D1_COLOR_F& color);
    void FillRoundedRect(const D2D1_RECT_F& rect, const D2D1_COLOR_F& color, float radius);
    void DrawRect(const D2D1_RECT_F& rect, const D2D1_COLOR_F& color, float strokeWidth = 1.0f);
    void DrawRoundedRect(const D2D1_RECT_F& rect, const D2D1_COLOR_F& color, float strokeWidth, float radius);
    void PushRoundedClip(const D2D1_RECT_F& rect, float radius);
    void PopLayer();
    void DrawTextW(const wchar_t* text, UINT32 len, const D2D1_RECT_F& rect, const D2D1_COLOR_F& color, float fontSize = 16.0f);
    Microsoft::WRL::ComPtr<ID2D1Bitmap> CreateBitmapFromBytes(const uint8_t* data, size_t size);
    void DrawBitmap(ID2D1Bitmap* bitmap, const D2D1_RECT_F& rect);

private:
    template <typename T>
    using Com = Microsoft::WRL::ComPtr<T>;

    bool EnsureRenderTarget();
    ID2D1HwndRenderTarget* Target() const { return m_renderTarget.Get(); }

    HWND m_hwnd = nullptr;
    Com<ID2D1Factory> m_d2dFactory;
    Com<IWICImagingFactory> m_wicFactory;
    Com<IDWriteFactory> m_dwriteFactory;
    Com<ID2D1HwndRenderTarget> m_renderTarget;
    Com<ID2D1SolidColorBrush> m_solidBrush;
    Com<IDWriteTextFormat> m_defaultTextFormat;
};
