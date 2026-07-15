#include "renderer.h"
#include "skia_text_layout.h"
#include "skia_font_shared.h"

#if defined(AI_WIN_UI_HAS_VCPKG_SKIA) || defined(AI_WIN_UI_HAS_LOCAL_SKIA)
#include "include/core/SkCanvas.h"
#include "include/core/SkColor.h"
#include "include/core/SkData.h"
#include "include/core/SkFontMetrics.h"
#include "include/core/SkImage.h"
#include "include/core/SkImageInfo.h"
#include "include/core/SkPaint.h"
#include "include/core/SkPath.h"
#include "include/core/SkRRect.h"
#include "include/core/SkSamplingOptions.h"
#include "include/core/SkStream.h"
#include "include/core/SkSize.h"
#include "include/core/SkSurface.h"
#include "include/core/SkSurfaceProps.h"
#include "include/core/SkTextBlob.h"
#include "include/core/SkTypes.h"

#include "modules/svg/include/SkSVGDOM.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <memory>
#include <string>

namespace {

SkColor ToSkColor(const Color& color) {
    const auto clamp = [](float value) -> U8CPU {
        return static_cast<U8CPU>(std::clamp(value, 0.0f, 1.0f) * 255.0f);
    };
    return SkColorSetARGB(clamp(color.a), clamp(color.r), clamp(color.g), clamp(color.b));
}

SkRect ToSkRect(const Rect& rect) {
    return SkRect::MakeLTRB(rect.left, rect.top, rect.right, rect.bottom);
}

void DrawSimpleTextWithFallback(SkCanvas* canvas,
                                const std::wstring& text,
                                float x,
                                float baseline,
                                float fontSize,
                                SkTypeface* defaultTypeface,
                                SkFontMgr* fontMgr,
                                const SkPaint& paint) {
    if (!canvas || text.empty()) {
        return;
    }

    const uint32_t len = static_cast<uint32_t>(text.size());
    if (skia_font::TypefaceSupportsText(defaultTypeface, text.c_str(), len)) {
        SkFont font = skia_font::CreateSkiaFont(fontSize, defaultTypeface);
        skia_font::StackUtf8Converter<> utf8(text.c_str(), len);
        if (!utf8.empty()) {
            canvas->drawSimpleText(
                utf8.c_str(),
                utf8.size(),
                SkTextEncoding::kUTF8,
                x,
                baseline,
                font,
                paint);
        }
        return;
    }

    float runX = x;
    skia_font::IterateTextRuns(fontMgr, defaultTypeface, text.c_str(), len, [&](const wchar_t* runText, uint32_t runLen, SkTypeface* typeface) {
        SkFont font = skia_font::CreateSkiaFont(fontSize, typeface);
        skia_font::StackUtf8Converter<> runUtf8(runText, runLen);
        if (!runUtf8.empty()) {
            canvas->drawSimpleText(
                runUtf8.c_str(),
                runUtf8.size(),
                SkTextEncoding::kUTF8,
                runX,
                baseline,
                font,
                paint);
            runX += font.measureText(runUtf8.c_str(), runUtf8.size(), SkTextEncoding::kUTF8);
        }
    });
}

class SkiaBitmapResource final : public BitmapResource {
public:
    explicit SkiaBitmapResource(sk_sp<SkImage> image)
        : m_image(std::move(image)) {
        if (m_image) {
            m_size = Size{
                static_cast<float>(m_image->width()),
                static_cast<float>(m_image->height()),
            };
        }
    }

    Size GetSize() const override {
        return m_size;
    }

