#include "renderer.h"

#include <d2d1.h>
#include <dwrite.h>
#include <wincodec.h>
#include <wrl/client.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

using Microsoft::WRL::ComPtr;

D2D1_COLOR_F ToD2DColor(const Color& color) {
    return D2D1::ColorF(color.r, color.g, color.b, color.a);
}

D2D1_RECT_F ToD2DRect(const Rect& rect) {
    return D2D1::RectF(rect.left, rect.top, rect.right, rect.bottom);
}

DWRITE_TEXT_ALIGNMENT ToDWriteTextAlignment(TextHorizontalAlign align) {
    switch (align) {
        case TextHorizontalAlign::Center:
            return DWRITE_TEXT_ALIGNMENT_CENTER;
        case TextHorizontalAlign::End:
            return DWRITE_TEXT_ALIGNMENT_TRAILING;
        case TextHorizontalAlign::Start:
        default:
            return DWRITE_TEXT_ALIGNMENT_LEADING;
    }
}

DWRITE_PARAGRAPH_ALIGNMENT ToDWriteParagraphAlignment(TextVerticalAlign align) {
    switch (align) {
        case TextVerticalAlign::Center:
            return DWRITE_PARAGRAPH_ALIGNMENT_CENTER;
        case TextVerticalAlign::End:
            return DWRITE_PARAGRAPH_ALIGNMENT_FAR;
        case TextVerticalAlign::Start:
        default:
            return DWRITE_PARAGRAPH_ALIGNMENT_NEAR;
    }
}

ComPtr<ID2D1PathGeometry> CreateD2DRRectGeometry(ID2D1Factory* factory, const Rect& rect, const CornerRadius& radii) {
    if (!factory) {
        return nullptr;
    }
    ComPtr<ID2D1PathGeometry> path;
    if (FAILED(factory->CreatePathGeometry(path.ReleaseAndGetAddressOf()))) {
        return nullptr;
    }
    ComPtr<ID2D1GeometrySink> sink;
    if (FAILED(path->Open(sink.ReleaseAndGetAddressOf()))) {
        return nullptr;
    }

    const float w = rect.Width();
    const float h = rect.Height();
    const float maxR = std::min(w, h) * 0.5f;
    const float tl = std::clamp(radii.topLeft, 0.0f, maxR);
    const float tr = std::clamp(radii.topRight, 0.0f, maxR);
    const float br = std::clamp(radii.bottomRight, 0.0f, maxR);
    const float bl = std::clamp(radii.bottomLeft, 0.0f, maxR);

    sink->BeginFigure(D2D1::Point2F(rect.left + tl, rect.top), D2D1_FIGURE_BEGIN_FILLED);
    sink->AddLine(D2D1::Point2F(rect.right - tr, rect.top));
    if (tr > 0.0f) {
        sink->AddArc(D2D1::ArcSegment(
            D2D1::Point2F(rect.right, rect.top + tr),
            D2D1::SizeF(tr, tr), 0.0f, D2D1_SWEEP_DIRECTION_CLOCKWISE, D2D1_ARC_SIZE_SMALL));
    }
    sink->AddLine(D2D1::Point2F(rect.right, rect.bottom - br));
    if (br > 0.0f) {
        sink->AddArc(D2D1::ArcSegment(
            D2D1::Point2F(rect.right - br, rect.bottom),
            D2D1::SizeF(br, br), 0.0f, D2D1_SWEEP_DIRECTION_CLOCKWISE, D2D1_ARC_SIZE_SMALL));
    }
    sink->AddLine(D2D1::Point2F(rect.left + bl, rect.bottom));
    if (bl > 0.0f) {
        sink->AddArc(D2D1::ArcSegment(
            D2D1::Point2F(rect.left, rect.bottom - bl),
            D2D1::SizeF(bl, bl), 0.0f, D2D1_SWEEP_DIRECTION_CLOCKWISE, D2D1_ARC_SIZE_SMALL));
    }
    sink->AddLine(D2D1::Point2F(rect.left, rect.top + tl));
    if (tl > 0.0f) {
        sink->AddArc(D2D1::ArcSegment(
            D2D1::Point2F(rect.left + tl, rect.top),
            D2D1::SizeF(tl, tl), 0.0f, D2D1_SWEEP_DIRECTION_CLOCKWISE, D2D1_ARC_SIZE_SMALL));
    }
    sink->EndFigure(D2D1_FIGURE_END_CLOSED);
    sink->Close();
    return path;
}

