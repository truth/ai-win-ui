#pragma once

#include "graphics_types.h"
#include "renderer.h"

#include <cstdint>
#include <string>
#include <vector>

// Shared Skia text layout rules (measure AND draw must call CreateSkiaTextLayout):
//
// 1. Explicit newlines: '\n' and '\r\n' split hard lines first.
// 2. TextWrapMode::NoWrap: only the first hard line is kept (rest ignored for metrics/draw).
// 3. TextWrapMode::Wrap: soft-wrap each hard line at maxWidth; prefer last whitespace,
//    else break mid-word; single glyph wider than maxWidth still emits one character.
// 4. Leading whitespace on a soft-wrapped segment is skipped; trailing whitespace trimmed.
// 5. Vertical metrics: ascent/descent from SkFontMetrics; lineHeight = max(spacing, a+d);
//    blockHeight = (lineCount-1)*lineHeight + (ascent+descent)  // matches single-line DWrite height style
// 6. bold/italic select the same default family chain as drawing (Segoe UI → Arial → …).
// 7. Empty / null text → empty layout, zero size (not a failure).
// 8. ellipsis=true + NoWrap: if first line width > maxWidth, binary-search prefix + U+2026
//    so measured width fits maxWidth (Wave2 R6).

struct SkiaTextLayoutLine {
    std::wstring text;
    float width = 0.0f;
};

struct SkiaTextLayout {
    std::vector<SkiaTextLayoutLine> lines;
    float ascent = 0.0f;
    float descent = 0.0f;
    float lineHeight = 0.0f;
    float textHeight = 0.0f;
    float blockHeight = 0.0f;
    float maxLineWidth = 0.0f;
};

bool CreateSkiaTextLayout(const wchar_t* text,
                          uint32_t len,
                          float fontSize,
                          float maxWidth,
                          TextWrapMode wrapMode,
                          SkiaTextLayout* layout,
                          bool bold = false,
                          bool italic = false,
                          bool ellipsis = false);

Size MeasureSkiaTextLayout(const wchar_t* text,
                           uint32_t len,
                           float fontSize,
                           float maxWidth,
                           TextWrapMode wrapMode,
                           bool bold = false,
                           bool italic = false,
                           bool ellipsis = false);
