#include "renderer.h"

#include <d2d1.h>
#include <dwrite.h>
#include <wincodec.h>
#include <wrl/client.h>

#include <limits>
#include <memory>
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

    ID2D1Bitmap* GetOrCreateBitmap(ID2D1HwndRenderTarget* renderTarget, uint64_t generation) {
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
    ID2D1HwndRenderTarget* m_cachedRenderTarget = nullptr;
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
        if (m_shouldCoUninitialize) {
            CoUninitialize();
        }
    }

    RendererBackend Backend() const override {
        return RendererBackend::Direct2D;
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

        return EnsureRenderTarget();
    }

    void Resize(UINT width, UINT height) override {
        if (m_renderTarget) {
            m_renderTarget->Resize(D2D1::SizeU(width, height));
        }
    }

    void BeginFrame(const Color& clearColor) override {
        if (!EnsureRenderTarget()) {
            return;
        }

        m_renderTarget->BeginDraw();
        m_renderTarget->Clear(ToD2DColor(clearColor));
    }

    void EndFrame() override {
        if (!m_renderTarget) {
            return;
        }

        const HRESULT hr = m_renderTarget->EndDraw();
        if (hr == D2DERR_RECREATE_TARGET) {
            m_layerStack.clear();
            m_solidBrush.Reset();
            m_renderTarget.Reset();
        }
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
                DWRITE_FONT_WEIGHT_NORMAL,
                DWRITE_FONT_STYLE_NORMAL,
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

    SvgHandle CreateSvgFromBytes(const uint8_t* /*data*/, size_t /*size*/) override {
        return std::make_shared<PlaceholderSvgResource>();
    }

    Size GetSvgSize(SvgHandle /*svg*/) override {
        return Size{16.0f, 16.0f};
    }

    void DrawSvg(SvgHandle svg, const Rect& rect) override {
        if (!m_renderTarget) return;
        const Color bg     = ColorFromHex(0x2A2A2A);
        const Color border = ColorFromHex(0x6A6A6A);
        const Color fg     = ColorFromHex(0xB7B7B7);
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

    bool EnsureRenderTarget() {
        if (m_renderTarget) {
            return true;
        }
        if (!m_d2dFactory || !m_hwnd) {
            return false;
        }

        RECT rc{};
        GetClientRect(m_hwnd, &rc);
        const auto size = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);

        if (FAILED(m_d2dFactory->CreateHwndRenderTarget(
                D2D1::RenderTargetProperties(),
                D2D1::HwndRenderTargetProperties(m_hwnd, size),
                m_renderTarget.ReleaseAndGetAddressOf()))) {
            return false;
        }

        // Force 1 DIP = 1 pixel so renderer coordinate space matches client
        // pixels used by layout / hit-test. Without this D2D scales by system
        // DPI and drawing drifts away from mouse coordinates on >100% scaling.
        m_renderTarget->SetDpi(96.0f, 96.0f);

        if (FAILED(m_renderTarget->CreateSolidColorBrush(
                D2D1::ColorF(D2D1::ColorF::White),
                m_solidBrush.ReleaseAndGetAddressOf()))) {
            m_renderTarget.Reset();
            return false;
        }

        m_renderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
        m_renderTarget->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE);

        ++m_renderTargetGeneration;
        return true;
    }

    ID2D1HwndRenderTarget* Target() const {
        return m_renderTarget.Get();
    }

    HWND m_hwnd = nullptr;
    ComPtr<ID2D1Factory> m_d2dFactory;
    ComPtr<IWICImagingFactory> m_wicFactory;
    ComPtr<IDWriteFactory> m_dwriteFactory;
    ComPtr<ID2D1HwndRenderTarget> m_renderTarget;
    ComPtr<ID2D1SolidColorBrush> m_solidBrush;
    std::vector<LayerEntry> m_layerStack;
    uint64_t m_renderTargetGeneration = 0;
    bool m_shouldCoUninitialize = false;
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
