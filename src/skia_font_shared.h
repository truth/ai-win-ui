#pragma once

#include "renderer.h"

#include <algorithm>
#include <cstdint>
#include <string>

#if defined(AI_WIN_UI_HAS_VCPKG_SKIA) || defined(AI_WIN_UI_HAS_LOCAL_SKIA)
#include "include/core/SkFont.h"
#include "include/core/SkFontMgr.h"
#include "include/core/SkFontTypes.h"
#include "include/core/SkTypes.h"
#include "include/core/SkTypeface.h"
#include "include/ports/SkTypeface_win.h"

namespace skia_font {

inline std::string WideToUtf8(const wchar_t* text, uint32_t len) {
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

inline bool IsHighSurrogate(wchar_t ch) {
    return ch >= 0xD800 && ch <= 0xDBFF;
}

inline bool IsLowSurrogate(wchar_t ch) {
    return ch >= 0xDC00 && ch <= 0xDFFF;
}

inline SkUnichar DecodeUtf16At(const wchar_t* text, uint32_t len, uint32_t index, uint32_t* units = nullptr) {
    if (!text || index >= len) {
        if (units) {
            *units = 0;
        }
        return 0;
    }

    const wchar_t first = text[index];
    if (IsHighSurrogate(first) && index + 1 < len && IsLowSurrogate(text[index + 1])) {
        if (units) {
            *units = 2;
        }
        const SkUnichar high = static_cast<SkUnichar>(first - 0xD800);
        const SkUnichar low = static_cast<SkUnichar>(text[index + 1] - 0xDC00);
        return 0x10000 + ((high << 10) | low);
    }

    if (units) {
        *units = 1;
    }
    return static_cast<SkUnichar>(first);
}

inline sk_sp<SkTypeface> TryMatchFamily(SkFontMgr* fontMgr, const char* familyName) {
    if (!fontMgr || !familyName || !familyName[0]) {
        return nullptr;
    }
    return fontMgr->matchFamilyStyle(familyName, SkFontStyle());
}

inline sk_sp<SkTypeface> CreateDefaultTypeface(SkFontMgr* fontMgr) {
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

inline sk_sp<SkTypeface> MatchTypefaceForCharacter(SkFontMgr* fontMgr,
                                                   SkTypeface* defaultTypeface,
                                                   SkUnichar character) {
    if (defaultTypeface && defaultTypeface->unicharToGlyph(character) != 0) {
        return sk_ref_sp(defaultTypeface);
    }
    if (!fontMgr || character == 0) {
        return defaultTypeface ? sk_ref_sp(defaultTypeface) : nullptr;
    }

    const char* bcp47[] = {"zh-Hans", "zh-Hant", "zh-CN", "zh-TW", "ja", "ko"};
    if (sk_sp<SkTypeface> typeface = fontMgr->matchFamilyStyleCharacter(
            nullptr,
            SkFontStyle(),
            bcp47,
            6,
            character)) {
        return typeface;
    }

    return defaultTypeface ? sk_ref_sp(defaultTypeface) : fontMgr->legacyMakeTypeface(nullptr, SkFontStyle());
}

inline bool TypefaceSupportsText(SkTypeface* typeface, const wchar_t* text, uint32_t len) {
    if (!typeface || !text || len == 0) {
        return true;
    }
    for (uint32_t i = 0; i < len;) {
        uint32_t units = 0;
        const SkUnichar ch = DecodeUtf16At(text, len, i, &units);
        if (units == 0) {
            break;
        }
        if (typeface->unicharToGlyph(ch) == 0) {
            return false;
        }
        i += units;
    }
    return true;
}

inline sk_sp<SkFontMgr> CreateDefaultFontManager() {
    return SkFontMgr_New_DirectWrite();
}

inline SkFont CreateSkiaFont(float fontSize, SkTypeface* typeface = nullptr) {
    SkFont font;
    if (typeface) {
        font.setTypeface(sk_ref_sp(typeface));
    }
    font.setSize(fontSize);
    font.setEdging(SkFont::Edging::kSubpixelAntiAlias);
    font.setSubpixel(true);
    font.setBaselineSnap(true);
    font.setHinting(SkFontHinting::kSlight);
    return font;
}

inline float MeasureTextWidthWithFallback(SkFontMgr* fontMgr,
                                          SkTypeface* defaultTypeface,
                                          float fontSize,
                                          const wchar_t* text,
                                          uint32_t len) {
    if (!text || len == 0 || fontSize <= 0.0f) {
        return 0.0f;
    }

    SkFont defaultFont = CreateSkiaFont(fontSize, defaultTypeface);
    const std::string utf8 = WideToUtf8(text, len);
    if (utf8.empty()) {
        return 0.0f;
    }

    if (TypefaceSupportsText(defaultTypeface, text, len)) {
        return defaultFont.measureText(utf8.data(), utf8.size(), SkTextEncoding::kUTF8);
    }

    float width = 0.0f;
    uint32_t runStart = 0;
    sk_sp<SkTypeface> currentTypeface = nullptr;
    uint32_t i = 0;

    for (; i < len;) {
        uint32_t units = 0;
        const SkUnichar ch = DecodeUtf16At(text, len, i, &units);
        if (units == 0) {
            break;
        }

        sk_sp<SkTypeface> charTypeface = MatchTypefaceForCharacter(fontMgr, defaultTypeface, ch);

        if (i == 0) {
            currentTypeface = charTypeface;
        } else {
            bool sameTypeface = false;
            if (currentTypeface.get() == charTypeface.get() && currentTypeface.get() == defaultTypeface) {
                sameTypeface = true;
            }

            if (!sameTypeface) {
                SkFont font = CreateSkiaFont(fontSize, currentTypeface.get());
                const std::string runUtf8 = WideToUtf8(text + runStart, i - runStart);
                if (!runUtf8.empty()) {
                    width += font.measureText(runUtf8.data(), runUtf8.size(), SkTextEncoding::kUTF8);
                }
                runStart = i;
                currentTypeface = charTypeface;
            }
        }

        i += units;
    }

    if (runStart < i && currentTypeface) {
        SkFont font = CreateSkiaFont(fontSize, currentTypeface.get());
        const std::string runUtf8 = WideToUtf8(text + runStart, i - runStart);
        if (!runUtf8.empty()) {
            width += font.measureText(runUtf8.data(), runUtf8.size(), SkTextEncoding::kUTF8);
        }
    }

    return width;
}

} // namespace skia_font

#endif
