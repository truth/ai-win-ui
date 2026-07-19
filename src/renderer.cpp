#include "renderer.h"

#include <d2d1.h>
#include <dwrite.h>
#include <wincodec.h>
#include <wrl/client.h>

#include <algorithm>
#include <cmath>
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

// SVG sentinel for the Direct2D backend: SVG is only fully drawn when the
// active backend is Skia. Direct2D paints a placeholder (gray box + "SVG"
// label) so the user notices the mismatch instead of getting a silent
// blank slot.
class PlaceholderSvgResource final : public SvgResource {
public:
    Size GetIntrinsicSize() const override { return Size{16.0f, 16.0f}; }
};

class Direct2DRenderer final : public IRenderer {
public:
    ~Direct2DRenderer() override {
        // Release all COM objects before CoUninitialize — otherwise combase AV on exit.
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
        // Top-down BGRA DIB: alpha in byte 3.
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
            // Layered surface size is baked into the DIB; recreate on resize.
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
        // CSS-like: 0deg = left→right, 90deg = top→bottom.
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
        const int passes = std::clamp(static_cast<int>(std::lround(blurPx / 3.0f)) + 2, 3, 8);
        for (int i = passes; i >= 1; --i) {
            const float t = static_cast<float>(i) / static_cast<float>(passes);
            const float expand = blurPx * t * 0.55f;
            const float alpha = color.a * (0.08f + 0.22f * (1.0f - t)) / static_cast<float>(passes) * 2.5f;
            const Color layer{color.r, color.g, color.b, std::clamp(alpha, 0.0f, 1.0f)};
            const Rect shadow = Rect::Make(
                rect.left - expand + offsetX,
                rect.top - expand + offsetY,
                rect.right + expand + offsetX,
                rect.bottom + expand + offsetY);
            const float r = cornerRadius + expand * 0.35f;
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

        ComPtr<IDWriteTextFormat> format;
        if (FAILED(m_dwriteFactory->CreateTextFormat(
                L"Segoe UI",
                nullptr,
                options.bold ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_NORMAL,
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

        // Wave2 R6: character-level ellipsis on NoWrap (single-line overflow).
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
        // D2D has no real SVG parse; still cache placeholders by key so handles are stable.
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
        // Clear to transparent.
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

        // Leave position unchanged (pptDst = nullptr); only refresh pixels/size.
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
            // Grayscale AA avoids ClearType fringing on transparent edges.
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
            // Force 1 DIP = 1 pixel so layout / hit-test stay aligned with mouse.
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