// Lightweight SVG Path parser for Direct2D (supports M/m, L/l, H/h, V/v, C/c, Z/z)
ComPtr<ID2D1PathGeometry> ParseSvgPathToD2D(ID2D1Factory* factory,
                                            const std::string& data,
                                            float scale,
                                            float offsetX,
                                            float offsetY,
                                            bool filled) {
    if (!factory || data.empty()) {
        return nullptr;
    }
    ComPtr<ID2D1PathGeometry> path;
    if (FAILED(factory->CreatePathGeometry(path.ReleaseAndGetAddressOf()))) {
        return nullptr;
    }
    ComPtr<ID2D1GeometrySink> sink;
    if (FAILED(path->Open(sink.ReleaseAndGetAddressOf()))) {
        return nullptr;
    }

    const char* p = data.c_str();
    auto skipWs = [&]() {
        while (*p && (std::isspace(static_cast<unsigned char>(*p)) || *p == ',')) {
            ++p;
        }
    };
    auto readFloat = [&](float& out) -> bool {
        skipWs();
        if (!*p || (*p != '-' && *p != '+' && *p != '.' && !std::isdigit(static_cast<unsigned char>(*p)))) {
            return false;
        }
        char* endPtr = nullptr;
        out = std::strtof(p, &endPtr);
        if (endPtr == p) {
            return false;
        }
        p = endPtr;
        return true;
    };
    auto mapPt = [&](float x, float y) -> D2D1_POINT_2F {
        return D2D1::Point2F(offsetX + x * scale, offsetY + y * scale);
    };

    float curX = 0.0f;
    float curY = 0.0f;
    float startX = 0.0f;
    float startY = 0.0f;
    bool inFigure = false;
    char cmd = 0;

    while (*p) {
        skipWs();
        if (!*p) break;
        if (std::isalpha(static_cast<unsigned char>(*p))) {
            cmd = *p++;
        } else if (!cmd) {
            break;
        }

        switch (cmd) {
            case 'M':
            case 'm': {
                float x = 0.0f, y = 0.0f;
                if (!readFloat(x) || !readFloat(y)) break;
                if (cmd == 'm') { x += curX; y += curY; }
                if (inFigure) {
                    sink->EndFigure(D2D1_FIGURE_END_OPEN);
                }
                sink->BeginFigure(mapPt(x, y), filled ? D2D1_FIGURE_BEGIN_FILLED : D2D1_FIGURE_BEGIN_HOLLOW);
                inFigure = true;
                curX = startX = x;
                curY = startY = y;
                cmd = (cmd == 'M') ? 'L' : 'l';
                break;
            }
            case 'L':
            case 'l': {
                float x = 0.0f, y = 0.0f;
                if (!readFloat(x) || !readFloat(y)) { cmd = 0; break; }
                if (cmd == 'l') { x += curX; y += curY; }
                if (inFigure) {
                    sink->AddLine(mapPt(x, y));
                }
                curX = x;
                curY = y;
                break;
            }
            case 'H':
            case 'h': {
                float x = 0.0f;
                if (!readFloat(x)) { cmd = 0; break; }
                if (cmd == 'h') x += curX;
                if (inFigure) {
                    sink->AddLine(mapPt(x, curY));
                }
                curX = x;
                break;
            }
            case 'V':
            case 'v': {
                float y = 0.0f;
                if (!readFloat(y)) { cmd = 0; break; }
                if (cmd == 'v') y += curY;
                if (inFigure) {
                    sink->AddLine(mapPt(curX, y));
                }
                curY = y;
                break;
            }
            case 'C':
            case 'c': {
                float x1 = 0, y1 = 0, x2 = 0, y2 = 0, x = 0, y = 0;
                if (!readFloat(x1) || !readFloat(y1) || !readFloat(x2) || !readFloat(y2) || !readFloat(x) || !readFloat(y)) {
                    cmd = 0;
                    break;
                }
                if (cmd == 'c') {
                    x1 += curX; y1 += curY;
                    x2 += curX; y2 += curY;
                    x += curX;  y += curY;
                }
                if (inFigure) {
                    sink->AddBezier(D2D1::BezierSegment(mapPt(x1, y1), mapPt(x2, y2), mapPt(x, y)));
                }
                curX = x;
                curY = y;
                break;
            }
            case 'A':
            case 'a': {
                float rx = 0, ry = 0, rot = 0, largeArc = 0, sweep = 0, x = 0, y = 0;
                if (!readFloat(rx) || !readFloat(ry) || !readFloat(rot) ||
                    !readFloat(largeArc) || !readFloat(sweep) || !readFloat(x) || !readFloat(y)) {
                    cmd = 0;
                    break;
                }
                if (cmd == 'a') {
                    x += curX;
                    y += curY;
                }
                if (inFigure) {
                    sink->AddArc(D2D1::ArcSegment(
                        mapPt(x, y),
                        D2D1::SizeF(std::abs(rx) * scale, std::abs(ry) * scale),
                        rot,
                        sweep > 0.5f ? D2D1_SWEEP_DIRECTION_CLOCKWISE : D2D1_SWEEP_DIRECTION_COUNTER_CLOCKWISE,
                        largeArc > 0.5f ? D2D1_ARC_SIZE_LARGE : D2D1_ARC_SIZE_SMALL));
                }
                curX = x;
                curY = y;
                break;
            }
            case 'Z':
            case 'z': {
                if (inFigure) {
                    sink->EndFigure(D2D1_FIGURE_END_CLOSED);
                    inFigure = false;
                }
                curX = startX;
                curY = startY;
                cmd = 0;
                break;
            }
            default:
                ++p;
                cmd = 0;
                break;
        }
    }
    if (inFigure) {
        sink->EndFigure(D2D1_FIGURE_END_OPEN);
    }
    sink->Close();
    return path;
}

class DecodedBitmapResource final : public BitmapResource {
public:
    DecodedBitmapResource(UINT width, UINT height, UINT stride, std::vector<uint8_t> pixels)
        : m_size{static_cast<float>(width), static_cast<float>(height)},
          m_width(width),
          m_height(height),
          m_stride(stride),
          m_pixels(std::move(pixels)) {}

    Size GetSize() const override {
        return m_size;
    }

    ID2D1Bitmap* GetOrCreateBitmap(ID2D1RenderTarget* renderTarget, uint64_t generation) {
        if (!renderTarget || m_pixels.empty()) {
            return nullptr;
        }

        if (!m_bitmap || m_generation != generation || m_cachedRenderTarget != renderTarget) {
            m_bitmap.Reset();
            const D2D1_BITMAP_PROPERTIES bitmapProperties = D2D1::BitmapProperties(
                D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));
            if (FAILED(renderTarget->CreateBitmap(
                    D2D1::SizeU(m_width, m_height),
                    m_pixels.data(),
                    m_stride,
                    &bitmapProperties,
                    m_bitmap.ReleaseAndGetAddressOf()))) {
                return nullptr;
            }
            m_cachedRenderTarget = renderTarget;
            m_generation = generation;
        }

        return m_bitmap.Get();
    }

private:
    Size m_size{};
    UINT m_width = 0;
    UINT m_height = 0;
    UINT m_stride = 0;
    std::vector<uint8_t> m_pixels;
    ComPtr<ID2D1Bitmap> m_bitmap;
    ID2D1RenderTarget* m_cachedRenderTarget = nullptr;
    uint64_t m_generation = 0;
};

class PlaceholderSvgResource final : public SvgResource {
public:
    Size GetIntrinsicSize() const override { return Size{16.0f, 16.0f}; }
};

