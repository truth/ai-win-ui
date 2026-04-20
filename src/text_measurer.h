#pragma once

#include "graphics_types.h"
#include "renderer.h"

#include <cstdint>
#include <memory>

class ITextMeasurer {
public:
    virtual ~ITextMeasurer() = default;
    virtual Size MeasureText(const wchar_t* text,
                             uint32_t len,
                             float fontSize,
                             float maxWidth,
                             TextWrapMode wrapMode = TextWrapMode::Wrap) = 0;
};

std::unique_ptr<ITextMeasurer> CreateTextMeasurer(RendererBackend backend = RendererBackend::Direct2D);
