#pragma once

#include "renderer.h"
#include <wrl/client.h>

#include <algorithm>
#include <functional>
#include <memory>
#include <string>
#include <vector>

struct Thickness {
    float left = 0;
    float top = 0;
    float right = 0;
    float bottom = 0;
};

class UIElement {
public:
    virtual ~UIElement() = default;

    void SetBounds(const D2D1_RECT_F& rect) { m_bounds = rect; }
    D2D1_RECT_F Bounds() const { return m_bounds; }
    void SetMargin(const Thickness& margin) { m_margin = margin; }
    const Thickness& Margin() const { return m_margin; }
    void SetFixedWidth(float width) { m_fixedWidth = width; }
    void SetFixedHeight(float height) { m_fixedHeight = height; }
    bool HasFixedWidth() const { return m_fixedWidth >= 0.0f; }
    bool HasFixedHeight() const { return m_fixedHeight >= 0.0f; }

    float GetPreferredWidth(float availableWidth) const {
        const float clampedWidth = std::max(0.0f, availableWidth);
        const float preferredWidth = HasFixedWidth() ? m_fixedWidth : MeasurePreferredWidth(clampedWidth);
        return std::clamp(preferredWidth, 0.0f, clampedWidth);
    }

    float GetPreferredHeight(float width) const {
        const float clampedWidth = std::max(0.0f, width);
        if (HasFixedHeight()) {
            return std::max(0.0f, m_fixedHeight);
        }
        return std::max(0.0f, MeasurePreferredHeight(clampedWidth));
    }

    virtual void Measure(float availableWidth, float availableHeight) {
        (void)availableHeight;
        m_desiredSize.width = GetPreferredWidth(availableWidth);
        m_desiredSize.height = GetPreferredHeight(m_desiredSize.width);
    }
    virtual void Arrange(const D2D1_RECT_F& finalRect) { m_bounds = finalRect; }
    virtual void Render(Renderer& renderer) {
        for (auto& child : m_children) {
            child->Render(renderer);
        }
    }

    virtual bool OnMouseMove(float x, float y) {
        bool changed = false;
        bool hitChild = false;
        for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
            if (!hitChild && (*it)->HitTest(x, y)) {
                changed = (*it)->OnMouseMove(x, y) || changed;
                hitChild = true;
            } else {
                changed = (*it)->OnMouseLeave() || changed;
            }
        }
        return changed;
    }

    virtual bool OnMouseDown(float x, float y) {
        for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
            if ((*it)->HitTest(x, y) && (*it)->OnMouseDown(x, y)) {
                return true;
            }
        }
        return false;
    }

    virtual bool OnMouseUp(float x, float y) {
        bool changed = false;
        for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
            changed = (*it)->OnMouseUp(x, y) || changed;
        }
        return changed;
    }

    virtual bool OnMouseLeave() {
        bool changed = false;
        for (auto& child : m_children) {
            changed = child->OnMouseLeave() || changed;
        }
        return changed;
    }

protected:
    virtual float MeasurePreferredWidth(float availableWidth) const {
        return availableWidth;
    }

    virtual float MeasurePreferredHeight(float width) const {
        return 36.0f;
    }

public:
    bool HitTest(float x, float y) const {
        return x >= m_bounds.left && x <= m_bounds.right && y >= m_bounds.top && y <= m_bounds.bottom;
    }

    void AddChild(std::unique_ptr<UIElement> child) { m_children.emplace_back(std::move(child)); }
    std::vector<std::unique_ptr<UIElement>>& Children() { return m_children; }

protected:
    D2D1_RECT_F m_bounds = D2D1::RectF();
    D2D1_SIZE_F m_desiredSize = D2D1::SizeF();
    std::vector<std::unique_ptr<UIElement>> m_children;
    Thickness m_margin{};
    float m_fixedWidth = -1.0f;
    float m_fixedHeight = -1.0f;
};

class Panel : public UIElement {
public:
    enum class AlignItems {
        Stretch,
        Start,
        Center,
        End
    };

    enum class JustifyContent {
        Start,
        Center,
        End,
        SpaceBetween
    };

    Thickness padding{8, 8, 8, 8};
    float spacing = 6.0f;
    float cornerRadius = 0.0f;
    D2D1_COLOR_F background = D2D1::ColorF(0x1E1E1E);
    AlignItems alignItems = AlignItems::Stretch;
    JustifyContent justifyContent = JustifyContent::Start;

