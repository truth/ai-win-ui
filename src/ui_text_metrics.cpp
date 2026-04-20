#include "ui_text_metrics.h"

#include <algorithm>

Size MeasureTextLayout(ITextMeasurer* measurer,
                       const std::wstring& text,
                       float fontSize,
                       float maxWidth,
                       TextWrapMode wrapMode) {
    if (!measurer || text.empty() || fontSize <= 0.0f) {
        return Size{0.0f, 0.0f};
    }

    return measurer->MeasureText(
        text.c_str(),
        static_cast<uint32_t>(text.size()),
        fontSize,
        std::max(1.0f, maxWidth),
        wrapMode);
}