    const sk_sp<SkImage>& Image() const {
        return m_image;
    }

private:
    Size m_size{};
    sk_sp<SkImage> m_image;
};

class SkiaSvgResource final : public SvgResource {
public:
    explicit SkiaSvgResource(sk_sp<SkSVGDOM> dom)
        : m_dom(std::move(dom)) {
        if (m_dom) {
            const SkSize sz = m_dom->containerSize();
            if (sz.width() > 0.0f && sz.height() > 0.0f) {
                m_intrinsic = Size{sz.width(), sz.height()};
            } else {
                // SVG without explicit width/height: leave a sensible 24x24 hint.
                m_intrinsic = Size{24.0f, 24.0f};
            }
        }
    }

    Size GetIntrinsicSize() const override { return m_intrinsic; }
    const sk_sp<SkSVGDOM>& Dom() const { return m_dom; }

private:
    Size m_intrinsic{24.0f, 24.0f};
    sk_sp<SkSVGDOM> m_dom;
};

class SkiaRenderer final : public IRenderer {
public:
    ~SkiaRenderer() override {
        ReleasePresentSurface();
    }

    RendererBackend Backend() const override {
        return RendererBackend::Skia;
    }

    void SetPresentMode(PresentMode mode) override {
        if (m_presentMode == mode) {
            return;
        }
        m_presentMode = mode;
        // Rebuild backing so layered/hwnd share the same DIB path.
        m_width = 0;
        m_height = 0;
        RecreateSurface();
    }

    PresentMode GetPresentMode() const override {
        return m_presentMode;
    }

    bool SampleOpaque(int x, int y, uint8_t alphaThreshold) const override {
        if (m_presentMode != PresentMode::Layered || !m_dibBits || m_width == 0 || m_height == 0) {
            return true;
        }
        if (x < 0 || y < 0 || x >= static_cast<int>(m_width) || y >= static_cast<int>(m_height)) {
            return false;
        }
        // N32 premul on Windows is BGRA; alpha is byte 3.
        const size_t index = (static_cast<size_t>(y) * m_width + static_cast<size_t>(x)) * 4u;
        const auto* bytes = static_cast<const uint8_t*>(m_dibBits);
        return bytes[index + 3] >= alphaThreshold;
    }

    bool Initialize(HWND hwnd) override {
        m_hwnd = hwnd;
        EnsureFontManager();
        return RecreateSurface();
    }

    void Resize(UINT width, UINT height) override {
        width = std::max(1u, width);
        height = std::max(1u, height);
        if (width == m_width && height == m_height && m_surface) {
            return;
        }
        m_width = width;
        m_height = height;
        RecreateSurface();
    }

    void BeginFrame(const Color& clearColor) override {
        if (!EnsureSurface()) {
            return;
        }
        SkCanvas* canvas = m_surface->getCanvas();
        if (!canvas) {
            return;
        }

        canvas->resetMatrix();
        canvas->restoreToCount(1);
        canvas->clear(ToSkColor(clearColor));
        m_frameDirty = true;
    }

    void EndFrame() override {
        if (!m_surface || !m_dibBits || !m_hwnd || m_width == 0 || m_height == 0) {
            return;
        }
        // Skip presents while minimized; surface stays warm for restore.
        if (IsIconic(m_hwnd)) {
            m_frameDirty = false;
            return;
        }
        if (!m_frameDirty) {
            return;
        }

        if (m_presentMode == PresentMode::Layered) {
            PresentLayered();
        } else {
            PresentHwnd();
        }
        m_frameDirty = false;
    }

    void FillRect(const Rect& rect, const Color& color) override {
        if (SkCanvas* canvas = Canvas()) {
            SkPaint paint;
            paint.setColor(ToSkColor(color));
            paint.setStyle(SkPaint::kFill_Style);
            paint.setAntiAlias(true);
            canvas->drawRect(ToSkRect(rect), paint);
        }
    }

    void FillRoundedRect(const Rect& rect, const Color& color, float radius) override {
        if (SkCanvas* canvas = Canvas()) {
            SkPaint paint;
            paint.setColor(ToSkColor(color));
            paint.setStyle(SkPaint::kFill_Style);
            paint.setAntiAlias(true);
            canvas->drawRoundRect(ToSkRect(rect), radius, radius, paint);
        }
    }