class Direct2DRenderer final : public IRenderer {
public:
    ~Direct2DRenderer() override {
        while (!m_layerStack.empty()) {
            if (m_renderTarget) {
                m_renderTarget->PopLayer();
            }
            m_layerStack.pop_back();
        }
        ReleaseLayeredSurface();
        DestroyRenderTarget();
        m_solidBrush.Reset();
        m_wicFactory.Reset();
        m_dwriteFactory.Reset();
        m_d2dFactory.Reset();
        m_hwnd = nullptr;
        if (m_shouldCoUninitialize) {
            CoUninitialize();
            m_shouldCoUninitialize = false;
        }
    }

    RendererBackend Backend() const override {
        return RendererBackend::Direct2D;
    }

    void SetPresentMode(PresentMode mode) override {
        if (m_presentMode == mode) {
            return;
        }
        m_presentMode = mode;
        DestroyRenderTarget();
    }

    PresentMode GetPresentMode() const override {
        return m_presentMode;
    }

    bool SampleOpaque(int x, int y, uint8_t alphaThreshold) const override {
        if (m_presentMode != PresentMode::Layered || !m_dibBits || m_pixelWidth == 0 || m_pixelHeight == 0) {
            return true;
        }
        if (x < 0 || y < 0 || x >= static_cast<int>(m_pixelWidth) || y >= static_cast<int>(m_pixelHeight)) {
            return false;
        }
        const size_t index = (static_cast<size_t>(y) * m_pixelWidth + static_cast<size_t>(x)) * 4u;
        const auto* bytes = static_cast<const uint8_t*>(m_dibBits);
        return bytes[index + 3] >= alphaThreshold;
    }

