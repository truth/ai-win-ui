#include "renderer.h"

#if defined(AI_WIN_UI_HAS_VCPKG_SKIA) || defined(AI_WIN_UI_HAS_LOCAL_SKIA)
#include "include/core/SkCanvas.h"
#include "include/core/SkColor.h"
#include "include/core/SkData.h"
#include "include/core/SkFont.h"
#include "include/core/SkImage.h"
#include "include/core/SkImageInfo.h"
#include "include/core/SkPaint.h"
#include "include/core/SkRRect.h"
#include "include/core/SkSamplingOptions.h"
#include "include/core/SkSurface.h"
#include "include/core/SkSurfaceProps.h"
#include "include/core/SkTextBlob.h"
#include "include/core/SkTypeface.h"
#include "include/core/SkTypes.h"

#include <algorithm>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

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

std::string WideToUtf8(const wchar_t* text, UINT32 len) {
    if (!text || len == 0) {
        return {};
    }
    const int size = WideCharToMultiByte(CP_UTF8, 0, text, static_cast<int>(len), nullptr, 0, nullptr, nullptr);
    if (size <= 0) {
        return {};
    }
    std::string utf8(static_cast<size_t>(size), '\0');
    WideCharToMultiByte(CP_UTF8, 0, text, static_cast<int>(len), utf8.data(), size, nullptr, nullptr);
    return utf8;
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

class SkiaRenderer final : public IRenderer {
public:
    RendererBackend Backend() const override {
        return RendererBackend::Skia;
    }

    bool Initialize(HWND hwnd) override {
        m_hwnd = hwnd;
        return RecreateSurface();
    }

    void Resize(UINT width, UINT height) override {
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
    }

    void EndFrame() override {
        if (!m_surface || m_pixels.empty() || !m_hwnd || m_width == 0 || m_height == 0) {
            return;
        }

        BITMAPINFO bmi{};
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = static_cast<LONG>(m_width);
        bmi.bmiHeader.biHeight = -static_cast<LONG>(m_height);
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;

        HDC dc = GetDC(m_hwnd);
        if (!dc) {
            return;
        }

        StretchDIBits(
            dc,
            0,
            0,
            static_cast<int>(m_width),
            static_cast<int>(m_height),
            0,
            0,
            static_cast<int>(m_width),
            static_cast<int>(m_height),
            m_pixels.data(),
            &bmi,
            DIB_RGB_COLORS,
            SRCCOPY);

        ReleaseDC(m_hwnd, dc);
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

    void DrawTextW(const wchar_t* text, UINT32 len, const Rect& rect, const Color& color, float fontSize) override {
        if (SkCanvas* canvas = Canvas()) {
            const std::string utf8 = WideToUtf8(text, len);
            if (utf8.empty()) {
                return;
            }

            canvas->save();
            canvas->clipRect(ToSkRect(rect), true);

            SkPaint paint;
            paint.setColor(ToSkColor(color));
            paint.setAntiAlias(true);

            SkFont font;
            font.setSize(fontSize);
            font.setEdging(SkFont::Edging::kAntiAlias);
            font.setSubpixel(true);

            // This first-pass backend draws single-line text and relies on the
            // existing text measurer for sizing until a fuller Skia text path lands.
            const float baseline = rect.top + fontSize;
            canvas->drawSimpleText(
                utf8.data(),
                utf8.size(),
                SkTextEncoding::kUTF8,
                rect.left,
                baseline,
                font,
                paint);

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
            SkSamplingOptions(),
            nullptr);
    }

private:
    bool EnsureSurface() {
        return m_surface || RecreateSurface();
    }

    bool RecreateSurface() {
        if (!m_hwnd) {
            return false;
        }

        if (m_width == 0 || m_height == 0) {
            RECT clientRect{};
            GetClientRect(m_hwnd, &clientRect);
            m_width = static_cast<UINT>(std::max<LONG>(0, clientRect.right - clientRect.left));
            m_height = static_cast<UINT>(std::max<LONG>(0, clientRect.bottom - clientRect.top));
        }

        if (m_width == 0 || m_height == 0) {
            m_surface.reset();
            m_pixels.clear();
            return false;
        }

        const size_t rowBytes = static_cast<size_t>(m_width) * 4;
        m_pixels.assign(rowBytes * m_height, 0);

        const SkImageInfo info = SkImageInfo::MakeN32Premul(static_cast<int>(m_width), static_cast<int>(m_height));
        m_surface = SkSurfaces::WrapPixels(info, m_pixels.data(), rowBytes, &m_surfaceProps);
        return static_cast<bool>(m_surface);
    }

    SkCanvas* Canvas() {
        return EnsureSurface() ? m_surface->getCanvas() : nullptr;
    }

    HWND m_hwnd = nullptr;
    UINT m_width = 0;
    UINT m_height = 0;
    std::vector<uint8_t> m_pixels;
    sk_sp<SkSurface> m_surface;
    SkSurfaceProps m_surfaceProps{0, kUnknown_SkPixelGeometry};
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
