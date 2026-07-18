#include "skia_text_layout.h"
#include "skia_font_shared.h"

#if defined(AI_WIN_UI_HAS_VCPKG_SKIA) || defined(AI_WIN_UI_HAS_LOCAL_SKIA)
#include "include/core/SkFont.h"
#include "include/core/SkFontMetrics.h"
#include "include/core/SkFontTypes.h"

#include <algorithm>
#include <cmath>
#include <cwctype>
#include <string>
#include <utility>
#include <vector>

namespace {

std::wstring TrimTrailingWhitespace(const std::wstring& value) {
    size_t end = value.size();
    while (end > 0 && std::iswspace(value[end - 1]) != 0) {
        --end;
    }
    return value.substr(0, end);
}

float MeasureWideTextWidth(SkFontMgr* fontMgr,
                           SkTypeface* defaultTypeface,
                           float fontSize,
                           const std::wstring& text) {
    return skia_font::MeasureTextWidthWithFallback(
        fontMgr,
        defaultTypeface,
        fontSize,
        text.c_str(),
        static_cast<uint32_t>(text.size()));
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

// Soft break after CJK / fullwidth ideographs (closer to DirectWrite WRAP / UAX#14 ID).
bool IsCjkOrFullwidth(wchar_t ch) {
    if (ch >= 0x3400 && ch <= 0x4DBF) {
        return true; // CJK Extension A
    }
    if (ch >= 0x4E00 && ch <= 0x9FFF) {
        return true; // CJK Unified
    }
    if (ch >= 0xF900 && ch <= 0xFAFF) {
        return true; // CJK Compatibility
    }
    if (ch >= 0x3000 && ch <= 0x303F) {
        return true; // CJK punctuation / ideographic space
    }
    if (ch >= 0xFF00 && ch <= 0xFFEF) {
        return true; // Fullwidth forms
    }
    // Common CJK punctuation outside the blocks above.
    switch (ch) {
    case L'，':
    case L'。':
    case L'、':
    case L'；':
    case L'：':
    case L'？':
    case L'！':
    case L'（':
    case L'）':
    case L'【':
    case L'】':
    case L'「':
    case L'」':
    case L'『':
    case L'』':
    case L'《':
    case L'》':
    case L'—':
    case L'…':
        return true;
    default:
        return false;
    }
}

std::vector<SkiaTextLayoutLine> WrapWideLine(const std::wstring& line,
                                             SkFontMgr* fontMgr,
                                             SkTypeface* defaultTypeface,
                                             float fontSize,
                                             float maxWidth) {
    if (line.empty()) {
        return {SkiaTextLayoutLine{}};
    }

    const float unwrappedWidth = MeasureWideTextWidth(fontMgr, defaultTypeface, fontSize, line);
    if (maxWidth <= 1.0f || unwrappedWidth <= maxWidth) {
        return {SkiaTextLayoutLine{line, unwrappedWidth}};
    }

    auto measureRange = [&](size_t begin, size_t end) -> float {
        if (end <= begin || begin >= line.size()) {
            return 0.0f;
        }
        end = std::min(end, line.size());
        return MeasureWideTextWidth(
            fontMgr,
            defaultTypeface,
            fontSize,
            line.substr(begin, end - begin));
    };

    // Small slack so Skia emergency mid-word breaks match DirectWrite packing
    // (hinting / subpixel measure can read ~0.5–1px wider per line).
    const float fitLimit = maxWidth + 1.0f;

    std::vector<SkiaTextLayoutLine> wrapped;
    size_t lineStart = 0;
    while (lineStart < line.size()) {
        while (lineStart < line.size() && std::iswspace(line[lineStart]) != 0) {
            ++lineStart;
        }
        if (lineStart >= line.size()) {
            break;
        }

        // lastSoftEnd: exclusive end of the latest soft-break that still fits.
        // Soft breaks: after whitespace, hyphen, or CJK/fullwidth (DWrite-like).
        // Only updated while the prefix still fits — never on the overflowing glyph.
        size_t lastSoftEnd = std::wstring::npos;
        size_t lastFitEnd = lineStart; // exclusive end of longest fitting prefix
        bool emitted = false;

        for (size_t i = lineStart; i < line.size(); ++i) {
            const size_t tryEnd = i + 1;
            const float width = measureRange(lineStart, tryEnd);
            if (width <= fitLimit) {
                lastFitEnd = tryEnd;
                const wchar_t ch = line[i];
                // Soft opportunities ≈ DWrite WRAP / common Western + CJK breaks.
                const bool soft =
                    std::iswspace(ch) != 0 ||
                    ch == L'-' || ch == L'/' ||
                    ch == L'.' || ch == L',' || ch == L';' || ch == L':' ||
                    ch == L'!' || ch == L'?' || ch == L')' || ch == L']' ||
                    IsCjkOrFullwidth(ch);
                if (soft) {
                    lastSoftEnd = tryEnd;
                }
                continue;
            }

            // Overflow at character i.
            size_t cutEnd = lastSoftEnd;
            if (cutEnd == std::wstring::npos || cutEnd <= lineStart) {
                // No soft break in the fitting prefix: hard-break after last fit.
                cutEnd = (lastFitEnd > lineStart) ? lastFitEnd : tryEnd;
            }

            if (cutEnd <= lineStart) {
                // Pathological: single glyph wider than maxWidth.
                cutEnd = tryEnd;
            }

            std::wstring segment = TrimTrailingWhitespace(line.substr(lineStart, cutEnd - lineStart));
            if (!segment.empty()) {
                wrapped.push_back(SkiaTextLayoutLine{
                    segment,
                    MeasureWideTextWidth(fontMgr, defaultTypeface, fontSize, segment)});
            }
            lineStart = cutEnd;
            emitted = true;
            break;
        }

        if (!emitted) {
            std::wstring segment = TrimTrailingWhitespace(line.substr(lineStart));
            if (!segment.empty()) {
                wrapped.push_back(SkiaTextLayoutLine{
                    segment,
                    MeasureWideTextWidth(fontMgr, defaultTypeface, fontSize, segment)});
            }
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
                                                SkFontMgr* fontMgr,
                                                SkTypeface* defaultTypeface,
                                                float fontSize,
                                                float maxWidth,
                                                TextWrapMode wrapMode) {
    const std::vector<std::wstring> splitLines = SplitWideLines(text, len);
    if (wrapMode == TextWrapMode::NoWrap) {
        const std::wstring firstLine = splitLines.empty() ? std::wstring{} : splitLines.front();
        return {SkiaTextLayoutLine{firstLine, MeasureWideTextWidth(fontMgr, defaultTypeface, fontSize, firstLine)}};
    }

    std::vector<SkiaTextLayoutLine> wrappedLines;
    for (const std::wstring& line : splitLines) {
        const auto wrapped = WrapWideLine(line, fontMgr, defaultTypeface, fontSize, maxWidth);
        wrappedLines.insert(wrappedLines.end(), wrapped.begin(), wrapped.end());
    }
    if (wrappedLines.empty()) {
        wrappedLines.emplace_back();
    }
    return wrappedLines;
}

struct FontBundle {
    sk_sp<SkFontMgr> fontMgr;
    sk_sp<SkTypeface> typeface;
    SkFont font;
};

FontBundle CreateFontBundle(float fontSize, bool bold, bool italic) {
    FontBundle bundle;
    static sk_sp<SkFontMgr> s_fontMgr = skia_font::CreateDefaultFontManager();
    if (!s_fontMgr) {
        return bundle;
    }
    bundle.fontMgr = s_fontMgr;
    bundle.typeface = skia_font::CreateDefaultTypeface(s_fontMgr.get(), bold, italic);
    if (!bundle.typeface) {
        return bundle;
    }
    bundle.font = skia_font::CreateSkiaFont(fontSize, bundle.typeface.get());
    return bundle;
}

} // namespace

void ApplyEllipsisToFirstLine(
    SkiaTextLayoutLine& line,
    SkFontMgr* fontMgr,
    SkTypeface* typeface,
    float fontSize,
    float maxWidth) {
    if (line.width <= maxWidth + 0.5f || maxWidth <= 1.0f) {
        return;
    }
    const std::wstring ellipsis = L"\u2026";
    const float ellipsisW = MeasureWideTextWidth(fontMgr, typeface, fontSize, ellipsis);
    if (ellipsisW >= maxWidth) {
        // Degenerate: only show ellipsis (or empty if even that does not fit).
        line.text = ellipsisW <= maxWidth + 0.5f ? ellipsis : std::wstring{};
        line.width = MeasureWideTextWidth(fontMgr, typeface, fontSize, line.text);
        return;
    }
    const float budget = maxWidth - ellipsisW;
    size_t lo = 0;
    size_t hi = line.text.size();
    size_t best = 0;
    while (lo <= hi && hi <= line.text.size()) {
        const size_t mid = lo + (hi - lo) / 2;
        const std::wstring prefix = line.text.substr(0, mid);
        const float w = MeasureWideTextWidth(fontMgr, typeface, fontSize, prefix);
        if (w <= budget + 0.5f) {
            best = mid;
            if (mid == line.text.size()) {
                break;
            }
            lo = mid + 1;
        } else {
            if (mid == 0) {
                break;
            }
            hi = mid - 1;
        }
    }
    line.text = line.text.substr(0, best) + ellipsis;
    line.width = MeasureWideTextWidth(fontMgr, typeface, fontSize, line.text);
}

bool CreateSkiaTextLayout(const wchar_t* text,
                          uint32_t len,
                          float fontSize,
                          float maxWidth,
                          TextWrapMode wrapMode,
                          SkiaTextLayout* layout,
                          bool bold,
                          bool italic,
                          bool ellipsis) {
    if (!layout) {
        return false;
    }

    *layout = SkiaTextLayout{};
    if (!text || len == 0 || fontSize <= 0.0f) {
        return true;
    }

    FontBundle fonts = CreateFontBundle(fontSize, bold, italic);
    if (!fonts.font.getTypeface()) {
        return false;
    }

    const float widthBudget = std::max(1.0f, maxWidth);
    layout->lines = LayoutTextLines(
        text,
        len,
        fonts.fontMgr.get(),
        fonts.typeface.get(),
        fontSize,
        widthBudget,
        wrapMode);

    // R6: single-line ellipsis only (NoWrap).
    if (ellipsis && wrapMode == TextWrapMode::NoWrap && !layout->lines.empty()) {
        ApplyEllipsisToFirstLine(
            layout->lines.front(),
            fonts.fontMgr.get(),
            fonts.typeface.get(),
            fontSize,
            widthBudget);
    }

    SkFontMetrics metrics{};
    fonts.font.getMetrics(&metrics);

    layout->ascent = std::isfinite(metrics.fAscent) ? -metrics.fAscent : fontSize * 0.8f;
    layout->descent = std::isfinite(metrics.fDescent) ? metrics.fDescent : fontSize * 0.25f;
    layout->lineHeight = std::max(fonts.font.getSpacing(), layout->ascent + layout->descent);
    layout->textHeight = layout->ascent + layout->descent;
    layout->maxLineWidth = 0.0f;
    for (const auto& line : layout->lines) {
        layout->maxLineWidth = std::max(layout->maxLineWidth, line.width);
    }

    // blockHeight uses (lines - 1) * lineHeight + textHeight so that:
    //   - Single-line text reports textHeight (ascent + descent), matching
    //     DirectWrite's DWRITE_TEXT_METRICS.height for one line.
    //   - Multi-line text reserves full line spacing between baselines
    //     (lineHeight) but only needs textHeight from the last baseline to
    //     the bottom. This keeps vertical centering and End alignment
    //     closer to DirectWrite's paragraph layout.
    layout->blockHeight = layout->lines.empty()
        ? 0.0f
        : static_cast<float>(layout->lines.size() - 1) * layout->lineHeight + layout->textHeight;

    return true;
}

Size MeasureSkiaTextLayout(const wchar_t* text,
                           uint32_t len,
                           float fontSize,
                           float maxWidth,
                           TextWrapMode wrapMode,
                           bool bold,
                           bool italic,
                           bool ellipsis) {
    SkiaTextLayout layout;
    if (!CreateSkiaTextLayout(text, len, fontSize, maxWidth, wrapMode, &layout, bold, italic, ellipsis)) {
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
                          SkiaTextLayout* layout,
                          bool,
                          bool,
                          bool) {
    if (layout) {
        *layout = SkiaTextLayout{};
    }
    return false;
}

Size MeasureSkiaTextLayout(const wchar_t*,
                           uint32_t,
                           float,
                           float,
                           TextWrapMode,
                           bool,
                           bool,
                           bool) {
    return {};
}

#endif
