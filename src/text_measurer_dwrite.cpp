#include "text_measurer.h"

#include <dwrite.h>
#include <wrl/client.h>

#include <algorithm>
#include <memory>

namespace {

using Microsoft::WRL::ComPtr;

class DirectWriteTextMeasurer final : public ITextMeasurer {
public:
    Size MeasureText(const wchar_t* text, uint32_t len, float fontSize, float maxWidth) override {
        if (!text || len == 0 || fontSize <= 0.0f) {
            return Size{};
        }

        if (!EnsureFactory()) {
            return Size{};
        }

        ComPtr<IDWriteTextFormat> textFormat;
        if (FAILED(m_dwriteFactory->CreateTextFormat(
                L"Segoe UI",
                nullptr,
                DWRITE_FONT_WEIGHT_NORMAL,
                DWRITE_FONT_STYLE_NORMAL,
                DWRITE_FONT_STRETCH_NORMAL,
                fontSize,
                L"",
                textFormat.ReleaseAndGetAddressOf()))) {
            return Size{};
        }

        textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
        textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);

        ComPtr<IDWriteTextLayout> textLayout;
        if (FAILED(m_dwriteFactory->CreateTextLayout(
                text,
                static_cast<UINT32>(len),
                textFormat.Get(),
                std::max(1.0f, maxWidth),
                4096.0f,
                textLayout.ReleaseAndGetAddressOf()))) {
            return Size{};
        }

        DWRITE_TEXT_METRICS metrics{};
        if (FAILED(textLayout->GetMetrics(&metrics))) {
            return Size{};
        }

        return Size{metrics.widthIncludingTrailingWhitespace, metrics.height};
    }

private:
    bool EnsureFactory() {
        if (m_dwriteFactory) {
            return true;
        }

        return SUCCEEDED(DWriteCreateFactory(
            DWRITE_FACTORY_TYPE_SHARED,
            __uuidof(IDWriteFactory),
            reinterpret_cast<IUnknown**>(m_dwriteFactory.ReleaseAndGetAddressOf())));
    }

    ComPtr<IDWriteFactory> m_dwriteFactory;
};

} // namespace

std::unique_ptr<ITextMeasurer> CreateTextMeasurer() {
    return std::make_unique<DirectWriteTextMeasurer>();
}
