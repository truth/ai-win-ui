#pragma once

#include "graphics_types.h"

#include <cstdint>
#include <memory>

class ITextMeasurer {
public:
    virtual ~ITextMeasurer() = default;
    virtual Size MeasureText(const wchar_t* text, uint32_t len, float fontSize, float maxWidth) = 0;
};

std::unique_ptr<ITextMeasurer> CreateTextMeasurer();
