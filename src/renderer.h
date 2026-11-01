#pragma once

#include "graphics_types.h"

#include <Windows.h>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

enum class RendererBackend {
    Default = -1, // Not explicitly set; honour the inherited AI_WIN_UI_RENDERER env.
    Direct2D = 0,
    Skia = 1,
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
    // Visual Rendering v2: fine-grained typography controls.
    uint16_t fontWeight = 0; // 0 = derive from bold (700 if bold else 400); otherwise 100..900
    std::wstring fontFamily; // empty = default "Segoe UI"
    float letterSpacing = 0.0f;
    float lineHeightFactor = 0.0f; // > 0 overrides default line spacing as fontSize * lineHeightFactor

    uint16_t EffectiveWeight() const {
        if (fontWeight >= 100 && fontWeight <= 900) {
            return fontWeight;
        }
        return bold ? 700 : 400;
    }
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
    virtual void FillRRect(const Rect& rect, const CornerRadius& radii, const Color& color) {
        if (radii.IsZero()) {
            FillRect(rect, color);
        } else {
            FillRoundedRect(rect, color, radii.MaxRadius());
        }
    }
    // Wave2 R7: two-stop linear gradient fill (optional rounded). angleDegrees: 0=L->R, 90=T->B.
    virtual void FillLinearGradient(const Rect& rect,
                                    float cornerRadius,
                                    const Color& start,
                                    const Color& end,
                                    float angleDegrees) = 0;
    // Soft shadow under a rounded rect (drawn around bounds + offset; Gaussian blur on Skia).
    virtual void FillSoftShadow(const Rect& rect,
                                float cornerRadius,
                                float offsetX,
                                float offsetY,
                                float blur,
                                const Color& color) = 0;
    virtual void DrawRect(const Rect& rect, const Color& color, float strokeWidth = 1.0f) = 0;
    virtual void DrawRoundedRect(const Rect& rect, const Color& color, float strokeWidth, float radius) = 0;
    virtual void DrawRRect(const Rect& rect, const CornerRadius& radii, const Color& color, float strokeWidth = 1.0f) {
        if (radii.IsZero()) {
            DrawRect(rect, color, strokeWidth);
        } else {
            DrawRoundedRect(rect, color, strokeWidth, radii.MaxRadius());
        }
    }
    virtual void DrawLine(const PointF& start, const PointF& end, const Color& color, float strokeWidth = 1.0f) = 0;
    virtual void DrawDashedLine(const PointF& start,
                                const PointF& end,
                                const Color& color,
                                float strokeWidth = 1.0f,
                                float dashLength = 4.0f,
                                float gapLength = 4.0f) {
        (void)dashLength;
        (void)gapLength;
        DrawLine(start, end, color, strokeWidth);
    }
    virtual void DrawPolyline(const std::vector<PointF>& points, const Color& color, float strokeWidth = 1.0f) = 0;
    // Filled closed polygon (first point connected to last). Empty / single-point is a no-op.
    virtual void FillPolygon(const std::vector<PointF>& points, const Color& color) = 0;
    virtual void FillPolygonLinearGradient(const std::vector<PointF>& points,
                                           float topY,
                                           float bottomY,
                                           const Color& topColor,
                                           const Color& bottomColor) {
        (void)topY;
        (void)bottomY;
        (void)bottomColor;
        FillPolygon(points, topColor);
    }
    virtual void DrawSvgPath(const std::string& svgPathData,
                             float viewBoxWidth,
                             float viewBoxHeight,
                             const Rect& destRect,
                             const Color& fill,
                             const Color& stroke,
                             float strokeWidth = 1.5f) {
        (void)svgPathData;
        (void)viewBoxWidth;
        (void)viewBoxHeight;
        (void)destRect;
        (void)fill;
        (void)stroke;
        (void)strokeWidth;
    }
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

    // Phase 2: Dirty Rect BBox Culling
    virtual void SetDirtyRect(const Rect& rect) { m_dirtyRect = rect; m_hasDirtyRect = true; }
    virtual void ClearDirtyRect() { m_hasDirtyRect = false; }
    virtual bool HasDirtyRect() const { return m_hasDirtyRect; }
    virtual Rect GetDirtyRect() const { return m_dirtyRect; }
    virtual bool IntersectsDirtyRect(const Rect& rect) const {
        if (!m_hasDirtyRect) return true;
        return !(rect.right < m_dirtyRect.left ||
                 rect.left > m_dirtyRect.right ||
                 rect.bottom < m_dirtyRect.top ||
                 rect.top > m_dirtyRect.bottom);
    }

protected:
    Rect m_dirtyRect{};
    bool m_hasDirtyRect = false;
};

std::unique_ptr<IRenderer> CreateDirect2DRenderer();
std::unique_ptr<IRenderer> CreateSkiaRenderer();
std::unique_ptr<IRenderer> CreateRenderer(RendererBackend backend = RendererBackend::Direct2D);
