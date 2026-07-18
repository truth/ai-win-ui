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
    // Wave2 R6: when wrap==NoWrap and text exceeds rect width, show "…" (draw+measure).
    // Ignored for TextWrapMode::Wrap (multi-line ellipsis not in v1).
    bool ellipsis = false;
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
    // Wave2 R7: two-stop linear gradient fill (optional rounded). angleDegrees: 0=L→R, 90=T→B.
    virtual void FillLinearGradient(const Rect& rect,
                                    float cornerRadius,
                                    const Color& start,
                                    const Color& end,
                                    float angleDegrees) = 0;
    // Soft shadow under a rounded rect (drawn around bounds + offset; multi-pass approx).
    virtual void FillSoftShadow(const Rect& rect,
                                float cornerRadius,
                                float offsetX,
                                float offsetY,
                                float blur,
                                const Color& color) = 0;
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

    // Wave2 R8: optional cacheKey (e.g. resource path) reuses parsed DOM; nullptr hashes content.
    virtual SvgHandle CreateSvgFromBytes(const uint8_t* data, size_t size, const char* cacheKey = nullptr) = 0;
    virtual Size GetSvgSize(SvgHandle svg) = 0;
    // tint: if hasTint, recolor non-transparent pixels (SrcIn) — monochrome icons.
    virtual void DrawSvg(SvgHandle svg, const Rect& rect) {
        DrawSvg(svg, rect, false, Color{});
    }
    virtual void DrawSvg(SvgHandle svg, const Rect& rect, bool hasTint, const Color& tint) = 0;
};

std::unique_ptr<IRenderer> CreateDirect2DRenderer();
std::unique_ptr<IRenderer> CreateSkiaRenderer();
std::unique_ptr<IRenderer> CreateRenderer(RendererBackend backend = RendererBackend::Direct2D);
