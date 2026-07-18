#pragma once
// Component module - part of the ai-win-ui library.
// See ui.h for the full umbrella include.
#include "ui_panels.h"

class Spacer : public UIElement {
protected:
    float MeasurePreferredWidth(float /*availableWidth*/) const override {
        return 0.0f;
    }

    float MeasurePreferredHeight(float /*width*/) const override {
        return 0.0f;
    }

public:
    void Render(IRenderer& /*renderer*/) override {}
};

class Image : public UIElement {
public:
    enum class StretchMode {
        Fill,
        Uniform,
        UniformToFill
    };

    explicit Image(std::wstring source) : m_source(std::move(source)) {}

    float cornerRadius = 0.0f;
    StretchMode stretch = StretchMode::Uniform;

    void SetImageData(std::vector<uint8_t> imageData) {
        m_imageData = std::move(imageData);
    }

protected:
    float MeasurePreferredHeight(float width) const override {
        (void)width;
        return 120.0f;
    }

public:
    void Render(IRenderer& renderer) override {
        if (!m_bitmap && !m_imageData.empty()) {
            m_bitmap = renderer.CreateBitmapFromBytes(m_imageData.data(), m_imageData.size());
        }
        if (m_bitmap) {
            Rect drawRect = m_bounds;
            if (stretch != StretchMode::Fill) {
                const Size sourceSize = renderer.GetBitmapSize(m_bitmap);
                if (sourceSize.width > 0 && sourceSize.height > 0) {
                    const float targetWidth = m_bounds.right - m_bounds.left;
                    const float targetHeight = m_bounds.bottom - m_bounds.top;
                    const float sourceRatio = sourceSize.width / sourceSize.height;
                    const float targetRatio = targetWidth / targetHeight;

                    if (stretch == StretchMode::Uniform) {
                        if (sourceRatio > targetRatio) {
                            const float scaledHeight = targetWidth / sourceRatio;
                            const float offsetY = (targetHeight - scaledHeight) * 0.5f;
                            drawRect = Rect::Make(m_bounds.left, m_bounds.top + offsetY, m_bounds.right, m_bounds.top + offsetY + scaledHeight);
                        } else {
                            const float scaledWidth = targetHeight * sourceRatio;
                            const float offsetX = (targetWidth - scaledWidth) * 0.5f;
                            drawRect = Rect::Make(m_bounds.left + offsetX, m_bounds.top, m_bounds.left + offsetX + scaledWidth, m_bounds.bottom);
                        }
                    } else if (stretch == StretchMode::UniformToFill) {
                        if (sourceRatio > targetRatio) {
                            const float scaledWidth = targetHeight * sourceRatio;
                            const float offsetX = (targetWidth - scaledWidth) * 0.5f;
                            drawRect = Rect::Make(m_bounds.left + offsetX, m_bounds.top, m_bounds.left + offsetX + scaledWidth, m_bounds.bottom);
                        } else {
                            const float scaledHeight = targetWidth / sourceRatio;
                            const float offsetY = (targetHeight - scaledHeight) * 0.5f;
                            drawRect = Rect::Make(m_bounds.left, m_bounds.top + offsetY, m_bounds.right, m_bounds.top + offsetY + scaledHeight);
                        }
                    }
                }
            }

            if (cornerRadius > 0.0f) {
                renderer.PushRoundedClip(m_bounds, cornerRadius);
                renderer.DrawBitmap(m_bitmap, drawRect);
                renderer.PopLayer();
            } else {
                renderer.DrawBitmap(m_bitmap, drawRect);
            }
        } else {
            renderer.FillRect(m_bounds, ColorFromHex(0x404040));
        }
    }

private:
    std::wstring m_source;
    std::vector<uint8_t> m_imageData;
    BitmapHandle m_bitmap = nullptr;
};

class SvgIcon : public UIElement {
public:
    enum class StretchMode {
        Fill,
        Uniform,
        UniformToFill
    };

    explicit SvgIcon(std::wstring source) : m_source(std::move(source)) {}

    StretchMode stretch = StretchMode::Uniform;
    // Wave2 R8: alpha > 0 enables SrcIn recolor (typical monochrome icon tint).
    Color tint = Color{0.0f, 0.0f, 0.0f, 0.0f};

    void SetSourcePath(std::string path) { m_sourcePath = std::move(path); }
    void SetSvgData(std::vector<uint8_t> svgData) {
        m_svgData = std::move(svgData);
        m_svg.reset();
    }
    void SetTint(Color color) { tint = color; }
    bool HasTint() const { return tint.a > 0.001f; }

protected:
    float MeasurePreferredWidth(float availableWidth) const override {
        const float w = m_intrinsicSize.width > 0 ? m_intrinsicSize.width : 24.0f;
        return std::min(availableWidth > 0 ? availableWidth : w, w);
    }

    float MeasurePreferredHeight(float /*width*/) const override {
        return m_intrinsicSize.height > 0 ? m_intrinsicSize.height : 24.0f;
    }

public:
    void Render(IRenderer& renderer) override {
        if (!m_svg && !m_svgData.empty()) {
            const char* cacheKey = m_sourcePath.empty() ? nullptr : m_sourcePath.c_str();
            m_svg = renderer.CreateSvgFromBytes(m_svgData.data(), m_svgData.size(), cacheKey);
            if (m_svg) {
                m_intrinsicSize = renderer.GetSvgSize(m_svg);
            }
        }
        if (!m_svg) {
            renderer.FillRect(m_bounds, ColorFromHex(0x404040));
            return;
        }

        Rect drawRect = m_bounds;
        if (stretch != StretchMode::Fill && m_intrinsicSize.width > 0 && m_intrinsicSize.height > 0) {
            const float targetWidth = m_bounds.Width();
            const float targetHeight = m_bounds.Height();
            const float sourceRatio = m_intrinsicSize.width / m_intrinsicSize.height;
            const float targetRatio = targetWidth > 0 && targetHeight > 0
                ? targetWidth / targetHeight : 1.0f;
            const bool letterbox = stretch == StretchMode::Uniform
                ? sourceRatio > targetRatio
                : sourceRatio < targetRatio;
            if (letterbox) {
                const float scaledHeight = targetWidth / sourceRatio;
                const float offsetY = (targetHeight - scaledHeight) * 0.5f;
                drawRect = Rect::Make(m_bounds.left, m_bounds.top + offsetY,
                                      m_bounds.right, m_bounds.top + offsetY + scaledHeight);
            } else {
                const float scaledWidth = targetHeight * sourceRatio;
                const float offsetX = (targetWidth - scaledWidth) * 0.5f;
                drawRect = Rect::Make(m_bounds.left + offsetX, m_bounds.top,
                                      m_bounds.left + offsetX + scaledWidth, m_bounds.bottom);
            }
        }
        if (HasTint()) {
            renderer.DrawSvg(m_svg, drawRect, true, tint);
        } else {
            renderer.DrawSvg(m_svg, drawRect);
        }
    }

private:
    std::wstring m_source;
    std::string m_sourcePath;
    std::vector<uint8_t> m_svgData;
    SvgHandle m_svg = nullptr;
    Size m_intrinsicSize{24.0f, 24.0f};
};