    bool Initialize(HWND hwnd) override {
        m_hwnd = hwnd;

        if (FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, m_d2dFactory.ReleaseAndGetAddressOf()))) {
            return false;
        }

        if (FAILED(DWriteCreateFactory(
                DWRITE_FACTORY_TYPE_SHARED,
                __uuidof(IDWriteFactory),
                reinterpret_cast<IUnknown**>(m_dwriteFactory.ReleaseAndGetAddressOf())))) {
            return false;
        }

        const HRESULT comResult = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        if (FAILED(comResult) && comResult != RPC_E_CHANGED_MODE) {
            return false;
        }
        m_shouldCoUninitialize = SUCCEEDED(comResult);

        if (FAILED(CoCreateInstance(
                CLSID_WICImagingFactory,
                nullptr,
                CLSCTX_INPROC_SERVER,
                IID_PPV_ARGS(m_wicFactory.ReleaseAndGetAddressOf())))) {
            return false;
        }

        if (m_hwnd) {
            RECT rc{};
            GetClientRect(m_hwnd, &rc);
            m_pixelWidth = static_cast<UINT>(std::max(1L, rc.right - rc.left));
            m_pixelHeight = static_cast<UINT>(std::max(1L, rc.bottom - rc.top));
        }

        return EnsureRenderTarget();
    }

    void Resize(UINT width, UINT height) override {
        width = std::max(1u, width);
        height = std::max(1u, height);
        if (width == m_pixelWidth && height == m_pixelHeight && m_renderTarget) {
            return;
        }
        m_pixelWidth = width;
        m_pixelHeight = height;

        if (m_presentMode == PresentMode::Layered) {
            DestroyRenderTarget();
            EnsureRenderTarget();
            return;
        }

        if (m_hwndRenderTarget) {
            m_hwndRenderTarget->Resize(D2D1::SizeU(m_pixelWidth, m_pixelHeight));
        }
    }

    void BeginFrame(const Color& clearColor) override {
        if (!EnsureRenderTarget()) {
            return;
        }

        if (m_presentMode == PresentMode::Layered && m_dcRenderTarget && m_memDC) {
            const RECT bindRect{0, 0, static_cast<LONG>(m_pixelWidth), static_cast<LONG>(m_pixelHeight)};
            if (FAILED(m_dcRenderTarget->BindDC(m_memDC, &bindRect))) {
                DestroyRenderTarget();
                if (!EnsureRenderTarget()) {
                    return;
                }
                if (FAILED(m_dcRenderTarget->BindDC(m_memDC, &bindRect))) {
                    return;
                }
            }
        }

        m_renderTarget->BeginDraw();
        m_renderTarget->Clear(ToD2DColor(clearColor));
        m_frameDirty = true;
    }

    void EndFrame() override {
        if (!m_renderTarget) {
            return;
        }

        const HRESULT hr = m_renderTarget->EndDraw();
        if (hr == D2DERR_RECREATE_TARGET) {
            DestroyRenderTarget();
            m_frameDirty = false;
            return;
        }

        if (FAILED(hr)) {
            m_frameDirty = false;
            return;
        }

        if (m_presentMode == PresentMode::Layered) {
            if (m_hwnd && IsIconic(m_hwnd)) {
                m_frameDirty = false;
                return;
            }
            if (m_frameDirty) {
                PresentLayeredSurface();
            }
        }
        m_frameDirty = false;
    }

    void FillRect(const Rect& rect, const Color& color) override {
        if (!m_renderTarget || !m_solidBrush) {
            return;
        }

        m_solidBrush->SetColor(ToD2DColor(color));
        m_renderTarget->FillRectangle(ToD2DRect(rect), m_solidBrush.Get());
    }

    void FillRoundedRect(const Rect& rect, const Color& color, float radius) override {
        if (!m_renderTarget || !m_solidBrush) {
            return;
        }

        m_solidBrush->SetColor(ToD2DColor(color));
        D2D1_ROUNDED_RECT roundedRect = D2D1::RoundedRect(ToD2DRect(rect), radius, radius);
        m_renderTarget->FillRoundedRectangle(&roundedRect, m_solidBrush.Get());
    }

    void FillRRect(const Rect& rect, const CornerRadius& radii, const Color& color) override {
        if (!m_renderTarget || !m_solidBrush || !m_d2dFactory) {
            return;
        }
        if (radii.IsUniform()) {
            FillRoundedRect(rect, color, radii.topLeft);
            return;
        }
        if (auto geom = CreateD2DRRectGeometry(m_d2dFactory.Get(), rect, radii)) {
            m_solidBrush->SetColor(ToD2DColor(color));
            m_renderTarget->FillGeometry(geom.Get(), m_solidBrush.Get());
        }
    }

    void FillLinearGradient(const Rect& rect,
                            float cornerRadius,
                            const Color& start,
                            const Color& end,
                            float angleDegrees) override {
        if (!m_renderTarget || !m_d2dFactory) {
            return;
        }
        const float cx = (rect.left + rect.right) * 0.5f;
        const float cy = (rect.top + rect.bottom) * 0.5f;
        const float rad = angleDegrees * 3.14159265f / 180.0f;
        const float c = std::cos(rad);
        const float s = std::sin(rad);
        const float maxDist = (rect.Width() * 0.5f * std::abs(c)) + (rect.Height() * 0.5f * std::abs(s));

        const float dx = c * maxDist;
        const float dy = s * maxDist;

        D2D1_GRADIENT_STOP stops[2]{};
        stops[0].position = 0.0f;
        stops[0].color = ToD2DColor(start);
        stops[1].position = 1.0f;
        stops[1].color = ToD2DColor(end);

        ComPtr<ID2D1GradientStopCollection> collection;
        if (FAILED(m_renderTarget->CreateGradientStopCollection(
                stops, 2, D2D1_GAMMA_2_2, D2D1_EXTEND_MODE_CLAMP, collection.ReleaseAndGetAddressOf()))) {
            return;
        }

        ComPtr<ID2D1LinearGradientBrush> brush;
        if (FAILED(m_renderTarget->CreateLinearGradientBrush(
                D2D1::LinearGradientBrushProperties(
                    D2D1::Point2F(cx - dx, cy - dy),
                    D2D1::Point2F(cx + dx, cy + dy)),
                collection.Get(),
                brush.ReleaseAndGetAddressOf()))) {
            return;
        }

        if (cornerRadius > 0.5f) {
            D2D1_ROUNDED_RECT rr = D2D1::RoundedRect(ToD2DRect(rect), cornerRadius, cornerRadius);
            m_renderTarget->FillRoundedRectangle(&rr, brush.Get());
        } else {
            m_renderTarget->FillRectangle(ToD2DRect(rect), brush.Get());
        }
    }

    void FillSoftShadow(const Rect& rect,
                        float cornerRadius,
                        float offsetX,
                        float offsetY,
                        float blur,
                        const Color& color) override {
        if (!m_renderTarget || !m_solidBrush || color.a <= 0.001f) {
            return;
        }
        const float blurPx = std::max(0.0f, blur);
        const int passes = std::clamp(static_cast<int>(std::lround(blurPx / 2.0f)) + 4, 6, 14);
        for (int i = passes; i >= 1; --i) {
            const float t = static_cast<float>(i) / static_cast<float>(passes);
            const float expand = blurPx * t * 0.65f;
            // Smooth Gaussian-like falloff weight
            const float weight = std::exp(-2.2f * t * t);
            const float alpha = color.a * weight / static_cast<float>(passes) * 2.2f;
            const Color layer{color.r, color.g, color.b, std::clamp(alpha, 0.0f, 1.0f)};
            const Rect shadow = Rect::Make(
                rect.left - expand + offsetX,
                rect.top - expand + offsetY,
                rect.right + expand + offsetX,
                rect.bottom + expand + offsetY);
            const float r = cornerRadius + expand * 0.4f;
            m_solidBrush->SetColor(ToD2DColor(layer));
            if (r > 0.5f) {
                D2D1_ROUNDED_RECT rr = D2D1::RoundedRect(ToD2DRect(shadow), r, r);
                m_renderTarget->FillRoundedRectangle(&rr, m_solidBrush.Get());
            } else {
                m_renderTarget->FillRectangle(ToD2DRect(shadow), m_solidBrush.Get());
            }
        }
    }

    void DrawRect(const Rect& rect, const Color& color, float strokeWidth) override {
        if (!m_renderTarget || !m_solidBrush) {
            return;
        }

        m_solidBrush->SetColor(ToD2DColor(color));
        m_renderTarget->DrawRectangle(ToD2DRect(rect), m_solidBrush.Get(), strokeWidth);
    }

    void DrawRoundedRect(const Rect& rect, const Color& color, float strokeWidth, float radius) override {
        if (!m_renderTarget || !m_solidBrush) {
            return;
        }

        m_solidBrush->SetColor(ToD2DColor(color));
        D2D1_ROUNDED_RECT roundedRect = D2D1::RoundedRect(ToD2DRect(rect), radius, radius);
        m_renderTarget->DrawRoundedRectangle(&roundedRect, m_solidBrush.Get(), strokeWidth);
    }

    void DrawRRect(const Rect& rect, const CornerRadius& radii, const Color& color, float strokeWidth) override {
        if (!m_renderTarget || !m_solidBrush || !m_d2dFactory) {
            return;
        }
        if (radii.IsUniform()) {
            DrawRoundedRect(rect, color, strokeWidth, radii.topLeft);
            return;
        }
        if (auto geom = CreateD2DRRectGeometry(m_d2dFactory.Get(), rect, radii)) {
            m_solidBrush->SetColor(ToD2DColor(color));
            m_renderTarget->DrawGeometry(geom.Get(), m_solidBrush.Get(), strokeWidth);
        }
    }

    void DrawLine(const PointF& start, const PointF& end, const Color& color, float strokeWidth) override {
        if (!m_renderTarget || !m_solidBrush) {
            return;
        }

        m_solidBrush->SetColor(ToD2DColor(color));
        m_renderTarget->DrawLine(
            D2D1::Point2F(start.x, start.y),
            D2D1::Point2F(end.x, end.y),
            m_solidBrush.Get(),
            strokeWidth);
    }

    void DrawDashedLine(const PointF& start,
                        const PointF& end,
                        const Color& color,
                        float strokeWidth,
                        float dashLength,
                        float gapLength) override {
        if (!m_renderTarget || !m_solidBrush || !m_d2dFactory) {
            return;
        }
        const float sw = std::max(0.5f, strokeWidth);
        const float dashes[2] = {std::max(1.0f, dashLength) / sw, std::max(1.0f, gapLength) / sw};
        ComPtr<ID2D1StrokeStyle> strokeStyle;
        if (SUCCEEDED(m_d2dFactory->CreateStrokeStyle(
                D2D1::StrokeStyleProperties(
                    D2D1_CAP_STYLE_FLAT,
                    D2D1_CAP_STYLE_FLAT,
                    D2D1_CAP_STYLE_FLAT,
                    D2D1_LINE_JOIN_MITER,
                    10.0f,
                    D2D1_DASH_STYLE_CUSTOM,
                    0.0f),
                dashes,
                2,
                strokeStyle.ReleaseAndGetAddressOf()))) {
            m_solidBrush->SetColor(ToD2DColor(color));
            m_renderTarget->DrawLine(
                D2D1::Point2F(start.x, start.y),
                D2D1::Point2F(end.x, end.y),
                m_solidBrush.Get(),
                strokeWidth,
                strokeStyle.Get());
            return;
        }
        DrawLine(start, end, color, strokeWidth);
    }

    void DrawPolyline(const std::vector<PointF>& points, const Color& color, float strokeWidth) override {
        if (!m_renderTarget || !m_solidBrush || !m_d2dFactory || points.size() < 2) {
            return;
        }

        ComPtr<ID2D1PathGeometry> geometry;
        if (FAILED(m_d2dFactory->CreatePathGeometry(geometry.ReleaseAndGetAddressOf()))) {
            return;
        }

        ComPtr<ID2D1GeometrySink> sink;
        if (FAILED(geometry->Open(sink.ReleaseAndGetAddressOf()))) {
            return;
        }

        sink->BeginFigure(D2D1::Point2F(points.front().x, points.front().y), D2D1_FIGURE_BEGIN_HOLLOW);

        std::vector<D2D1_POINT_2F> segmentPoints;
        segmentPoints.reserve(points.size() - 1);
        for (size_t i = 1; i < points.size(); ++i) {
            segmentPoints.push_back(D2D1::Point2F(points[i].x, points[i].y));
        }
        if (!segmentPoints.empty()) {
            sink->AddLines(segmentPoints.data(), static_cast<UINT32>(segmentPoints.size()));
        }
        sink->EndFigure(D2D1_FIGURE_END_OPEN);

        if (FAILED(sink->Close())) {
            return;
        }

        m_solidBrush->SetColor(ToD2DColor(color));
        m_renderTarget->DrawGeometry(geometry.Get(), m_solidBrush.Get(), strokeWidth);
    }

    void FillPolygon(const std::vector<PointF>& points, const Color& color) override {
        if (!m_renderTarget || !m_solidBrush || !m_d2dFactory || points.size() < 3) {
            return;
        }

        ComPtr<ID2D1PathGeometry> geometry;
        if (FAILED(m_d2dFactory->CreatePathGeometry(geometry.ReleaseAndGetAddressOf()))) {
            return;
        }

        ComPtr<ID2D1GeometrySink> sink;
        if (FAILED(geometry->Open(sink.ReleaseAndGetAddressOf()))) {
            return;
        }

        sink->BeginFigure(D2D1::Point2F(points.front().x, points.front().y), D2D1_FIGURE_BEGIN_FILLED);
        std::vector<D2D1_POINT_2F> segmentPoints;
        segmentPoints.reserve(points.size() - 1);
        for (size_t i = 1; i < points.size(); ++i) {
            segmentPoints.push_back(D2D1::Point2F(points[i].x, points[i].y));
        }
        if (!segmentPoints.empty()) {
            sink->AddLines(segmentPoints.data(), static_cast<UINT32>(segmentPoints.size()));
        }
        sink->EndFigure(D2D1_FIGURE_END_CLOSED);
        if (FAILED(sink->Close())) {
            return;
        }

        m_solidBrush->SetColor(ToD2DColor(color));
        m_renderTarget->FillGeometry(geometry.Get(), m_solidBrush.Get());
    }

    void FillPolygonLinearGradient(const std::vector<PointF>& points,
                                   float topY,
                                   float bottomY,
                                   const Color& topColor,
                                   const Color& bottomColor) override {
        if (!m_renderTarget || !m_d2dFactory || points.size() < 3) {
            return;
        }

        ComPtr<ID2D1PathGeometry> geometry;
        if (FAILED(m_d2dFactory->CreatePathGeometry(geometry.ReleaseAndGetAddressOf()))) {
            return;
        }

        ComPtr<ID2D1GeometrySink> sink;
        if (FAILED(geometry->Open(sink.ReleaseAndGetAddressOf()))) {
            return;
        }

        sink->BeginFigure(D2D1::Point2F(points.front().x, points.front().y), D2D1_FIGURE_BEGIN_FILLED);
        std::vector<D2D1_POINT_2F> segmentPoints;
        segmentPoints.reserve(points.size() - 1);
        for (size_t i = 1; i < points.size(); ++i) {
            segmentPoints.push_back(D2D1::Point2F(points[i].x, points[i].y));
        }
        if (!segmentPoints.empty()) {
            sink->AddLines(segmentPoints.data(), static_cast<UINT32>(segmentPoints.size()));
        }
        sink->EndFigure(D2D1_FIGURE_END_CLOSED);
        if (FAILED(sink->Close())) {
            return;
        }

        D2D1_GRADIENT_STOP stops[2]{};
        stops[0].position = 0.0f;
        stops[0].color = ToD2DColor(topColor);
        stops[1].position = 1.0f;
        stops[1].color = ToD2DColor(bottomColor);

        ComPtr<ID2D1GradientStopCollection> collection;
        if (FAILED(m_renderTarget->CreateGradientStopCollection(
                stops, 2, D2D1_GAMMA_2_2, D2D1_EXTEND_MODE_CLAMP, collection.ReleaseAndGetAddressOf()))) {
            return;
        }

        ComPtr<ID2D1LinearGradientBrush> brush;
        if (FAILED(m_renderTarget->CreateLinearGradientBrush(
                D2D1::LinearGradientBrushProperties(
                    D2D1::Point2F(0.0f, topY),
                    D2D1::Point2F(0.0f, std::max(topY + 1.0f, bottomY))),
                collection.Get(),
                brush.ReleaseAndGetAddressOf()))) {
            return;
        }

        m_renderTarget->FillGeometry(geometry.Get(), brush.Get());
    }

    void DrawSvgPath(const std::string& svgPathData,
                     float viewBoxWidth,
                     float viewBoxHeight,
                     const Rect& destRect,
                     const Color& fill,
                     const Color& stroke,
                     float strokeWidth) override {
        if (!m_renderTarget || !m_solidBrush || !m_d2dFactory || svgPathData.empty()) {
            return;
        }
        const float vbW = std::max(1.0f, viewBoxWidth);
        const float vbH = std::max(1.0f, viewBoxHeight);
        const float scale = std::min(destRect.Width() / vbW, destRect.Height() / vbH);
        const float offsetX = destRect.left + (destRect.Width() - vbW * scale) * 0.5f;
        const float offsetY = destRect.top + (destRect.Height() - vbH * scale) * 0.5f;

        if (fill.a > 0.001f) {
            if (auto geom = ParseSvgPathToD2D(m_d2dFactory.Get(), svgPathData, scale, offsetX, offsetY, true)) {
                m_solidBrush->SetColor(ToD2DColor(fill));
                m_renderTarget->FillGeometry(geom.Get(), m_solidBrush.Get());
            }
        }
        if (stroke.a > 0.001f && strokeWidth > 0.0f) {
            if (auto geom = ParseSvgPathToD2D(m_d2dFactory.Get(), svgPathData, scale, offsetX, offsetY, false)) {
                ComPtr<ID2D1StrokeStyle> roundStyle;
                m_d2dFactory->CreateStrokeStyle(
                    D2D1::StrokeStyleProperties(
                        D2D1_CAP_STYLE_ROUND,
                        D2D1_CAP_STYLE_ROUND,
                        D2D1_CAP_STYLE_ROUND,
                        D2D1_LINE_JOIN_ROUND),
                    nullptr,
                    0,
                    roundStyle.ReleaseAndGetAddressOf());
                m_solidBrush->SetColor(ToD2DColor(stroke));
                m_renderTarget->DrawGeometry(geom.Get(), m_solidBrush.Get(), strokeWidth, roundStyle.Get());
            }
        }
    }

    void PushRoundedClip(const Rect& rect, float radius) override {
        if (!m_renderTarget || !m_d2dFactory) {
            return;
        }

        LayerEntry entry;
        if (FAILED(m_d2dFactory->CreateRoundedRectangleGeometry(
                D2D1::RoundedRect(ToD2DRect(rect), radius, radius),
                entry.geometry.ReleaseAndGetAddressOf()))) {
            return;
        }

        if (FAILED(m_renderTarget->CreateLayer(entry.layer.ReleaseAndGetAddressOf()))) {
            return;
        }

        D2D1_LAYER_PARAMETERS params = D2D1::LayerParameters(
            ToD2DRect(rect),
            entry.geometry.Get(),
            D2D1_ANTIALIAS_MODE_PER_PRIMITIVE,
            D2D1::Matrix3x2F::Identity(),
            1.0f,
            nullptr,
            D2D1_LAYER_OPTIONS_NONE);

        m_renderTarget->PushLayer(&params, entry.layer.Get());
        m_layerStack.emplace_back(std::move(entry));
    }

    void PopLayer() override {
        if (!m_renderTarget || m_layerStack.empty()) {
            return;
        }

        m_renderTarget->PopLayer();
        m_layerStack.pop_back();
    }

    void DrawTextW(const wchar_t* text,
                   UINT32 len,
                   const Rect& rect,
                   const Color& color,
                   float fontSize,
                   const TextRenderOptions& options) override {
        if (!m_renderTarget || !m_solidBrush || !m_dwriteFactory) {
            return;
        }

        const wchar_t* family = options.fontFamily.empty() ? L"Segoe UI" : options.fontFamily.c_str();
        const DWRITE_FONT_WEIGHT weight = static_cast<DWRITE_FONT_WEIGHT>(options.EffectiveWeight());

        ComPtr<IDWriteTextFormat> format;
        if (FAILED(m_dwriteFactory->CreateTextFormat(
                family,
                nullptr,
                weight,
                options.italic ? DWRITE_FONT_STYLE_ITALIC : DWRITE_FONT_STYLE_NORMAL,
                DWRITE_FONT_STRETCH_NORMAL,
                fontSize,
                L"",
                format.ReleaseAndGetAddressOf()))) {
            return;
        }

        format->SetTextAlignment(ToDWriteTextAlignment(options.horizontalAlign));
        format->SetParagraphAlignment(ToDWriteParagraphAlignment(options.verticalAlign));
        format->SetWordWrapping(
            options.wrap == TextWrapMode::Wrap
                ? DWRITE_WORD_WRAPPING_WRAP
                : DWRITE_WORD_WRAPPING_NO_WRAP);
        if (options.lineHeightFactor > 0.0f) {
            const float lh = fontSize * options.lineHeightFactor;
            format->SetLineSpacing(DWRITE_LINE_SPACING_METHOD_UNIFORM, lh, lh * 0.8f);
        }

        if (options.ellipsis && options.wrap == TextWrapMode::NoWrap) {
            ComPtr<IDWriteInlineObject> trimmingSign;
            if (SUCCEEDED(m_dwriteFactory->CreateEllipsisTrimmingSign(format.Get(), trimmingSign.GetAddressOf()))) {
                DWRITE_TRIMMING trimming{};
                trimming.granularity = DWRITE_TRIMMING_GRANULARITY_CHARACTER;
                format->SetTrimming(&trimming, trimmingSign.Get());
            }
        }

        m_solidBrush->SetColor(ToD2DColor(color));
        m_renderTarget->DrawTextW(text, len, format.Get(), ToD2DRect(rect), m_solidBrush.Get());
    }

    Size GetBitmapSize(BitmapHandle bitmap) override {
        return bitmap ? bitmap->GetSize() : Size{};
    }

    BitmapHandle CreateBitmapFromBytes(const uint8_t* data, size_t size) override {
        if (!m_wicFactory || !data || size == 0 || size > std::numeric_limits<DWORD>::max()) {
            return nullptr;
        }

        ComPtr<IWICStream> stream;
        if (FAILED(m_wicFactory->CreateStream(stream.ReleaseAndGetAddressOf()))) {
            return nullptr;
        }
        if (FAILED(stream->InitializeFromMemory(const_cast<BYTE*>(data), static_cast<DWORD>(size)))) {
            return nullptr;
        }

        ComPtr<IWICBitmapDecoder> decoder;
        if (FAILED(m_wicFactory->CreateDecoderFromStream(
                stream.Get(),
                nullptr,
                WICDecodeMetadataCacheOnLoad,
                decoder.ReleaseAndGetAddressOf()))) {
            return nullptr;
        }

        ComPtr<IWICBitmapFrameDecode> frame;
        if (FAILED(decoder->GetFrame(0, frame.ReleaseAndGetAddressOf()))) {
            return nullptr;
        }

        ComPtr<IWICFormatConverter> converter;
        if (FAILED(m_wicFactory->CreateFormatConverter(converter.ReleaseAndGetAddressOf()))) {
            return nullptr;
        }

        if (FAILED(converter->Initialize(
                frame.Get(),
                GUID_WICPixelFormat32bppPBGRA,
                WICBitmapDitherTypeNone,
                nullptr,
                0.0,
                WICBitmapPaletteTypeMedianCut))) {
            return nullptr;
        }

        UINT width = 0;
        UINT height = 0;
        if (FAILED(converter->GetSize(&width, &height)) || width == 0 || height == 0) {
            return nullptr;
        }

        const UINT stride = width * 4;
        const size_t bufferSize = static_cast<size_t>(stride) * height;
        if (bufferSize == 0 || bufferSize > std::numeric_limits<UINT>::max()) {
            return nullptr;
        }

        std::vector<uint8_t> pixels(bufferSize);
        if (FAILED(converter->CopyPixels(
                nullptr,
                stride,
                static_cast<UINT>(bufferSize),
                pixels.data()))) {
            return nullptr;
        }

        return std::make_shared<DecodedBitmapResource>(width, height, stride, std::move(pixels));
    }

    void DrawBitmap(BitmapHandle bitmap, const Rect& rect) override {
        if (!m_renderTarget || !bitmap) {
            return;
        }

        auto decodedBitmap = std::dynamic_pointer_cast<DecodedBitmapResource>(bitmap);
        if (!decodedBitmap) {
            return;
        }

        ID2D1Bitmap* d2dBitmap = decodedBitmap->GetOrCreateBitmap(Target(), m_renderTargetGeneration);
        if (!d2dBitmap) {
            return;
        }

        m_renderTarget->DrawBitmap(d2dBitmap, ToD2DRect(rect));
    }

    SvgHandle CreateSvgFromBytes(const uint8_t* /*data*/, size_t /*size*/, const char* cacheKey) override {
        const std::string key = cacheKey && cacheKey[0]
            ? std::string(cacheKey)
            : std::string("d2d-placeholder");
        auto it = m_svgCache.find(key);
        if (it != m_svgCache.end()) {
            return it->second;
        }
        auto handle = std::make_shared<PlaceholderSvgResource>();
        m_svgCache[key] = handle;
        return handle;
    }

    Size GetSvgSize(SvgHandle /*svg*/) override {
        return Size{16.0f, 16.0f};
    }

    void DrawSvg(SvgHandle svg, const Rect& rect, bool hasTint, const Color& tint) override {
        if (!m_renderTarget) return;
        const Color bg     = hasTint ? Color{tint.r, tint.g, tint.b, 0.25f} : ColorFromHex(0x2A2A2A);
        const Color border = hasTint ? Color{tint.r, tint.g, tint.b, std::min(1.0f, tint.a)} : ColorFromHex(0x6A6A6A);
        const Color fg     = hasTint ? tint : ColorFromHex(0xB7B7B7);
        FillRect(rect, bg);
        DrawRect(rect, border, 1.0f);
        if (!svg) return;
        const wchar_t* label = L"SVG";
        DrawTextW(label, 3, rect, fg, 12.0f,
                  TextRenderOptions{TextWrapMode::NoWrap,
                                    TextHorizontalAlign::Center,
                                    TextVerticalAlign::Center});
    }

