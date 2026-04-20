#pragma once

#include "graphics_types.h"
#include "renderer.h"

#include <cstdint>
#include <string>
#include <vector>

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
                          SkiaTextLayout* layout);

Size MeasureSkiaTextLayout(const wchar_t* text,
                           uint32_t len,
                           float fontSize,
                           float maxWidth,
                           TextWrapMode wrapMode);