    void DrawRect(const Rect& rect, const Color& color, float strokeWidth) override {
        if (SkCanvas* canvas = Canvas()) {
            SkPaint paint;
            paint.setColor(ToSkColor(color));
            paint.setStyle(SkPaint::kStroke_Style);
            paint.setStrokeWidth(strokeWidth);
            paint.setAntiAlias(true);
            canvas->drawRect(ToSkRect(rect), paint);
        }
    }

    void DrawRoundedRect(const Rect& rect, const Color& color, float strokeWidth, float radius) override {
        if (SkCanvas* canvas = Canvas()) {
            SkPaint paint;
            paint.setColor(ToSkColor(color));
            paint.setStyle(SkPaint::kStroke_Style);
            paint.setStrokeWidth(strokeWidth);
            paint.setAntiAlias(true);
            canvas->drawRoundRect(ToSkRect(rect), radius, radius, paint);
        }
    }

    void DrawLine(const PointF& start, const PointF& end, const Color& color, float strokeWidth) override {
        if (SkCanvas* canvas = Canvas()) {
            SkPaint paint;
            paint.setColor(ToSkColor(color));
            paint.setStyle(SkPaint::kStroke_Style);
            paint.setStrokeWidth(strokeWidth);
            paint.setAntiAlias(true);
            canvas->drawLine(start.x, start.y, end.x, end.y, paint);
        }
    }

    void DrawPolyline(const std::vector<PointF>& points, const Color& color, float strokeWidth) override {
        if (points.size() < 2) {
            return;
        }
        if (SkCanvas* canvas = Canvas()) {
            SkPaint paint;
            paint.setColor(ToSkColor(color));
            paint.setStyle(SkPaint::kStroke_Style);
            paint.setStrokeWidth(strokeWidth);
            paint.setAntiAlias(true);

            SkPath path;
            path.moveTo(points.front().x, points.front().y);
            for (size_t i = 1; i < points.size(); ++i) {
                path.lineTo(points[i].x, points[i].y);
            }
            canvas->drawPath(path, paint);
        }
    }

    void PushRoundedClip(const Rect& rect, float radius) override {
        if (SkCanvas* canvas = Canvas()) {
            canvas->save();
            SkRRect clip;
            clip.setRectXY(ToSkRect(rect), radius, radius);
            canvas->clipRRect(clip, true);
        }
    }

    void PopLayer() override {
        if (SkCanvas* canvas = Canvas()) {
            canvas->restore();
        }
    }

    void DrawTextW(const wchar_t* text,
                   UINT32 len,
                   const Rect& rect,
                   const Color& color,
                   float fontSize,
                   const TextRenderOptions& options) override {
        if (SkCanvas* canvas = Canvas()) {
            if (!text || len == 0 || rect.Width() <= 0.0f || rect.Height() <= 0.0f) {
                return;
            }

            EnsureFontManager();

            SkFont font = skia_font::CreateSkiaFont(fontSize, m_defaultTypeface.get());

            SkiaTextLayout layout;
            if (!CreateSkiaTextLayout(
                text,
                len,
                fontSize,
                std::max(1.0f, rect.Width()),
                options.wrap,
                &layout)) {
                return;
            }
            if (layout.lines.empty()) {
                return;
            }
            float startTop = rect.top;
            if (options.verticalAlign == TextVerticalAlign::Center) {
                startTop = rect.top + std::max(0.0f, (rect.Height() - layout.blockHeight) * 0.5f);
            } else if (options.verticalAlign == TextVerticalAlign::End) {
                startTop = rect.bottom - layout.blockHeight;
            }

            canvas->save();
            canvas->clipRect(ToSkRect(rect), true);

            SkPaint paint;
            paint.setColor(ToSkColor(color));
            paint.setStyle(SkPaint::kFill_Style);
            paint.setAntiAlias(true);

            for (size_t i = 0; i < layout.lines.size(); ++i) {
                const float baseline = startTop + layout.ascent + static_cast<float>(i) * layout.lineHeight;
                if (baseline - layout.ascent > rect.bottom) {
                    break;
                }

                float drawX = rect.left;
                if (options.horizontalAlign == TextHorizontalAlign::Center) {
                    drawX = rect.left + std::max(0.0f, (rect.Width() - layout.lines[i].width) * 0.5f);
                } else if (options.horizontalAlign == TextHorizontalAlign::End) {
                    drawX = rect.right - layout.lines[i].width;
                }

                DrawSimpleTextWithFallback(
                    canvas,
                    layout.lines[i].text,
                    drawX,
                    baseline,
                    fontSize,
                    m_defaultTypeface.get(),
                    m_fontMgr.get(),
                    paint);
            }

            canvas->restore();
        }
    }