    void Arrange(const D2D1_RECT_F& finalRect) override {
        UIElement::Arrange(finalRect);

        const float contentLeft = m_bounds.left + padding.left;
        const float contentTop = m_bounds.top + padding.top;
        const float contentWidth = std::max(0.0f, (m_bounds.right - m_bounds.left) - padding.left - padding.right);
        const float contentHeight = std::max(0.0f, (m_bounds.bottom - m_bounds.top) - padding.top - padding.bottom);

        struct ChildLayout {
            UIElement* element;
            Thickness margin;
            float width;
            float height;
            float totalHeight;
        };

        std::vector<ChildLayout> layouts;
        layouts.reserve(m_children.size());

        float totalHeight = 0.0f;
        float totalChildHeight = 0.0f;
        bool first = true;
        for (auto& child : m_children) {
            const Thickness margin = child->Margin();
            const float availableChildWidth = std::max(0.0f, contentWidth - margin.left - margin.right);
            const float childWidth = (alignItems == AlignItems::Stretch && !child->HasFixedWidth())
                ? availableChildWidth
                : child->GetPreferredWidth(availableChildWidth);
            const float childHeight = child->GetPreferredHeight(childWidth);
            const float blockHeight = margin.top + childHeight + margin.bottom;

            if (!first) {
                totalHeight += spacing;
            }
            totalHeight += blockHeight;
            totalChildHeight += blockHeight;
            first = false;

            layouts.push_back(ChildLayout{
                child.get(),
                margin,
                childWidth,
                childHeight,
                blockHeight
            });
        }

        float y = contentTop;
        float spacingBetweenChildren = layouts.size() > 1 ? spacing : 0.0f;

        switch (justifyContent) {
            case JustifyContent::Center:
                y += std::max(0.0f, (contentHeight - totalHeight) * 0.5f);
                break;
            case JustifyContent::End:
                y += std::max(0.0f, contentHeight - totalHeight);
                break;
            case JustifyContent::SpaceBetween:
                if (layouts.size() > 1) {
                    const float distributedSpacing = (contentHeight - totalChildHeight) / static_cast<float>(layouts.size() - 1);
                    spacingBetweenChildren = std::max(spacing, distributedSpacing);
                }
                break;
            case JustifyContent::Start:
            default:
                break;
        }

        for (size_t i = 0; i < layouts.size(); ++i) {
            const auto& layout = layouts[i];
            const float availableChildWidth = std::max(0.0f, contentWidth - layout.margin.left - layout.margin.right);

            float x = contentLeft + layout.margin.left;
            if (alignItems == AlignItems::Center) {
                x += std::max(0.0f, (availableChildWidth - layout.width) * 0.5f);
            } else if (alignItems == AlignItems::End) {
                x += std::max(0.0f, availableChildWidth - layout.width);
            }

            y += layout.margin.top;
            layout.element->Arrange(D2D1::RectF(x, y, x + layout.width, y + layout.height));
            y += layout.height + layout.margin.bottom;
            if (i + 1 < layouts.size()) {
                y += spacingBetweenChildren;
            }
        }
    }

protected:
    float MeasurePreferredHeight(float width) const override {
        float totalHeight = padding.top + padding.bottom;
        const float contentWidth = std::max(0.0f, width - padding.left - padding.right);
        bool first = true;
        for (const auto& child : m_children) {
            if (!first) {
                totalHeight += spacing;
            }
            const Thickness& margin = child->Margin();
            const float availableChildWidth = std::max(0.0f, contentWidth - margin.left - margin.right);
            const float childWidth = (alignItems == AlignItems::Stretch && !child->HasFixedWidth())
                ? availableChildWidth
                : child->GetPreferredWidth(availableChildWidth);
            totalHeight += margin.top + child->GetPreferredHeight(childWidth) + margin.bottom;
            first = false;
        }
        return totalHeight;
    }

public:
    void Render(Renderer& renderer) override {
        if (cornerRadius > 0.0f) {
            renderer.FillRoundedRect(m_bounds, background, cornerRadius);
            renderer.DrawRoundedRect(m_bounds, D2D1::ColorF(0x333333), 1.0f, cornerRadius);
        } else {
            renderer.FillRect(m_bounds, background);
        }
        for (auto& child : m_children) {
            child->Render(renderer);
        }
    }
};

class GridPanel : public UIElement {
public:
    int columns = 3;
    float cellSpacing = 8.0f;
    float cornerRadius = 0.0f;
    Thickness padding{8, 8, 8, 8};
    D2D1_COLOR_F background = D2D1::ColorF(0x1A1A1A);
    float rowHeight = 120.0f;