private:
    struct LayerEntry {
        ComPtr<ID2D1RoundedRectangleGeometry> geometry;
        ComPtr<ID2D1Layer> layer;
    };

    void DestroyRenderTarget() {
        m_layerStack.clear();
        m_solidBrush.Reset();
        m_renderTarget.Reset();
        m_hwndRenderTarget.Reset();
        m_dcRenderTarget.Reset();
        ReleaseLayeredSurface();
    }

    void ReleaseLayeredSurface() {
        if (m_memDC) {
            if (m_oldBitmap) {
                SelectObject(m_memDC, m_oldBitmap);
                m_oldBitmap = nullptr;
            }
            DeleteDC(m_memDC);
            m_memDC = nullptr;
        }
        if (m_dib) {
            DeleteObject(m_dib);
            m_dib = nullptr;
        }
        m_dibBits = nullptr;
    }

    bool CreateLayeredSurface() {
        ReleaseLayeredSurface();
        if (m_pixelWidth == 0 || m_pixelHeight == 0) {
            return false;
        }

        HDC screenDc = GetDC(nullptr);
        if (!screenDc) {
            return false;
        }
        m_memDC = CreateCompatibleDC(screenDc);
        ReleaseDC(nullptr, screenDc);
        if (!m_memDC) {
            return false;
        }

        BITMAPINFO bmi{};
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = static_cast<LONG>(m_pixelWidth);
        bmi.bmiHeader.biHeight = -static_cast<LONG>(m_pixelHeight); // top-down
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;

        m_dib = CreateDIBSection(m_memDC, &bmi, DIB_RGB_COLORS, &m_dibBits, nullptr, 0);
        if (!m_dib || !m_dibBits) {
            ReleaseLayeredSurface();
            return false;
        }
        m_oldBitmap = static_cast<HBITMAP>(SelectObject(m_memDC, m_dib));
        ZeroMemory(m_dibBits, static_cast<size_t>(m_pixelWidth) * m_pixelHeight * 4u);
        return true;
    }

    void PresentLayeredSurface() {
        if (!m_hwnd || !m_memDC || m_pixelWidth == 0 || m_pixelHeight == 0) {
            return;
        }

        POINT src{0, 0};
        SIZE size{static_cast<LONG>(m_pixelWidth), static_cast<LONG>(m_pixelHeight)};
        BLENDFUNCTION blend{};
        blend.BlendOp = AC_SRC_OVER;
        blend.BlendFlags = 0;
        blend.SourceConstantAlpha = 255;
        blend.AlphaFormat = AC_SRC_ALPHA;

        UpdateLayeredWindow(
            m_hwnd,
            nullptr,
            nullptr,
            &size,
            m_memDC,
            &src,
            0,
            &blend,
            ULW_ALPHA);
    }

    bool EnsureRenderTarget() {
        if (m_renderTarget) {
            return true;
        }
        if (!m_d2dFactory || !m_hwnd) {
            return false;
        }

        if (m_pixelWidth == 0 || m_pixelHeight == 0) {
            RECT rc{};
            GetClientRect(m_hwnd, &rc);
            m_pixelWidth = static_cast<UINT>(std::max(1L, rc.right - rc.left));
            m_pixelHeight = static_cast<UINT>(std::max(1L, rc.bottom - rc.top));
        }

        if (m_presentMode == PresentMode::Layered) {
            if (!CreateLayeredSurface()) {
                return false;
            }

            const D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
                D2D1_RENDER_TARGET_TYPE_DEFAULT,
                D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
                96.0f,
                96.0f,
                D2D1_RENDER_TARGET_USAGE_NONE,
                D2D1_FEATURE_LEVEL_DEFAULT);

            if (FAILED(m_d2dFactory->CreateDCRenderTarget(&props, m_dcRenderTarget.ReleaseAndGetAddressOf()))) {
                ReleaseLayeredSurface();
                return false;
            }

            const RECT bindRect{0, 0, static_cast<LONG>(m_pixelWidth), static_cast<LONG>(m_pixelHeight)};
            if (FAILED(m_dcRenderTarget->BindDC(m_memDC, &bindRect))) {
                m_dcRenderTarget.Reset();
                ReleaseLayeredSurface();
                return false;
            }

            m_renderTarget = m_dcRenderTarget;
            m_renderTarget->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);
        } else {
            const auto size = D2D1::SizeU(m_pixelWidth, m_pixelHeight);
            if (FAILED(m_d2dFactory->CreateHwndRenderTarget(
                    D2D1::RenderTargetProperties(),
                    D2D1::HwndRenderTargetProperties(m_hwnd, size),
                    m_hwndRenderTarget.ReleaseAndGetAddressOf()))) {
                return false;
            }
            m_renderTarget = m_hwndRenderTarget;
            m_renderTarget->SetDpi(96.0f, 96.0f);
            m_renderTarget->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE);
        }

        if (FAILED(m_renderTarget->CreateSolidColorBrush(
                D2D1::ColorF(D2D1::ColorF::White),
                m_solidBrush.ReleaseAndGetAddressOf()))) {
            DestroyRenderTarget();
            return false;
        }

        m_renderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
        ++m_renderTargetGeneration;
        return true;
    }

    ID2D1RenderTarget* Target() const {
        return m_renderTarget.Get();
    }

    HWND m_hwnd = nullptr;
    PresentMode m_presentMode = PresentMode::Hwnd;
    UINT m_pixelWidth = 0;
    UINT m_pixelHeight = 0;
    bool m_frameDirty = false;
    ComPtr<ID2D1Factory> m_d2dFactory;
    ComPtr<IWICImagingFactory> m_wicFactory;
    ComPtr<IDWriteFactory> m_dwriteFactory;
    ComPtr<ID2D1RenderTarget> m_renderTarget;
    ComPtr<ID2D1HwndRenderTarget> m_hwndRenderTarget;
    ComPtr<ID2D1DCRenderTarget> m_dcRenderTarget;
    ComPtr<ID2D1SolidColorBrush> m_solidBrush;
    std::vector<LayerEntry> m_layerStack;
    uint64_t m_renderTargetGeneration = 0;
    bool m_shouldCoUninitialize = false;

    HDC m_memDC = nullptr;
    HBITMAP m_dib = nullptr;
    HBITMAP m_oldBitmap = nullptr;
    void* m_dibBits = nullptr;
    std::unordered_map<std::string, SvgHandle> m_svgCache;
};

} // namespace

const char* RendererBackendId(RendererBackend backend) {
    switch (backend) {
        case RendererBackend::Skia:
            return "skia";
        case RendererBackend::Direct2D:
        default:
            return "direct2d";
    }
}

const wchar_t* RendererBackendDisplayName(RendererBackend backend) {
    switch (backend) {
        case RendererBackend::Skia:
            return L"Skia";
        case RendererBackend::Direct2D:
        default:
            return L"Direct2D";
    }
}

std::unique_ptr<IRenderer> CreateDirect2DRenderer() {
    return std::make_unique<Direct2DRenderer>();
}

std::unique_ptr<IRenderer> CreateRenderer(RendererBackend backend) {
    if (backend == RendererBackend::Skia) {
        if (auto renderer = CreateSkiaRenderer()) {
            return renderer;
        }
    }
    return CreateDirect2DRenderer();
}