    Size GetBitmapSize(BitmapHandle bitmap) override {
        return bitmap ? bitmap->GetSize() : Size{};
    }

    BitmapHandle CreateBitmapFromBytes(const uint8_t* data, size_t size) override {
        if (!data || size == 0) {
            return nullptr;
        }

        sk_sp<SkData> encoded = SkData::MakeWithCopy(data, size);
        if (!encoded) {
            return nullptr;
        }

        sk_sp<SkImage> image = SkImages::DeferredFromEncodedData(encoded);
        if (!image) {
            return nullptr;
        }

        return std::make_shared<SkiaBitmapResource>(std::move(image));
    }

    void DrawBitmap(BitmapHandle bitmap, const Rect& rect) override {
        SkCanvas* canvas = Canvas();
        if (!canvas || !bitmap) {
            return;
        }

        auto skiaBitmap = std::dynamic_pointer_cast<SkiaBitmapResource>(bitmap);
        if (!skiaBitmap || !skiaBitmap->Image()) {
            return;
        }

        canvas->drawImageRect(
            skiaBitmap->Image(),
            ToSkRect(rect),
            m_bitmapSampling,
            nullptr);
    }

    SvgHandle CreateSvgFromBytes(const uint8_t* data, size_t size) override {
        if (!data || size == 0) {
            return nullptr;
        }
        EnsureFontManager();
        auto stream = SkMemoryStream::MakeCopy(data, size);
        if (!stream) {
            return nullptr;
        }
        sk_sp<SkSVGDOM> dom = SkSVGDOM::Builder()
            .setFontManager(m_fontMgr)
            .make(*stream);
        if (!dom) {
            return nullptr;
        }
        return std::make_shared<SkiaSvgResource>(std::move(dom));
    }

    Size GetSvgSize(SvgHandle svg) override {
        return svg ? svg->GetIntrinsicSize() : Size{};
    }

    void DrawSvg(SvgHandle svg, const Rect& rect) override {
        SkCanvas* canvas = Canvas();
        if (!canvas || !svg) {
            return;
        }
        auto skiaSvg = std::dynamic_pointer_cast<SkiaSvgResource>(svg);
        if (!skiaSvg || !skiaSvg->Dom()) {
            return;
        }
        SkAutoCanvasRestore restore(canvas, /*doSave=*/true);
        canvas->translate(rect.left, rect.top);
        skiaSvg->Dom()->setContainerSize(SkSize::Make(rect.Width(), rect.Height()));
        skiaSvg->Dom()->render(canvas);
    }

private:
    void EnsureFontManager() {
        if (!m_fontMgr) {
            m_fontMgr = skia_font::CreateDefaultFontManager();
        }
        if (!m_defaultTypeface) {
            m_defaultTypeface = skia_font::CreateDefaultTypeface(m_fontMgr.get());
        }
    }

    bool EnsureSurface() {
        return m_surface || RecreateSurface();
    }