    void Arrange(const D2D1_RECT_F& finalRect) override {
        UIElement::Arrange(finalRect);

        const float x0 = m_bounds.left + padding.left;
        float y = m_bounds.top + padding.top;
        const float contentWidth = (m_bounds.right - m_bounds.left) - padding.left - padding.right;
        const int cols = std::max(1, columns);
        const float cellWidth = (contentWidth - cellSpacing * (cols - 1)) / cols;

        float x = x0;
        int col = 0;
        for (auto& child : m_children) {
            const Thickness& margin = child->Margin();
            const float availableCellWidth = std::max(0.0f, cellWidth - margin.left - margin.right);
            const float childWidth = child->HasFixedWidth() ? child->GetPreferredWidth(availableCellWidth) : availableCellWidth;
            const float childHeight = std::min(
                std::max(0.0f, rowHeight - margin.top - margin.bottom),
                child->GetPreferredHeight(childWidth));
            child->Arrange(D2D1::RectF(
                x + margin.left,
                y + margin.top,
                x + margin.left + childWidth,
                y + margin.top + childHeight));
            col++;
            if (col >= cols) {
                col = 0;
                x = x0;
                y += rowHeight + cellSpacing;
            } else {
                x += cellWidth + cellSpacing;
            }
        }
    }

protected:
    float MeasurePreferredHeight(float width) const override {
        (void)width;
        const int cols = std::max(1, columns);
        const int cellCount = static_cast<int>(m_children.size());
        const int rows = (cellCount + cols - 1) / cols;
        float totalHeight = padding.top + padding.bottom;
        if (rows > 0) {
            totalHeight += rows * rowHeight;
            totalHeight += (rows - 1) * cellSpacing;
        }
        return totalHeight;
    }

public:
    void Render(Renderer& renderer) override {
        if (cornerRadius > 0.0f) {
            renderer.FillRoundedRect(m_bounds, background, cornerRadius);
            renderer.DrawRoundedRect(m_bounds, D2D1::ColorF(0x333333), 1.0f, cornerRadius);
        } else {
            renderer.FillRect(m_bounds, background);
        }
        for (auto& child : m_children) {
            child->Render(renderer);
        }
    }
};

class Label : public UIElement {
public:
    explicit Label(std::wstring text) : m_text(std::move(text)) {}

    void SetText(std::wstring text) { m_text = std::move(text); }

protected:
    float MeasurePreferredWidth(float availableWidth) const override {
        const float estimatedWidth = static_cast<float>(m_text.size()) * m_fontSize * 0.56f + 4.0f;
        return std::min(availableWidth, estimatedWidth);
    }

    float MeasurePreferredHeight(float width) const override {
        (void)width;
        return m_fontSize + 12.0f;
    }

public:
    void Render(Renderer& renderer) override {
        renderer.DrawTextW(
            m_text.c_str(),
            static_cast<UINT32>(m_text.size()),
            m_bounds,
            m_color,
            m_fontSize
        );
    }

    float m_fontSize = 15.0f;
    D2D1_COLOR_F m_color = D2D1::ColorF(0xEDEDED);

private:
    std::wstring m_text;
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
    void Render(Renderer& renderer) override {
        if (!m_bitmap && !m_imageData.empty()) {
            m_bitmap = renderer.CreateBitmapFromBytes(m_imageData.data(), m_imageData.size());
        }
        if (m_bitmap) {
            D2D1_RECT_F drawRect = m_bounds;
            if (stretch != StretchMode::Fill) {
                const D2D1_SIZE_F sourceSize = m_bitmap->GetSize();
                if (sourceSize.width > 0 && sourceSize.height > 0) {
                    const float targetWidth = m_bounds.right - m_bounds.left;
                    const float targetHeight = m_bounds.bottom - m_bounds.top;
                    const float sourceRatio = sourceSize.width / sourceSize.height;
                    const float targetRatio = targetWidth / targetHeight;

                    if (stretch == StretchMode::Uniform) {
                        if (sourceRatio > targetRatio) {
                            const float scaledHeight = targetWidth / sourceRatio;
                            const float offsetY = (targetHeight - scaledHeight) * 0.5f;
                            drawRect = D2D1::RectF(m_bounds.left, m_bounds.top + offsetY, m_bounds.right, m_bounds.top + offsetY + scaledHeight);
                        } else {
                            const float scaledWidth = targetHeight * sourceRatio;
                            const float offsetX = (targetWidth - scaledWidth) * 0.5f;
                            drawRect = D2D1::RectF(m_bounds.left + offsetX, m_bounds.top, m_bounds.left + offsetX + scaledWidth, m_bounds.bottom);
                        }
                    } else if (stretch == StretchMode::UniformToFill) {
                        if (sourceRatio > targetRatio) {
                            const float scaledWidth = targetHeight * sourceRatio;
                            const float offsetX = (targetWidth - scaledWidth) * 0.5f;
                            drawRect = D2D1::RectF(m_bounds.left + offsetX, m_bounds.top, m_bounds.left + offsetX + scaledWidth, m_bounds.bottom);
                        } else {
                            const float scaledHeight = targetWidth / sourceRatio;
                            const float offsetY = (targetHeight - scaledHeight) * 0.5f;
                            drawRect = D2D1::RectF(m_bounds.left, m_bounds.top + offsetY, m_bounds.right, m_bounds.top + offsetY + scaledHeight);
                        }
                    }
                }
            }

            if (cornerRadius > 0.0f) {
                renderer.PushRoundedClip(m_bounds, cornerRadius);
                renderer.DrawBitmap(m_bitmap.Get(), drawRect);
                renderer.PopLayer();
            } else {
                renderer.DrawBitmap(m_bitmap.Get(), drawRect);
            }
        } else {
            renderer.FillRect(m_bounds, D2D1::ColorF(0x404040));
        }
    }

private:
    std::wstring m_source;
    std::vector<uint8_t> m_imageData;
    Microsoft::WRL::ComPtr<ID2D1Bitmap> m_bitmap;
};

