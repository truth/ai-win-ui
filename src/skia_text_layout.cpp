#include "skia_text_layout.h"

#if defined(AI_WIN_UI_HAS_VCPKG_SKIA) || defined(AI_WIN_UI_HAS_LOCAL_SKIA)
#include "include/core/SkFont.h"
#include "include/core/SkFontMetrics.h"
#include "include/core/SkFontMgr.h"
#include "include/core/SkFontTypes.h"
#include "include/core/SkTypeface.h"
#include "include/ports/SkTypeface_win.h"

#include <algorithm>
#include <cmath>
#include <cwctype>
#include <string>
#include <utility>
#include <vector>

namespace {

std::string WideToUtf8(const wchar_t* text, uint32_t len) {
    if (!text || len == 0) {
        return {};
    }

    const int size = WideCharToMultiByte(
        CP_UTF8,
        0,
        text,
        static_cast<int>(len),
        nullptr,
        0,
        nullptr,
        nullptr);
    if (size <= 0) {
        return {};
    }

    std::string utf8(static_cast<size_t>(size), '\0');
    WideCharToMultiByte(
        CP_UTF8,
        0,
        text,
        static_cast<int>(len),
        utf8.data(),
        size,
        nullptr,
        nullptr);
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
    const std::string utf8 = WideToUtf8(text.c_str(), static_cast<uint32_t>(text.size()));
    if (utf8.empty()) {
        return 0.0f;
    }

    return font.measureText(utf8.data(), utf8.size(), SkTextEncoding::kUTF8);
}

std::vector<std::wstring> SplitWideLines(const wchar_t* text, uint32_t len) {
    std::vector<std::wstring> lines;
    if (!text || len == 0) {
        lines.emplace_back();
        return lines;
    }

    std::wstring current;
    current.reserve(len);
    for (uint32_t i = 0; i < len; ++i) {
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

std::vector<SkiaTextLayoutLine> WrapWideLine(const std::wstring& line,
                                             const SkFont& font,
                                             float maxWidth) {
    if (line.empty()) {
        return {SkiaTextLayoutLine{}};
    }

    const float unwrappedWidth = MeasureWideTextWidth(font, line);
    if (maxWidth <= 1.0f || unwrappedWidth <= maxWidth) {
        return {SkiaTextLayoutLine{line, unwrappedWidth}};
    }

    std::vector<SkiaTextLayoutLine> wrapped;
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
                wrapped.push_back(SkiaTextLayoutLine{candidate, MeasureWideTextWidth(font, candidate)});
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
                wrapped.push_back(SkiaTextLayoutLine{segment, MeasureWideTextWidth(font, segment)});
            } else {
                const std::wstring segment = candidate.substr(0, candidate.size() - 1);
                wrapped.push_back(SkiaTextLayoutLine{segment, MeasureWideTextWidth(font, segment)});
                lineStart = i;
            }

            emitted = true;
            break;
        }

        if (!emitted) {
            std::wstring segment = TrimTrailingWhitespace(line.substr(lineStart));
            wrapped.push_back(SkiaTextLayoutLine{segment, MeasureWideTextWidth(font, segment)});
            break;
        }
    }

    if (wrapped.empty()) {
        wrapped.push_back(SkiaTextLayoutLine{line, unwrappedWidth});
    }
    return wrapped;
}

std::vector<SkiaTextLayoutLine> LayoutTextLines(const wchar_t* text,
                                                uint32_t len,
                                                const SkFont& font,
                                                float maxWidth,
                                                TextWrapMode wrapMode) {
    const std::vector<std::wstring> splitLines = SplitWideLines(text, len);
    if (wrapMode == TextWrapMode::NoWrap) {
        const std::wstring firstLine = splitLines.empty() ? std::wstring{} : splitLines.front();
        return {SkiaTextLayoutLine{firstLine, MeasureWideTextWidth(font, firstLine)}};
    }

    std::vector<SkiaTextLayoutLine> wrappedLines;
    for (const std::wstring& line : splitLines) {
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

SkFont CreateDefaultFont(float fontSize) {
    static sk_sp<SkFontMgr> s_fontMgr = SkFontMgr_New_DirectWrite();
    static sk_sp<SkTypeface> s_typeface = CreateDefaultTypeface(s_fontMgr.get());

    SkFont font;
    font.setTypeface(s_typeface);
    font.setSize(fontSize);
    font.setEdging(SkFont::Edging::kSubpixelAntiAlias);
    font.setSubpixel(true);
    font.setBaselineSnap(true);
    font.setHinting(SkFontHinting::kSlight);
    return font;
}

} // namespace

bool CreateSkiaTextLayout(const wchar_t* text,
                          uint32_t len,
                          float fontSize,
                          float maxWidth,
                          TextWrapMode wrapMode,
                          SkiaTextLayout* layout) {
    if (!layout) {
        return false;
    }

    *layout = SkiaTextLayout{};
    if (!text || len == 0 || fontSize <= 0.0f) {
        return true;
    }

    SkFont font = CreateDefaultFont(fontSize);
    if (!font.getTypeface()) {
        return false;
    }

    layout->lines = LayoutTextLines(text, len, font, std::max(1.0f, maxWidth), wrapMode);

    SkFontMetrics metrics{};
    font.getMetrics(&metrics);

    layout->ascent = std::isfinite(metrics.fAscent) ? -metrics.fAscent : fontSize * 0.8f;
    layout->descent = std::isfinite(metrics.fDescent) ? metrics.fDescent : fontSize * 0.25f;
    layout->lineHeight = std::max(font.getSpacing(), layout->ascent + layout->descent);
    layout->textHeight = layout->ascent + layout->descent;
    layout->maxLineWidth = 0.0f;
    for (const auto& line : layout->lines) {
        layout->maxLineWidth = std::max(layout->maxLineWidth, line.width);
    }
    layout->blockHeight = layout->lines.empty()
        ? 0.0f
        : layout->textHeight +
            std::max(0.0f, static_cast<float>(layout->lines.size() - 1)) * layout->lineHeight;

    return true;
}

Size MeasureSkiaTextLayout(const wchar_t* text,
                           uint32_t len,
                           float fontSize,
                           float maxWidth,
                           TextWrapMode wrapMode) {
    SkiaTextLayout layout;
    if (!CreateSkiaTextLayout(text, len, fontSize, maxWidth, wrapMode, &layout)) {
        return {};
    }
    return Size{layout.maxLineWidth, layout.blockHeight};
}

#else

bool CreateSkiaTextLayout(const wchar_t*,
                          uint32_t,
                          float,
                          float,
                          TextWrapMode,
                          SkiaTextLayout* layout) {
    if (layout) {
        *layout = SkiaTextLayout{};
    }
    return false;
}

Size MeasureSkiaTextLayout(const wchar_t*,
                           uint32_t,
                           float,
                           float,
                           TextWrapMode) {
    return {};
}

#endif
