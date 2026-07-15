#pragma once

#include "renderer.h"

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>

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

inline SkFontStyle MakeFontStyle(bool bold = false, bool italic = false) {
    return SkFontStyle(
        bold ? SkFontStyle::kBold_Weight : SkFontStyle::kNormal_Weight,
        SkFontStyle::kNormal_Width,
        italic ? SkFontStyle::kItalic_Slant : SkFontStyle::kUpright_Slant);
}

inline sk_sp<SkTypeface> TryMatchFamily(SkFontMgr* fontMgr, const char* familyName, bool bold = false, bool italic = false) {
    if (!fontMgr || !familyName || !familyName[0]) {
        return nullptr;
    }
    return fontMgr->matchFamilyStyle(familyName, MakeFontStyle(bold, italic));
}

inline sk_sp<SkTypeface> CreateDefaultTypeface(SkFontMgr* fontMgr, bool bold = false, bool italic = false) {
    if (!fontMgr) {
        return nullptr;
    }
    if (sk_sp<SkTypeface> typeface = TryMatchFamily(fontMgr, "Segoe UI", bold, italic)) {
        return typeface;
    }
    if (sk_sp<SkTypeface> typeface = TryMatchFamily(fontMgr, "Arial", bold, italic)) {
        return typeface;
    }
    if (sk_sp<SkTypeface> typeface = TryMatchFamily(fontMgr, "Tahoma", bold, italic)) {
        return typeface;
    }
    return fontMgr->legacyMakeTypeface(nullptr, MakeFontStyle(bold, italic));
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

    static std::unordered_map<SkUnichar, sk_sp<SkTypeface>> s_fallbackCache;
    auto it = s_fallbackCache.find(character);
    if (it != s_fallbackCache.end()) {
        return it->second;
    }

    const char* bcp47[] = {"zh-Hans", "zh-Hant", "zh-CN", "zh-TW", "ja", "ko"};
    sk_sp<SkTypeface> typeface = fontMgr->matchFamilyStyleCharacter(
            nullptr,
            SkFontStyle(),
            bcp47,
            6,
            character);

    if (!typeface) {
        typeface = defaultTypeface ? sk_ref_sp(defaultTypeface) : fontMgr->legacyMakeTypeface(nullptr, SkFontStyle());
    }

    s_fallbackCache[character] = typeface;
    return typeface;
}

inline bool TypefaceSupportsText(SkTypeface* typeface, const wchar_t* text, uint32_t len) {
    if (!typeface || !text || len == 0) {
        return false;
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

template <size_t N = 256>
class StackUtf8Converter {
public:
    StackUtf8Converter(const wchar_t* text, uint32_t len) {
        if (!text || len == 0) {
            m_ptr = "";
            m_size = 0;
            return;
        }

        int requiredSize = WideCharToMultiByte(CP_UTF8, 0, text, static_cast<int>(len), nullptr, 0, nullptr, nullptr);
        if (requiredSize <= 0) {
            m_ptr = "";
            m_size = 0;
            return;
        }

        m_size = static_cast<size_t>(requiredSize);
        if (m_size < N) {
            WideCharToMultiByte(CP_UTF8, 0, text, static_cast<int>(len), m_stackBuf, requiredSize, nullptr, nullptr);
            m_stackBuf[m_size] = '\0';
            m_ptr = m_stackBuf;
        } else {
            m_heapBuf.resize(m_size + 1);
            WideCharToMultiByte(CP_UTF8, 0, text, static_cast<int>(len), m_heapBuf.data(), requiredSize, nullptr, nullptr);
            m_heapBuf[m_size] = '\0';
            m_ptr = m_heapBuf.data();
        }
    }

    const char* c_str() const { return m_ptr; }
    size_t size() const { return m_size; }
    bool empty() const { return m_size == 0; }

private:
    char m_stackBuf[N];
    std::vector<char> m_heapBuf;
    const char* m_ptr = nullptr;
    size_t m_size = 0;
};

template <typename Callback>
inline void IterateTextRuns(SkFontMgr* fontMgr,
                            SkTypeface* defaultTypeface,
                            const wchar_t* text,
                            uint32_t len,
                            Callback&& callback) {
    if (!text || len == 0) {
        return;
    }

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
            const bool sameTypeface = (currentTypeface.get() == charTypeface.get());

            if (!sameTypeface) {
                if (currentTypeface) {
                    callback(text + runStart, i - runStart, currentTypeface.get());
                }
                runStart = i;
                currentTypeface = charTypeface;
            }
        }

        i += units;
    }

    if (runStart < i && currentTypeface) {
        callback(text + runStart, i - runStart, currentTypeface.get());
    }
}

inline float MeasureTextWidthWithFallback(SkFontMgr* fontMgr,
                                          SkTypeface* defaultTypeface,
                                          float fontSize,
                                          const wchar_t* text,
                                          uint32_t len) {
    if (!text || len == 0 || fontSize <= 0.0f) {
        return 0.0f;
    }

    if (TypefaceSupportsText(defaultTypeface, text, len)) {
        SkFont font = CreateSkiaFont(fontSize, defaultTypeface);
        StackUtf8Converter<> utf8(text, len);
        if (!utf8.empty()) {
            return font.measureText(utf8.c_str(), utf8.size(), SkTextEncoding::kUTF8);
        }
        return 0.0f;
    }

    float width = 0.0f;
    IterateTextRuns(fontMgr, defaultTypeface, text, len, [&](const wchar_t* runText, uint32_t runLen, SkTypeface* typeface) {
        SkFont font = CreateSkiaFont(fontSize, typeface);
        StackUtf8Converter<> runUtf8(runText, runLen);
        if (!runUtf8.empty()) {
            width += font.measureText(runUtf8.c_str(), runUtf8.size(), SkTextEncoding::kUTF8);
        }
    });

    return width;
}

} // namespace skia_font

#endif
