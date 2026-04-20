#include "text_measurer.h"

#include "skia_text_layout.h"

#if defined(AI_WIN_UI_HAS_VCPKG_SKIA) || defined(AI_WIN_UI_HAS_LOCAL_SKIA)

#include <memory>

namespace {

class SkiaTextMeasurer final : public ITextMeasurer {
public:
    Size MeasureText(const wchar_t* text,
                     uint32_t len,
                     float fontSize,
                     float maxWidth,
                     TextWrapMode wrapMode) override {
        return MeasureSkiaTextLayout(text, len, fontSize, maxWidth, wrapMode);
    }
};

} // namespace

std::unique_ptr<ITextMeasurer> CreateSkiaTextMeasurer() {
    return std::make_unique<SkiaTextMeasurer>();
}

#else

std::unique_ptr<ITextMeasurer> CreateSkiaTextMeasurer() {
    return nullptr;
}

#endif
