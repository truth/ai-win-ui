#pragma once

#include "renderer.h"

#include <algorithm>
#include <cstdint>
#include <string>

#if defined(AI_WIN_UI_HAS_VCPKG_SKIA) || defined(AI_WIN_UI_HAS_LOCAL_SKIA)
#include "include/core/SkFont.h"
#include "include/core/SkFontMgr.h"
#include "include/core/SkFontTypes.h"
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

} // namespace skia_font

#endif