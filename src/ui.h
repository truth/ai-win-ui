#pragma once

#include "graphics_types.h"
#include "layout_engine.h"
#include "renderer.h"
#include "ui_context.h"
#include "ui_text_metrics.h"

#include <algorithm>
#include <functional>
#include <memory>
#include <map>
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

    void SetBounds(const Rect& rect) { m_bounds = rect; }
    Rect Bounds() const { return m_bounds; }
    Size DesiredSize() const { return m_desiredSize; }
    void SetMargin(const Thickness& margin) { m_margin = margin; }
    const Thickness& Margin() const { return m_margin; }
    void SetFixedWidth(float width) { m_fixedWidth = width; }
    void SetFixedHeight(float height) { m_fixedHeight = height; }
    void SetFlexGrow(float flexGrow) { m_flexGrow = std::max(0.0f, flexGrow); }
    void SetFlexShrink(float flexShrink) { m_flexShrink = std::max(0.0f, flexShrink); }
    void SetFlexBasis(float flexBasis) { m_flexBasis = std::max(0.0f, flexBasis); }
    bool HasFixedWidth() const { return m_fixedWidth >= 0.0f; }
    bool HasFixedHeight() const { return m_fixedHeight >= 0.0f; }
    float FlexGrow() const { return m_flexGrow; }
    float FlexShrink() const { return m_flexShrink; }
    bool HasFlexBasis() const { return m_flexBasis >= 0.0f; }
    float FlexBasis() const { return m_flexBasis; }
    void SetContext(UIContext* context) {
        m_context = context;
        for (auto& child : m_children) {
            child->SetContext(context);
        }
    }
    UIContext* Context() const { return m_context; }

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
        const float width = GetPreferredWidth(availableWidth);
        m_desiredSize.width = width;
        m_desiredSize.height = GetPreferredHeight(width);
    }
    virtual void Arrange(const Rect& finalRect) { m_bounds = finalRect; }
    virtual void Render(IRenderer& renderer) {
        for (auto& child : m_children) {
            child->Render(renderer);
        }
    }

    virtual bool IsFocusable() const { return false; }
    bool HasFocus() const { return m_hasFocus; }

    virtual bool OnFocus() {
        if (!m_hasFocus) {
            m_hasFocus = true;
            return true;
        }
        return false;
    }

    virtual bool OnBlur() {
        if (m_hasFocus) {
            m_hasFocus = false;
            return true;
        }
        return false;
    }

    virtual bool OnKeyDown(WPARAM /*keyCode*/, LPARAM /*lParam*/) {
        return false;
    }

    virtual bool OnKeyUp(WPARAM /*keyCode*/, LPARAM /*lParam*/) {
        return false;
    }

    virtual bool OnChar(wchar_t /*ch*/) {
        return false;
    }

    virtual bool OnTimer(UINT_PTR /*timerId*/) {
        return false;
    }

    virtual UIElement* FindFocusableAt(float x, float y) {
        for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
            if ((*it)->HitTest(x, y)) {
                if (auto* focusable = (*it)->FindFocusableAt(x, y)) {
                    return focusable;
                }
            }
        }
        return IsFocusable() && HitTest(x, y) ? const_cast<UIElement*>(this) : nullptr;
    }

    virtual UIElement* FindHitElementAt(float x, float y) {
        for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
            if ((*it)->HitTest(x, y)) {
                if (auto* hitChild = (*it)->FindHitElementAt(x, y)) {
                    return hitChild;
                }
                return it->get();
            }
        }
        return HitTest(x, y) ? const_cast<UIElement*>(this) : nullptr;
    }

    virtual void CollectFocusable(std::vector<UIElement*>& out) {
        if (IsFocusable()) {
            out.push_back(this);
        }
        for (auto& child : m_children) {
            child->CollectFocusable(out);
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

    void AddChild(std::unique_ptr<UIElement> child) {
        if (child) {
            child->SetContext(m_context);
        }
        m_children.emplace_back(std::move(child));
    }
    std::vector<std::unique_ptr<UIElement>>& Children() { return m_children; }

protected:
    Size MeasureTextValue(const std::wstring& text, float fontSize, float maxWidth) const {
        return MeasureTextLayout(m_context ? m_context->textMeasurer : nullptr, text, fontSize, maxWidth);
    }

    Rect m_bounds{};
    Size m_desiredSize{};
    std::vector<std::unique_ptr<UIElement>> m_children;
    Thickness m_margin{};
    float m_fixedWidth = -1.0f;
    float m_fixedHeight = -1.0f;
    float m_flexGrow = 0.0f;
    float m_flexShrink = 0.0f;
    float m_flexBasis = -1.0f;
    bool m_hasFocus = false;
    UIContext* m_context = nullptr;
};

class Panel : public UIElement {
public:
    enum class Direction {
        Column,
        Row
    };

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
    Color background = ColorFromHex(0x1E1E1E);
    Direction direction = Direction::Column;
    AlignItems alignItems = AlignItems::Stretch;
    JustifyContent justifyContent = JustifyContent::Start;

    void Arrange(const Rect& finalRect) override {
        UIElement::Arrange(finalRect);

        if (m_context && m_context->layoutEngine) {
            m_context->layoutEngine->ArrangeStack(ToStackLayoutStyle(), BuildLayoutChildren(), m_bounds);
            return;
        }

        if (direction == Direction::Row) {
            ArrangeRowFallback();
            return;
        }

        const float contentLeft = m_bounds.left + padding.left;
        const float contentTop = m_bounds.top + padding.top;
        const float contentWidth = std::max(0.0f, m_bounds.Width() - padding.left - padding.right);
        const float contentHeight = std::max(0.0f, m_bounds.Height() - padding.top - padding.bottom);

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
            child->Measure(childWidth, contentHeight);
            const float childHeight = child->DesiredSize().height;
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
            layout.element->Arrange(Rect::Make(x, y, x + layout.width, y + layout.height));
            y += layout.height + layout.margin.bottom;
            if (i + 1 < layouts.size()) {
                y += spacingBetweenChildren;
            }
        }
    }

    void Measure(float availableWidth, float availableHeight) override {
        if (m_context && m_context->layoutEngine) {
            m_desiredSize = m_context->layoutEngine->MeasureStack(
                ToStackLayoutStyle(),
                BuildLayoutChildren(),
                availableWidth,
                availableHeight);
            return;
        }

        if (direction == Direction::Row) {
            MeasureRowFallback(availableWidth, availableHeight);
            return;
        }

        const float contentWidth = std::max(0.0f, availableWidth - padding.left - padding.right);
        float totalHeight = padding.top + padding.bottom;
        bool first = true;

        for (auto& child : m_children) {
            const Thickness& margin = child->Margin();
            const float availableChildWidth = std::max(0.0f, contentWidth - margin.left - margin.right);
            const float childWidth = (alignItems == AlignItems::Stretch && !child->HasFixedWidth())
                ? availableChildWidth
                : child->GetPreferredWidth(availableChildWidth);
            child->Measure(childWidth, availableHeight);
            const float childHeight = child->DesiredSize().height;

            if (!first) {
                totalHeight += spacing;
            }
            totalHeight += margin.top + childHeight + margin.bottom;
            first = false;
        }

        m_desiredSize.width = availableWidth;
        m_desiredSize.height = totalHeight;
    }

protected:
    StackLayoutStyle ToStackLayoutStyle() const {
        return StackLayoutStyle{
            direction == Direction::Row ? StackDirection::Row : StackDirection::Column,
            LayoutSpacing{padding.left, padding.top, padding.right, padding.bottom},
            spacing,
            static_cast<StackAlignItems>(alignItems),
            static_cast<StackJustifyContent>(justifyContent)
        };
    }

    std::vector<StackLayoutChild> BuildLayoutChildren() const {
        std::vector<StackLayoutChild> children;
        children.reserve(m_children.size());
        for (const auto& child : m_children) {
            const Thickness margin = child->Margin();
            children.push_back(StackLayoutChild{
                child.get(),
                LayoutSpacing{margin.left, margin.top, margin.right, margin.bottom}
            });
        }
        return children;
    }

    float MeasurePreferredHeight(float width) const override {
        if (direction == Direction::Row) {
            float maxHeight = 0.0f;
            bool first = true;
            float totalWidth = padding.left + padding.right;
            const float contentHeight = 4096.0f;
            for (const auto& child : m_children) {
                if (!first) {
                    totalWidth += spacing;
                }
                const Thickness& margin = child->Margin();
                const float preferredWidth = child->HasFixedWidth()
                    ? child->GetPreferredWidth(std::max(0.0f, width))
                    : child->GetPreferredWidth(std::max(0.0f, width));
                const float preferredHeight = child->GetPreferredHeight(preferredWidth);
                totalWidth += margin.left + preferredWidth + margin.right;
                maxHeight = std::max(maxHeight, margin.top + preferredHeight + margin.bottom);
                first = false;
            }
            (void)contentHeight;
            return padding.top + maxHeight + padding.bottom;
        }

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
    void Render(IRenderer& renderer) override {
        if (cornerRadius > 0.0f) {
            renderer.FillRoundedRect(m_bounds, background, cornerRadius);
            renderer.DrawRoundedRect(m_bounds, ColorFromHex(0x333333), 1.0f, cornerRadius);
        } else {
            renderer.FillRect(m_bounds, background);
        }
        for (auto& child : m_children) {
            child->Render(renderer);
        }
    }

private:
    void MeasureRowFallback(float availableWidth, float availableHeight) {
        const float contentHeight = std::max(0.0f, availableHeight - padding.top - padding.bottom);
        float totalWidth = padding.left + padding.right;
        float maxHeight = 0.0f;
        bool first = true;

        for (auto& child : m_children) {
            const Thickness& margin = child->Margin();
            const float availableChildHeight = std::max(0.0f, contentHeight - margin.top - margin.bottom);
            const float childWidth = child->GetPreferredWidth(std::max(0.0f, availableWidth));
            child->Measure(childWidth, availableChildHeight);
            const float childHeight = child->DesiredSize().height;

            if (!first) {
                totalWidth += spacing;
            }
            totalWidth += margin.left + childWidth + margin.right;
            maxHeight = std::max(maxHeight, margin.top + childHeight + margin.bottom);
            first = false;
        }

        m_desiredSize.width = totalWidth;
        m_desiredSize.height = padding.top + maxHeight + padding.bottom;
    }

    void ArrangeRowFallback() {
        const float contentLeft = m_bounds.left + padding.left;
        const float contentTop = m_bounds.top + padding.top;
        const float contentWidth = std::max(0.0f, m_bounds.Width() - padding.left - padding.right);
        const float contentHeight = std::max(0.0f, m_bounds.Height() - padding.top - padding.bottom);

        struct ChildLayout {
            UIElement* element = nullptr;
            Thickness margin{};
            float width = 0.0f;
            float height = 0.0f;
        };

        std::vector<ChildLayout> layouts;
        layouts.reserve(m_children.size());

        float totalWidth = 0.0f;
        bool first = true;
        for (auto& child : m_children) {
            const Thickness margin = child->Margin();
            const float availableChildHeight = std::max(0.0f, contentHeight - margin.top - margin.bottom);
            const float childWidth = child->GetPreferredWidth(contentWidth);
            child->Measure(childWidth, availableChildHeight);
            const float childHeight = child->DesiredSize().height;

            if (!first) {
                totalWidth += spacing;
            }
            totalWidth += margin.left + childWidth + margin.right;
            first = false;

            layouts.push_back(ChildLayout{child.get(), margin, childWidth, childHeight});
        }

        float x = contentLeft;
        if (justifyContent == JustifyContent::Center) {
            x += std::max(0.0f, (contentWidth - totalWidth) * 0.5f);
        } else if (justifyContent == JustifyContent::End) {
            x += std::max(0.0f, contentWidth - totalWidth);
        }

        for (size_t i = 0; i < layouts.size(); ++i) {
            const auto& layout = layouts[i];
            float y = contentTop + layout.margin.top;
            const float availableChildHeight = std::max(0.0f, contentHeight - layout.margin.top - layout.margin.bottom);

            if (alignItems == AlignItems::Center) {
                y += std::max(0.0f, (availableChildHeight - layout.height) * 0.5f);
            } else if (alignItems == AlignItems::End) {
                y += std::max(0.0f, availableChildHeight - layout.height);
            }

            x += layout.margin.left;
            layout.element->Arrange(Rect::Make(x, y, x + layout.width, y + layout.height));
            x += layout.width + layout.margin.right;
            if (i + 1 < layouts.size()) {
                x += spacing;
            }
        }
    }
};

class GridPanel : public UIElement {
public:
    int columns = 3;
    float cellSpacing = 8.0f;
    float cornerRadius = 0.0f;
    Thickness padding{8, 8, 8, 8};
    Color background = ColorFromHex(0x1A1A1A);
    float rowHeight = 120.0f;

    void Arrange(const Rect& finalRect) override {
        UIElement::Arrange(finalRect);

        const float x0 = m_bounds.left + padding.left;
        float y = m_bounds.top + padding.top;
        const float contentWidth = m_bounds.Width() - padding.left - padding.right;
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
            child->Arrange(Rect::Make(
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

    void Measure(float availableWidth, float availableHeight) override {
        const float contentWidth = std::max(0.0f, availableWidth - padding.left - padding.right);
        const int cols = std::max(1, columns);
        const float cellWidth = (contentWidth - cellSpacing * (cols - 1)) / cols;
        const int cellCount = static_cast<int>(m_children.size());
        const int rows = (cellCount + cols - 1) / cols;

        for (auto& child : m_children) {
            const Thickness& margin = child->Margin();
            const float availableCellWidth = std::max(0.0f, cellWidth - margin.left - margin.right);
            const float childWidth = child->HasFixedWidth() ? child->GetPreferredWidth(availableCellWidth) : availableCellWidth;
            child->Measure(childWidth, availableHeight);
        }

        m_desiredSize.width = availableWidth;
        m_desiredSize.height = padding.top + padding.bottom + rows * rowHeight + std::max(0, rows - 1) * cellSpacing;
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
    void Render(IRenderer& renderer) override {
        if (cornerRadius > 0.0f) {
            renderer.FillRoundedRect(m_bounds, background, cornerRadius);
            renderer.DrawRoundedRect(m_bounds, ColorFromHex(0x333333), 1.0f, cornerRadius);
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
        const Size measured = MeasureTextValue(m_text, m_fontSize, availableWidth > 0.0f ? availableWidth : 4096.0f);
        return std::min(availableWidth, measured.width + 4.0f);
    }

    float MeasurePreferredHeight(float width) const override {
        const Size measured = MeasureTextValue(m_text, m_fontSize, width > 0.0f ? width : 4096.0f);
        return measured.height + 8.0f;
    }

public:
    void Render(IRenderer& renderer) override {
        renderer.DrawTextW(
            m_text.c_str(),
            static_cast<UINT32>(m_text.size()),
            m_bounds,
            m_color,
            m_fontSize
        );
    }

    float m_fontSize = 15.0f;
    Color m_color = ColorFromHex(0xEDEDED);

private:
    std::wstring m_text;
};

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

class Button : public UIElement {
public:
    explicit Button(std::wstring text) : m_text(std::move(text)) {}

    void SetOnClick(std::function<void()> onClick) { m_onClick = std::move(onClick); }
    void SetText(std::wstring text) { m_text = std::move(text); }

    float cornerRadius = 4.0f;
    float fontSize = 14.0f;
    Color background = ColorFromHex(0x2D2D30);
    Color foreground = ColorFromHex(0xFFFFFF);

    bool IsFocusable() const override { return true; }

    bool OnFocus() override {
        if (!m_hasFocus) {
            m_hasFocus = true;
            return true;
        }
        return false;
    }

    bool OnBlur() override {
        if (m_hasFocus) {
            m_hasFocus = false;
            return true;
        }
        return false;
    }

    bool OnKeyDown(WPARAM keyCode, LPARAM /*lParam*/) override {
        if (keyCode == VK_SPACE || keyCode == VK_RETURN) {
            if (!m_pressed) {
                m_pressed = true;
                return true;
            }
        }
        return false;
    }

    bool OnKeyUp(WPARAM keyCode, LPARAM /*lParam*/) override {
        if ((keyCode == VK_SPACE || keyCode == VK_RETURN) && m_pressed) {
            m_pressed = false;
            if (m_onClick) {
                m_onClick();
            }
            return true;
        }
        return false;
    }

protected:
    float MeasurePreferredWidth(float availableWidth) const override {
        const Size measured = MeasureTextValue(m_text, fontSize, availableWidth > 0.0f ? availableWidth : 4096.0f);
        return std::min(availableWidth, measured.width + 24.0f);
    }

    float MeasurePreferredHeight(float width) const override {
        const Size measured = MeasureTextValue(m_text, fontSize, width > 0.0f ? width : 4096.0f);
        return measured.height + 14.0f;
    }

public:
    void Render(IRenderer& renderer) override {
        Color bg = background;
        if (m_pressed && m_hovered) {
            bg = ColorFromHex(0x0E639C);
        } else if (m_hovered) {
            bg = ColorFromHex(0x3E3E42);
        }

        if (cornerRadius > 0.0f) {
            renderer.FillRoundedRect(m_bounds, bg, cornerRadius);
            renderer.DrawRoundedRect(m_bounds, ColorFromHex(0x6A6A6A), 1.0f, cornerRadius);
            if (m_hasFocus) {
                renderer.DrawRoundedRect(m_bounds, ColorFromHex(0xFFFFFF), 2.0f, cornerRadius);
            }
        } else {
            renderer.FillRect(m_bounds, bg);
            renderer.DrawRect(m_bounds, ColorFromHex(0x6A6A6A), 1.0f);
            if (m_hasFocus) {
                renderer.DrawRect(m_bounds, ColorFromHex(0xFFFFFF), 2.0f);
            }
        }

        Rect textRect = m_bounds;
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

class TextInput : public UIElement {
public:
    explicit TextInput(std::wstring text = L"") : m_text(std::move(text)), m_caretPosition(m_text.size()) {}

    bool IsFocusable() const override { return true; }

    bool OnFocus() override {
        if (!m_hasFocus) {
            m_hasFocus = true;
            return true;
        }
        return false;
    }

    bool OnBlur() override {
        if (m_hasFocus) {
            m_hasFocus = false;
            return true;
        }
        return false;
    }

    bool OnKeyDown(WPARAM keyCode, LPARAM /*lParam*/) override {
        const bool shiftDown = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
        const bool ctrlDown = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
        switch (keyCode) {
            case VK_BACK:
                if (HasSelection()) {
                    DeleteSelection();
                    ResetCaretBlink();
                    return true;
                }
                if (m_caretPosition > 0) {
                    m_text.erase(m_caretPosition - 1, 1);
                    --m_caretPosition;
                    ClearSelection();
                    ResetCaretBlink();
                    return true;
                }
                break;
            case VK_DELETE:
                if (HasSelection()) {
                    DeleteSelection();
                    ResetCaretBlink();
                    return true;
                }
                if (m_caretPosition < m_text.size()) {
                    m_text.erase(m_caretPosition, 1);
                    ResetCaretBlink();
                    return true;
                }
                break;
            case VK_LEFT:
                if (m_caretPosition > 0) {
                    if (shiftDown && !HasSelection()) {
                        m_selectionStart = m_caretPosition;
                    }
                    --m_caretPosition;
                    if (shiftDown) {
                        m_selectionEnd = m_caretPosition;
                    } else {
                        ClearSelection();
                    }
                    ResetCaretBlink();
                    return true;
                }
                break;
            case VK_RIGHT:
                if (m_caretPosition < m_text.size()) {
                    if (shiftDown && !HasSelection()) {
                        m_selectionStart = m_caretPosition;
                    }
                    ++m_caretPosition;
                    if (shiftDown) {
                        m_selectionEnd = m_caretPosition;
                    } else {
                        ClearSelection();
                    }
                    ResetCaretBlink();
                    return true;
                }
                break;
            case VK_HOME:
                m_caretPosition = 0;
                if (shiftDown) {
                    m_selectionEnd = m_caretPosition;
                } else {
                    ClearSelection();
                }
                ResetCaretBlink();
                return true;
            case VK_END:
                m_caretPosition = m_text.size();
                if (shiftDown) {
                    m_selectionEnd = m_caretPosition;
                } else {
                    ClearSelection();
                }
                ResetCaretBlink();
                return true;
            case 'A':
                if (ctrlDown) {
                    m_selectionStart = 0;
                    m_selectionEnd = m_text.size();
                    m_caretPosition = m_selectionEnd;
                    ResetCaretBlink();
                    return true;
                }
                break;
            case 'C':
                if (ctrlDown && HasSelection()) {
                    const auto [start, end] = GetSelectionRange();
                    const std::wstring selectedText = m_text.substr(start, end - start);
                    if (OpenClipboard(nullptr)) {
                        EmptyClipboard();
                        const size_t bytes = (selectedText.size() + 1) * sizeof(wchar_t);
                        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, bytes);
                        if (hMem) {
                            void* dest = GlobalLock(hMem);
                            if (dest) {
                                memcpy(dest, selectedText.c_str(), bytes);
                                GlobalUnlock(hMem);
                                SetClipboardData(CF_UNICODETEXT, hMem);
                            } else {
                                GlobalFree(hMem);
                            }
                        }
                        CloseClipboard();
                    }
                    return true;
                }
                break;
            case 'V':
                if (ctrlDown) {
                    if (OpenClipboard(nullptr)) {
                        HANDLE hData = GetClipboardData(CF_UNICODETEXT);
                        if (hData) {
                            wchar_t* clipboardText = static_cast<wchar_t*>(GlobalLock(hData));
                            if (clipboardText) {
                                if (HasSelection()) {
                                    DeleteSelection();
                                }
                                m_text.insert(m_caretPosition, clipboardText);
                                m_caretPosition += wcslen(clipboardText);
                                GlobalUnlock(hData);
                                ClearSelection();
                                ResetCaretBlink();
                                CloseClipboard();
                                return true;
                            }
                        }
                        CloseClipboard();
                    }
                }
                break;
            case 'X':
                if (ctrlDown && HasSelection()) {
                    const auto [start, end] = GetSelectionRange();
                    const std::wstring selectedText = m_text.substr(start, end - start);
                    if (OpenClipboard(nullptr)) {
                        EmptyClipboard();
                        const size_t bytes = (selectedText.size() + 1) * sizeof(wchar_t);
                        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, bytes);
                        if (hMem) {
                            void* dest = GlobalLock(hMem);
                            if (dest) {
                                memcpy(dest, selectedText.c_str(), bytes);
                                GlobalUnlock(hMem);
                                SetClipboardData(CF_UNICODETEXT, hMem);
                            } else {
                                GlobalFree(hMem);
                            }
                        }
                        CloseClipboard();
                    }
                    DeleteSelection();
                    ResetCaretBlink();
                    return true;
                }
                break;
        }
        return false;
    }

    bool OnChar(wchar_t ch) override {
        if (ch >= L' ' && ch != 0x7F) {
            if (HasSelection()) {
                DeleteSelection();
            }
            m_text.insert(m_caretPosition, 1, ch);
            ++m_caretPosition;
            ResetCaretBlink();
            return true;
        }
        return false;
    }

    bool OnMouseDown(float x, float y) override {
        const Rect textRect = Rect::Make(m_bounds.left + 8.0f, m_bounds.top + 6.0f, m_bounds.right - 8.0f, m_bounds.bottom - 6.0f);
        if (!HitTest(x, y)) {
            return false;
        }

        const float localX = x - textRect.left;
        const size_t newCaret = ComputeCaretIndex(localX, textRect);
        const bool shiftDown = (GetKeyState(VK_SHIFT) & 0x8000) != 0;

        if (shiftDown) {
            if (!HasSelection()) {
                m_selectionStart = m_caretPosition;
            }
            m_caretPosition = newCaret;
            m_selectionEnd = m_caretPosition;
        } else {
            m_caretPosition = newCaret;
            ClearSelection();
        }

        m_dragAnchor = m_caretPosition;
        m_isDragging = true;
        ResetCaretBlink();
        return true;
    }

    bool OnMouseMove(float x, float y) override {
        if (!m_isDragging) {
            return false;
        }

        const Rect textRect = Rect::Make(m_bounds.left + 8.0f, m_bounds.top + 6.0f, m_bounds.right - 8.0f, m_bounds.bottom - 6.0f);
        const float localX = x - textRect.left;
        const size_t newCaret = ComputeCaretIndex(localX, textRect);
        if (newCaret == m_caretPosition) {
            return false;
        }

        m_caretPosition = newCaret;
        if (!HasSelection()) {
            m_selectionStart = m_dragAnchor;
        }
        m_selectionEnd = m_caretPosition;
        ResetCaretBlink();
        return true;
    }

    bool OnMouseUp(float x, float y) override {
        if (!m_isDragging) {
            return false;
        }
        m_isDragging = false;
        return true;
    }

    bool OnMouseLeave() override {
        if (!m_isDragging) {
            return false;
        }
        m_isDragging = false;
        return true;
    }

    size_t ComputeCaretIndex(float localX, const Rect& textRect) const {
        size_t newCaret = m_text.size();
        for (size_t i = 0; i <= m_text.size(); ++i) {
            const std::wstring prefix = m_text.substr(0, i);
            const Size metrics = MeasureTextValue(prefix.empty() ? L" " : prefix, fontSize, std::max(1.0f, textRect.right - textRect.left));
            if (localX < metrics.width + 4.0f) {
                newCaret = i;
                break;
            }
        }
        return newCaret;
    }

    bool OnTimer(UINT_PTR /*timerId*/) override {
        if (!m_hasFocus) {
            return false;
        }
        const DWORD now = GetTickCount();
        if (now - m_lastBlink >= 500) {
            m_showCaret = !m_showCaret;
            m_lastBlink = now;
            return true;
        }
        return false;
    }

    void SetFontSize(float size) { fontSize = size; }
    void SetBackgroundColor(const Color& color) { background = color; }
    void SetTextColor(const Color& color) { textColor = color; }
    void SetCornerRadius(float radius) { cornerRadius = radius; }

protected:
    float MeasurePreferredWidth(float availableWidth) const override {
        const std::wstring measureText = m_text.empty() ? L" " : m_text;
        const Size measured = MeasureTextValue(measureText, fontSize, std::max(1.0f, availableWidth - 16.0f));
        return std::min(availableWidth, measured.width + 16.0f);
    }

    float MeasurePreferredHeight(float width) const override {
        const std::wstring measureText = m_text.empty() ? L" " : m_text;
        const Size measured = MeasureTextValue(measureText, fontSize, std::max(1.0f, width - 16.0f));
        return measured.height + 12.0f;
    }

public:
    void Render(IRenderer& renderer) override {
        renderer.FillRoundedRect(m_bounds, background, cornerRadius);
        renderer.DrawRoundedRect(m_bounds, borderColor, 1.0f, cornerRadius);
        if (m_hasFocus) {
            renderer.DrawRoundedRect(m_bounds, focusBorderColor, 2.0f, cornerRadius);
        }

        Rect textRect = m_bounds;
        textRect.left += 8.0f;
        textRect.top += 6.0f;
        if (HasSelection()) {
            const auto [selStart, selEnd] = GetSelectionRange();
            std::wstring prefix = m_text.substr(0, selStart);
            std::wstring selection = m_text.substr(selStart, selEnd - selStart);
            const Size prefixMetrics = MeasureTextValue(prefix.empty() ? L" " : prefix, fontSize, std::max(1.0f, textRect.right - textRect.left));
            const Size selectionMetrics = MeasureTextValue(selection.empty() ? L" " : selection, fontSize, std::max(1.0f, textRect.right - textRect.left));
            Rect selectionRect = Rect::Make(
                textRect.left + prefixMetrics.width,
                textRect.top,
                textRect.left + prefixMetrics.width + selectionMetrics.width,
                textRect.top + selectionMetrics.height
            );
            renderer.FillRect(selectionRect, ColorFromHex(0x3A86FF));
        }
        renderer.DrawTextW(m_text.c_str(), static_cast<UINT32>(m_text.size()), textRect, textColor, fontSize);

        if (m_hasFocus) {
            const std::wstring prefix = m_text.substr(0, m_caretPosition);
            const Size caretMetrics = MeasureTextValue(prefix.empty() ? L" " : prefix, fontSize, std::max(1.0f, textRect.right - textRect.left));
            const float caretX = textRect.left + caretMetrics.width;
            const float caretTop = textRect.top;
            const float caretBottom = textRect.top + MeasureTextValue(L"|", fontSize, std::max(1.0f, textRect.right - textRect.left)).height;
            if (m_showCaret) {
                renderer.DrawRect(Rect::Make(caretX, caretTop, caretX + 1.0f, caretBottom), textColor, 1.0f);
            }
        }
    }

private:
    std::wstring m_text;
    size_t m_caretPosition = 0;
    size_t m_selectionStart = 0;
    size_t m_selectionEnd = 0;
    bool m_showCaret = true;
    DWORD m_lastBlink = 0;
    float fontSize = 14.0f;
    Color background = ColorFromHex(0x1F1F1F);
    Color borderColor = ColorFromHex(0x6A6A6A);
    Color focusBorderColor = ColorFromHex(0xFFFFFF);
    Color textColor = ColorFromHex(0xEDEDED);
    float cornerRadius = 4.0f;

    bool HasSelection() const {
        return m_selectionStart != m_selectionEnd;
    }

    std::pair<size_t, size_t> GetSelectionRange() const {
        return m_selectionStart <= m_selectionEnd
            ? std::make_pair(m_selectionStart, m_selectionEnd)
            : std::make_pair(m_selectionEnd, m_selectionStart);
    }

    void ClearSelection() {
        m_selectionStart = m_selectionEnd = m_caretPosition;
    }

    void DeleteSelection() {
        if (!HasSelection()) {
            return;
        }
        const auto [start, end] = GetSelectionRange();
        m_text.erase(start, end - start);
        m_caretPosition = start;
        ClearSelection();
    }

    void ResetCaretBlink() {
        m_showCaret = true;
        m_lastBlink = GetTickCount();
    }

    bool m_isDragging = false;
    size_t m_dragAnchor = 0;
};

class Checkbox : public UIElement {
public:
    explicit Checkbox(std::wstring text = L"") : m_text(std::move(text)) {}

    bool IsFocusable() const override { return true; }

    bool OnFocus() override {
        if (!m_hasFocus) {
            m_hasFocus = true;
            return true;
        }
        return false;
    }

    bool OnBlur() override {
        if (m_hasFocus) {
            m_hasFocus = false;
            return true;
        }
        return false;
    }

    bool OnKeyDown(WPARAM keyCode, LPARAM /*lParam*/) override {
        if (keyCode == VK_SPACE || keyCode == VK_RETURN) {
            m_checked = !m_checked;
            return true;
        }
        return false;
    }

    float MeasurePreferredWidth(float availableWidth) const override {
        const Size textSize = MeasureTextValue(m_text.empty() ? L" " : m_text, fontSize, std::max(1.0f, availableWidth - 30.0f));
        return std::min(availableWidth, textSize.width + 30.0f);
    }

    float MeasurePreferredHeight(float width) const override {
        const Size textSize = MeasureTextValue(m_text.empty() ? L" " : m_text, fontSize, std::max(1.0f, width - 30.0f));
        return textSize.height + 10.0f;
    }

    void Render(IRenderer& renderer) override {
        const Rect box = Rect::Make(m_bounds.left, m_bounds.top + 2.0f, m_bounds.left + 18.0f, m_bounds.top + 20.0f);
        renderer.FillRect(box, ColorFromHex(0x2D2D30));
        renderer.DrawRect(box, ColorFromHex(0x6A6A6A), 1.0f);
        if (m_checked) {
            const Rect mark = Rect::Make(box.left + 4.0f, box.top + 4.0f, box.right - 4.0f, box.bottom - 4.0f);
            renderer.FillRect(mark, ColorFromHex(0x2D6CDF));
        }
        if (m_hasFocus) {
            renderer.DrawRect(box, ColorFromHex(0xFFFFFF), 2.0f);
        }

        Rect textRect = m_bounds;
        textRect.left += 26.0f;
        renderer.DrawTextW(m_text.c_str(), static_cast<UINT32>(m_text.size()), textRect, textColor, fontSize);
    }

    bool OnMouseDown(float x, float y) override {
        if (!HitTest(x, y)) {
            return false;
        }
        m_pressed = true;
        return true;
    }

    bool OnMouseUp(float x, float y) override {
        const bool hit = HitTest(x, y);
        if (m_pressed && hit) {
            m_checked = !m_checked;
        }
        m_pressed = false;
        return hit;
    }

    bool OnMouseLeave() override {
        if (!m_pressed) {
            return false;
        }
        m_pressed = false;
        return true;
    }

    void SetText(std::wstring text) { m_text = std::move(text); }
    void SetChecked(bool checked) { m_checked = checked; }
    bool IsChecked() const { return m_checked; }

private:
    std::wstring m_text;
    bool m_checked = false;
    bool m_pressed = false;
    Color textColor = ColorFromHex(0xEDEDED);
    float fontSize = 14.0f;
};

class RadioButton : public UIElement {
public:
    explicit RadioButton(std::wstring text = L"", std::wstring group = L"default")
        : m_text(std::move(text)), m_group(std::move(group)) {
        s_groups[m_group].push_back(this);
    }

    ~RadioButton() {
        auto& group = s_groups[m_group];
        group.erase(std::remove(group.begin(), group.end(), this), group.end());
    }

    bool IsFocusable() const override { return true; }

    bool OnFocus() override {
        if (!m_hasFocus) {
            m_hasFocus = true;
            return true;
        }
        return false;
    }

    bool OnBlur() override {
        if (m_hasFocus) {
            m_hasFocus = false;
            return true;
        }
        return false;
    }

    bool OnKeyDown(WPARAM keyCode, LPARAM /*lParam*/) override {
        if (keyCode == VK_SPACE || keyCode == VK_RETURN) {
            SetChecked(true);
            return true;
        }
        return false;
    }

    float MeasurePreferredWidth(float availableWidth) const override {
        const Size textSize = MeasureTextValue(m_text.empty() ? L" " : m_text, fontSize, std::max(1.0f, availableWidth - 28.0f));
        return std::min(availableWidth, textSize.width + 28.0f);
    }

    float MeasurePreferredHeight(float width) const override {
        const Size textSize = MeasureTextValue(m_text.empty() ? L" " : m_text, fontSize, std::max(1.0f, width - 28.0f));
        return textSize.height + 10.0f;
    }

    void Render(IRenderer& renderer) override {
        const Rect circle = Rect::Make(m_bounds.left, m_bounds.top + 2.0f, m_bounds.left + 18.0f, m_bounds.top + 20.0f);
        renderer.DrawRoundedRect(circle, ColorFromHex(0x6A6A6A), 1.0f, 9.0f);
        if (m_checked) {
            const Rect dot = Rect::Make(circle.left + 5.0f, circle.top + 5.0f, circle.right - 5.0f, circle.bottom - 5.0f);
            renderer.FillRoundedRect(dot, ColorFromHex(0x2D6CDF), 5.0f);
        }
        if (m_hasFocus) {
            renderer.DrawRoundedRect(circle, ColorFromHex(0xFFFFFF), 2.0f, 9.0f);
        }

        Rect textRect = m_bounds;
        textRect.left += 26.0f;
        renderer.DrawTextW(m_text.c_str(), static_cast<UINT32>(m_text.size()), textRect, textColor, fontSize);
    }

    bool OnMouseDown(float x, float y) override {
        const bool hit = HitTest(x, y);
        if (hit) {
            m_pressed = true;
        }
        return m_pressed;
    }

    bool OnMouseUp(float x, float y) override {
        const bool hit = HitTest(x, y);
        if (m_pressed && hit) {
            SetChecked(true);
        }
        m_pressed = false;
        return hit;
    }

    bool OnMouseLeave() override {
        if (!m_pressed) {
            return false;
        }
        m_pressed = false;
        return true;
    }

    void SetText(std::wstring text) { m_text = std::move(text); }
    void SetGroup(std::wstring group) { m_group = std::move(group); }
    void SetChecked(bool checked) {
        if (checked && !m_checked) {
            for (RadioButton* button : s_groups[m_group]) {
                if (button != this) {
                    button->m_checked = false;
                }
            }
        }
        m_checked = checked;
    }
    bool IsChecked() const { return m_checked; }

private:
    std::wstring m_text;
    std::wstring m_group;
    bool m_checked = false;
    bool m_pressed = false;
    Color textColor = ColorFromHex(0xEDEDED);
    float fontSize = 14.0f;
    inline static std::map<std::wstring, std::vector<RadioButton*>> s_groups;
};

class Slider : public UIElement {
public:
    explicit Slider(std::wstring label = L"") : m_label(std::move(label)) {}

    bool IsFocusable() const override { return true; }

    bool OnFocus() override {
        if (!m_hasFocus) {
            m_hasFocus = true;
            return true;
        }
        return false;
    }

    bool OnBlur() override {
        if (m_hasFocus) {
            m_hasFocus = false;
            return true;
        }
        return false;
    }

    bool OnKeyDown(WPARAM keyCode, LPARAM /*lParam*/) override {
        if (keyCode == VK_LEFT || keyCode == VK_DOWN) {
            SetValue(m_value - m_step);
            return true;
        }
        if (keyCode == VK_RIGHT || keyCode == VK_UP) {
            SetValue(m_value + m_step);
            return true;
        }
        return false;
    }

    bool OnMouseDown(float x, float y) override {
        if (!HitTest(x, y)) {
            return false;
        }
        m_dragging = true;
        UpdateValueFromPoint(x);
        return true;
    }

    bool OnMouseMove(float x, float y) override {
        if (m_dragging) {
            UpdateValueFromPoint(x);
            return true;
        }
        return false;
    }

    bool OnMouseUp(float x, float y) override {
        if (m_dragging) {
            m_dragging = false;
            return true;
        }
        return false;
    }

    bool OnMouseLeave() override {
        if (!m_dragging) {
            return false;
        }
        m_dragging = false;
        return true;
    }

    float MeasurePreferredWidth(float availableWidth) const override {
        return std::min(availableWidth, 220.0f);
    }

    float MeasurePreferredHeight(float width) const override {
        (void)width;
        return 42.0f;
    }

    void Render(IRenderer& renderer) override {
        renderer.FillRoundedRect(m_bounds, background, cornerRadius);
        renderer.DrawRoundedRect(m_bounds, borderColor, 1.0f, cornerRadius);
        if (m_hasFocus) {
            renderer.DrawRoundedRect(m_bounds, focusBorderColor, 2.0f, cornerRadius);
        }

        const float trackLeft = m_bounds.left + 14.0f;
        const float trackRight = m_bounds.right - 14.0f;
        const float trackTop = m_bounds.top + (m_bounds.bottom - m_bounds.top) * 0.5f - 4.0f;
        const float trackBottom = trackTop + 8.0f;
        const Rect trackRect = Rect::Make(trackLeft, trackTop, trackRight, trackBottom);
        renderer.FillRect(trackRect, ColorFromHex(0x333333));

        const float position = trackLeft + (trackRight - trackLeft) * ((m_value - m_min) / std::max(1.0f, m_max - m_min));
        const Rect thumbRect = Rect::Make(position - 8.0f, trackTop - 6.0f, position + 8.0f, trackBottom + 6.0f);
        renderer.FillRoundedRect(thumbRect, highlightColor, 8.0f);
        renderer.DrawRoundedRect(thumbRect, ColorFromHex(0x6A6A6A), 1.0f, 8.0f);

        if (!m_label.empty()) {
            Rect labelRect = m_bounds;
            labelRect.right = trackLeft - 8.0f;
            renderer.DrawTextW(m_label.c_str(), static_cast<UINT32>(m_label.size()), labelRect, textColor, fontSize);
        }
    }

    void SetText(std::wstring text) { m_label = std::move(text); }
    void SetRange(float minValue, float maxValue) { m_min = minValue; m_max = maxValue; SetValue(m_value); }
    void SetValue(float value) { m_value = std::clamp(value, m_min, m_max); }
    void SetStep(float step) { m_step = std::max(0.001f, step); }

private:
    void UpdateValueFromPoint(float x) {
        const float trackLeft = m_bounds.left + 14.0f;
        const float trackRight = m_bounds.right - 14.0f;
        const float ratio = std::clamp((x - trackLeft) / (trackRight - trackLeft), 0.0f, 1.0f);
        SetValue(m_min + ratio * (m_max - m_min));
    }

    std::wstring m_label;
    float m_min = 0.0f;
    float m_max = 1.0f;
    float m_value = 0.5f;
    float m_step = 0.05f;
    bool m_dragging = false;
    float fontSize = 13.0f;
    Color background = ColorFromHex(0x1F1F1F);
    Color borderColor = ColorFromHex(0x6A6A6A);
    Color focusBorderColor = ColorFromHex(0xFFFFFF);
    Color textColor = ColorFromHex(0xEDEDED);
    Color highlightColor = ColorFromHex(0x2D6CDF);
    float cornerRadius = 8.0f;
};