    void ReleasePresentSurface() {
        m_surface.reset();
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

    bool CreatePresentSurface() {
        ReleasePresentSurface();
        if (m_width == 0 || m_height == 0) {
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
        bmi.bmiHeader.biWidth = static_cast<LONG>(m_width);
        bmi.bmiHeader.biHeight = -static_cast<LONG>(m_height); // top-down
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;

        m_dib = CreateDIBSection(m_memDC, &bmi, DIB_RGB_COLORS, &m_dibBits, nullptr, 0);
        if (!m_dib || !m_dibBits) {
            ReleasePresentSurface();
            return false;
        }
        m_oldBitmap = static_cast<HBITMAP>(SelectObject(m_memDC, m_dib));
        ZeroMemory(m_dibBits, static_cast<size_t>(m_width) * m_height * 4u);
        return true;
    }

    void PresentLayered() {
        if (!m_memDC) {
            return;
        }
        POINT src{0, 0};
        SIZE size{static_cast<LONG>(m_width), static_cast<LONG>(m_height)};
        BLENDFUNCTION blend{};
        blend.BlendOp = AC_SRC_OVER;
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

    void PresentHwnd() {
        if (!m_memDC) {
            return;
        }
        HDC dc = GetDC(m_hwnd);
        if (!dc) {
            return;
        }
        // Zero-copy present: Skia draws into the DIB, BitBlt to the window.
        BitBlt(
            dc,
            0,
            0,
            static_cast<int>(m_width),
            static_cast<int>(m_height),
            m_memDC,
            0,
            0,
            SRCCOPY);
        ReleaseDC(m_hwnd, dc);
    }

    bool RecreateSurface() {
        if (!m_hwnd) {
            return false;
        }

        if (m_width == 0 || m_height == 0) {
            RECT clientRect{};
            GetClientRect(m_hwnd, &clientRect);
            m_width = static_cast<UINT>(std::max<LONG>(1, clientRect.right - clientRect.left));
            m_height = static_cast<UINT>(std::max<LONG>(1, clientRect.bottom - clientRect.top));
        }

        if (m_width == 0 || m_height == 0) {
            ReleasePresentSurface();
            return false;
        }

        if (!CreatePresentSurface()) {
            return false;
        }

        const size_t rowBytes = static_cast<size_t>(m_width) * 4;
        // Draw directly into the DIB (shared by HWND BitBlt and layered ULW).
        const SkImageInfo info = SkImageInfo::MakeN32Premul(static_cast<int>(m_width), static_cast<int>(m_height));
        m_surface = SkSurfaces::WrapPixels(info, m_dibBits, rowBytes, &m_surfaceProps);
        m_frameDirty = true;
        return static_cast<bool>(m_surface);
    }

    SkCanvas* Canvas() {
        return EnsureSurface() ? m_surface->getCanvas() : nullptr;
    }

    HWND m_hwnd = nullptr;
    PresentMode m_presentMode = PresentMode::Hwnd;
    UINT m_width = 0;
    UINT m_height = 0;
    bool m_frameDirty = false;
    HDC m_memDC = nullptr;
    HBITMAP m_dib = nullptr;
    HBITMAP m_oldBitmap = nullptr;
    void* m_dibBits = nullptr;
    sk_sp<SkSurface> m_surface;
    SkSurfaceProps m_surfaceProps{
        SkSurfaceProps::kUseDeviceIndependentFonts_Flag,
        kRGB_H_SkPixelGeometry
    };
    sk_sp<SkFontMgr> m_fontMgr;
    sk_sp<SkTypeface> m_defaultTypeface;
    SkSamplingOptions m_bitmapSampling{SkFilterMode::kLinear, SkMipmapMode::kLinear};
};

} // namespace

std::unique_ptr<IRenderer> CreateSkiaRenderer() {
    return std::make_unique<SkiaRenderer>();
}

#else

std::unique_ptr<IRenderer> CreateSkiaRenderer() {
    return nullptr;
}

#endif
