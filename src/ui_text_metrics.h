#pragma once

#include "graphics_types.h"
#include "text_measurer.h"

#include <string>

Size MeasureTextLayout(ITextMeasurer* measurer,
                       const std::wstring& text,
                       float fontSize,
                       float maxWidth,
                       TextWrapMode wrapMode = TextWrapMode::Wrap);
