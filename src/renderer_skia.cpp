#include "renderer.h"

#if defined(AI_WIN_UI_HAS_VCPKG_SKIA) || defined(AI_WIN_UI_HAS_LOCAL_SKIA)
#include "include/core/SkCanvas.h"
#include "include/core/SkColor.h"
#include "include/core/SkData.h"
#include "include/core/SkFont.h"
#include "include/core/SkFontMetrics.h"
#include "include/core/SkFontMgr.h"
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
#include "include/ports/SkTypeface_win.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <cwctype>
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

std::wstring TrimTrailingWhitespace(const std::wstring& value) {
    size_t end = value.size();
    while (end > 0 && std::iswspace(value[end - 1]) != 0) {
        --end;
    }
    return value.substr(0, end);
}

float MeasureWideTextWidth(const SkFont& font, const std::wstring& text) {
    const std::string utf8 = WideToUtf8(text.c_str(), static_cast<UINT32>(text.size()));
    if (utf8.empty()) {
        return 0.0f;
    }
    return font.measureText(utf8.data(), utf8.size(), SkTextEncoding::kUTF8);
}

std::vector<std::wstring> SplitWideLines(const wchar_t* text, UINT32 len) {
    std::vector<std::wstring> lines;
    if (!text || len == 0) {
        lines.emplace_back();
        return lines;
    }

    std::wstring current;
    current.reserve(len);
    for (UINT32 i = 0; i < len; ++i) {
        const wchar_t ch = text[i];
        if (ch == L'\r') {
            if (i + 1 < len && text[i + 1] == L'\n') {
                ++i;
            }
            lines.push_back(current);
            current.clear();
            continue;
        }
        if (ch == L'\n') {
            lines.push_back(current);
            current.clear();
            continue;
        }
        current.push_back(ch);
    }
    lines.push_back(current);
    return lines;
}

std::vector<std::wstring> WrapWideLine(const std::wstring& line, const SkFont& font, float maxWidth) {
    if (line.empty()) {
        return {L""};
    }

    if (maxWidth <= 1.0f || MeasureWideTextWidth(font, line) <= maxWidth) {
        return {line};
    }

    std::vector<std::wstring> wrapped;
    size_t lineStart = 0;
    while (lineStart < line.size()) {
        std::wstring candidate;
        candidate.reserve(line.size() - lineStart);
        size_t lastWhitespaceBreak = std::wstring::npos;
        bool emitted = false;

        for (size_t i = lineStart; i < line.size(); ++i) {
            candidate.push_back(line[i]);
            if (std::iswspace(line[i]) != 0) {
                lastWhitespaceBreak = i;
            }

            if (MeasureWideTextWidth(font, candidate) <= maxWidth) {
                continue;
            }

            if (candidate.size() == 1) {
                wrapped.push_back(candidate);
                lineStart = i + 1;
                emitted = true;
                break;
            }

            if (lastWhitespaceBreak != std::wstring::npos && lastWhitespaceBreak >= lineStart) {
                std::wstring segment = TrimTrailingWhitespace(
                    line.substr(lineStart, lastWhitespaceBreak - lineStart + 1));
                if (segment.empty()) {
                    segment = line.substr(lineStart, 1);
                    lineStart += 1;
                } else {
                    lineStart = lastWhitespaceBreak + 1;
                }
                wrapped.push_back(segment);
            } else {
                wrapped.push_back(candidate.substr(0, candidate.size() - 1));
                lineStart = i;
            }

            emitted = true;
            break;
        }

        if (!emitted) {
            wrapped.push_back(TrimTrailingWhitespace(line.substr(lineStart)));
            break;
        }
    }

    if (wrapped.empty()) {
        wrapped.push_back(line);
    }
    return wrapped;
}

