#pragma once

#include "graphics_types.h"

#include <Windows.h>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

enum class RendererBackend {
    Direct2D,
    Skia,
};

// How the backend presents pixels to the OS window.
enum class PresentMode {
    Hwnd = 0,    // ID2D1HwndRenderTarget / normal HWND paint
    Layered = 1, // offscreen BGRA + UpdateLayeredWindow (per-pixel alpha)
};

enum class TextWrapMode {
    NoWrap,
    Wrap,
};

enum class TextHorizontalAlign {
    Start,
    Center,
    End,
};

enum class TextVerticalAlign {
    Start,
    Center,
    End,
};

struct TextRenderOptions {
    TextWrapMode wrap = TextWrapMode::Wrap;
    TextHorizontalAlign horizontalAlign = TextHorizontalAlign::Start;
    TextVerticalAlign verticalAlign = TextVerticalAlign::Start;
    bool bold = false;
    bool italic = false;
};

const char* RendererBackendId(RendererBackend backend);
const wchar_t* RendererBackendDisplayName(RendererBackend backend);

class IRenderer {
public:
    virtual ~IRenderer() = default;
    virtual RendererBackend Backend() const = 0;
    virtual bool Initialize(HWND hwnd) = 0;
    virtual void Resize(UINT width, UINT height) = 0;
    virtual void BeginFrame(const Color& clearColor) = 0;
    virtual void EndFrame() = 0;

    // Optional present path. Default backends keep HWND presentation.
    virtual void SetPresentMode(PresentMode mode) { (void)mode; }
    virtual PresentMode GetPresentMode() const { return PresentMode::Hwnd; }
    // Sample last presented frame alpha (Layered only). Defaults to opaque.
    virtual bool SampleOpaque(int x, int y, uint8_t alphaThreshold = 16) const {
        (void)x;
        (void)y;
        (void)alphaThreshold;
        return true;
    }

    virtual void FillRect(const Rect& rect, const Color& color) = 0;
    virtual void FillRoundedRect(const Rect& rect, const Color& color, float radius) = 0;
    virtual void DrawRect(const Rect& rect, const Color& color, float strokeWidth = 1.0f) = 0;
    virtual void DrawRoundedRect(const Rect& rect, const Color& color, float strokeWidth, float radius) = 0;
    virtual void DrawLine(const PointF& start, const PointF& end, const Color& color, float strokeWidth = 1.0f) = 0;
    virtual void DrawPolyline(const std::vector<PointF>& points, const Color& color, float strokeWidth = 1.0f) = 0;
    // Filled closed polygon (first point connected to last). Empty / single-point is a no-op.
    virtual void FillPolygon(const std::vector<PointF>& points, const Color& color) = 0;
    virtual void PushRoundedClip(const Rect& rect, float radius) = 0;
    virtual void PopLayer() = 0;
    virtual void DrawTextW(const wchar_t* text,
                           UINT32 len,
                           const Rect& rect,
                           const Color& color,
                           float fontSize = 16.0f,
                           const TextRenderOptions& options = {}) = 0;
    virtual Size GetBitmapSize(BitmapHandle bitmap) = 0;
    virtual BitmapHandle CreateBitmapFromBytes(const uint8_t* data, size_t size) = 0;
    virtual void DrawBitmap(BitmapHandle bitmap, const Rect& rect) = 0;

    virtual SvgHandle CreateSvgFromBytes(const uint8_t* data, size_t size) = 0;
    virtual Size GetSvgSize(SvgHandle svg) = 0;
    virtual void DrawSvg(SvgHandle svg, const Rect& rect) = 0;
};

std::unique_ptr<IRenderer> CreateDirect2DRenderer();
std::unique_ptr<IRenderer> CreateSkiaRenderer();
std::unique_ptr<IRenderer> CreateRenderer(RendererBackend backend = RendererBackend::Direct2D);
