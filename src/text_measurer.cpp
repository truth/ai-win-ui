#include "text_measurer.h"

std::unique_ptr<ITextMeasurer> CreateDirectWriteTextMeasurer();
std::unique_ptr<ITextMeasurer> CreateSkiaTextMeasurer();

std::unique_ptr<ITextMeasurer> CreateTextMeasurer(RendererBackend backend) {
    if (backend == RendererBackend::Skia) {
        if (auto skiaMeasurer = CreateSkiaTextMeasurer()) {
            return skiaMeasurer;
        }
    }

    return CreateDirectWriteTextMeasurer();
}