class Button : public UIElement {
public:
    explicit Button(std::wstring text) : m_text(std::move(text)) {}

    void SetOnClick(std::function<void()> onClick) { m_onClick = std::move(onClick); }
    void SetText(std::wstring text) { m_text = std::move(text); }

    float cornerRadius = 4.0f;
    float fontSize = 14.0f;
    D2D1_COLOR_F background = D2D1::ColorF(0x2D2D30);
    D2D1_COLOR_F foreground = D2D1::ColorF(0xFFFFFF);

protected:
    float MeasurePreferredWidth(float availableWidth) const override {
        const float estimatedWidth = static_cast<float>(m_text.size()) * fontSize * 0.62f + 28.0f;
        return std::min(availableWidth, estimatedWidth);
    }

    float MeasurePreferredHeight(float width) const override {
        (void)width;
        return fontSize + 18.0f;
    }

public:
    void Render(Renderer& renderer) override {
        D2D1_COLOR_F bg = background;
        if (m_pressed && m_hovered) {
            bg = D2D1::ColorF(0x0E639C);
        } else if (m_hovered) {
            bg = D2D1::ColorF(0x3E3E42);
        }

        if (cornerRadius > 0.0f) {
            renderer.FillRoundedRect(m_bounds, bg, cornerRadius);
            renderer.DrawRoundedRect(m_bounds, D2D1::ColorF(0x6A6A6A), 1.0f, cornerRadius);
        } else {
            renderer.FillRect(m_bounds, bg);
            renderer.DrawRect(m_bounds, D2D1::ColorF(0x6A6A6A), 1.0f);
        }

        D2D1_RECT_F textRect = m_bounds;
        textRect.left += 10.0f;
        renderer.DrawTextW(
            m_text.c_str(),
            static_cast<UINT32>(m_text.size()),
            textRect,
            foreground,
            fontSize
        );
    }

    bool OnMouseMove(float x, float y) override {
        const bool nextHovered = HitTest(x, y);
        const bool changed = m_hovered != nextHovered;
        m_hovered = nextHovered;
        return changed;
    }

    bool OnMouseDown(float x, float y) override {
        const bool nextPressed = HitTest(x, y);
        const bool changed = m_pressed != nextPressed || m_hovered != nextPressed;
        m_pressed = nextPressed;
        m_hovered = nextPressed;
        return changed;
    }

    bool OnMouseUp(float x, float y) override {
        const bool wasPressed = m_pressed;
        const bool nextHovered = HitTest(x, y);
        const bool changed = wasPressed || (m_hovered != nextHovered);
        m_pressed = false;
        m_hovered = nextHovered;
        if (wasPressed && nextHovered && m_onClick) {
            m_onClick();
            return true;
        }
        return changed;
    }

    bool OnMouseLeave() override {
        if (!m_hovered) {
            return false;
        }
        m_hovered = false;
        return true;
    }

private:
    std::wstring m_text;
    bool m_hovered = false;
    bool m_pressed = false;
    std::function<void()> m_onClick;
};