std::vector<std::wstring> LayoutWrappedText(const wchar_t* text, UINT32 len, const SkFont& font, float maxWidth) {
    std::vector<std::wstring> wrappedLines;
    for (const std::wstring& line : SplitWideLines(text, len)) {
        const auto wrapped = WrapWideLine(line, font, maxWidth);
        wrappedLines.insert(wrappedLines.end(), wrapped.begin(), wrapped.end());
    }
    if (wrappedLines.empty()) {
        wrappedLines.emplace_back();
    }
    return wrappedLines;
}

sk_sp<SkTypeface> TryMatchFamily(SkFontMgr* fontMgr, const char* familyName) {
    if (!fontMgr || !familyName || !familyName[0]) {
        return nullptr;
    }
    return fontMgr->matchFamilyStyle(familyName, SkFontStyle());
}

sk_sp<SkTypeface> CreateDefaultTypeface(SkFontMgr* fontMgr) {
    if (!fontMgr) {
        return nullptr;
    }

    if (sk_sp<SkTypeface> typeface = TryMatchFamily(fontMgr, "Segoe UI")) {
        return typeface;
    }
    if (sk_sp<SkTypeface> typeface = TryMatchFamily(fontMgr, "Arial")) {
        return typeface;
    }
    if (sk_sp<SkTypeface> typeface = TryMatchFamily(fontMgr, "Tahoma")) {
        return typeface;
    }

    return fontMgr->legacyMakeTypeface(nullptr, SkFontStyle());
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
        EnsureFontManager();
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
            if (!text || len == 0 || rect.Width() <= 0.0f || rect.Height() <= 0.0f) {
                return;
            }

            EnsureFontManager();

            SkFont font;
            font.setTypeface(m_defaultTypeface);
            font.setSize(fontSize);
            font.setEdging(SkFont::Edging::kAntiAlias);
            font.setSubpixel(true);
            font.setHinting(SkFontHinting::kSlight);

            const std::vector<std::wstring> lines = LayoutWrappedText(
                text,
                len,
                font,
                std::max(1.0f, rect.Width()));
            if (lines.empty()) {
                return;
            }

            SkFontMetrics metrics{};
            font.getMetrics(&metrics);

            const float ascent = std::isfinite(metrics.fAscent) ? -metrics.fAscent : fontSize * 0.8f;
            const float descent = std::isfinite(metrics.fDescent) ? metrics.fDescent : fontSize * 0.25f;
            const float lineHeight = std::max(font.getSpacing(), ascent + descent);
            const float textHeight = ascent + descent;
            const float blockHeight = textHeight + std::max(0.0f, static_cast<float>(lines.size() - 1)) * lineHeight;
            const float startTop = rect.top + std::max(0.0f, (rect.Height() - blockHeight) * 0.5f);

            canvas->save();
            canvas->clipRect(ToSkRect(rect), true);

            SkPaint paint;
            paint.setColor(ToSkColor(color));
            paint.setStyle(SkPaint::kFill_Style);
            paint.setAntiAlias(true);

            for (size_t i = 0; i < lines.size(); ++i) {
                const float baseline = startTop + ascent + static_cast<float>(i) * lineHeight;
                if (baseline - ascent > rect.bottom) {
                    break;
                }

                const std::string utf8 = WideToUtf8(lines[i].c_str(), static_cast<UINT32>(lines[i].size()));
                if (utf8.empty()) {
                    continue;
                }

                canvas->drawSimpleText(
                    utf8.data(),
                    utf8.size(),
                    SkTextEncoding::kUTF8,
                    rect.left,
                    baseline,
                    font,
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
            SkSamplingOptions(),
            nullptr);
    }

private:
    void EnsureFontManager() {
        if (!m_fontMgr) {
            m_fontMgr = SkFontMgr_New_DirectWrite();
        }
        if (!m_defaultTypeface) {
            m_defaultTypeface = CreateDefaultTypeface(m_fontMgr.get());
        }
    }

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
    sk_sp<SkFontMgr> m_fontMgr;
    sk_sp<SkTypeface> m_defaultTypeface;
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
