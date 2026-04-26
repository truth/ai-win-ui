#pragma once

#include "box_decoration.h"
#include "graphics_types.h"
#include "layout_engine.h"
#include "renderer.h"
#include "style.h"
#include "ui_context.h"
#include "ui_text_metrics.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <memory>
#include <map>
#include <string>
#include <vector>

class UIElement {
public:
    enum class SelfAlign {
        Auto,
        Stretch,
        Start,
        Center,
        End
    };

    virtual ~UIElement() = default;

    void SetBounds(const Rect& rect) { m_bounds = rect; }
    Rect Bounds() const { return m_bounds; }
    Size DesiredSize() const { return m_desiredSize; }
    void SetMargin(const Thickness& margin) { m_margin = margin; }
    const Thickness& Margin() const { return m_margin; }
    void SetBorder(const Thickness& border) { m_border = border; }
    const Thickness& Border() const { return m_border; }
    void SetFixedWidth(float width) { m_fixedWidth = width; }
    void SetFixedHeight(float height) { m_fixedHeight = height; }
    void SetMinWidth(float width) { m_minWidth = std::max(0.0f, width); }
    void SetMaxWidth(float width) { m_maxWidth = std::max(0.0f, width); }
    void SetMinHeight(float height) { m_minHeight = std::max(0.0f, height); }
    void SetMaxHeight(float height) { m_maxHeight = std::max(0.0f, height); }
    void SetFlexGrow(float flexGrow) { m_flexGrow = std::max(0.0f, flexGrow); }
    void SetFlexShrink(float flexShrink) { m_flexShrink = std::max(0.0f, flexShrink); }
    void SetFlexBasis(float flexBasis) { m_flexBasis = std::max(0.0f, flexBasis); }
    void SetAlignSelf(SelfAlign alignSelf) { m_alignSelf = alignSelf; }
    bool HasFixedWidth() const { return m_fixedWidth >= 0.0f; }
    bool HasFixedHeight() const { return m_fixedHeight >= 0.0f; }
    bool HasMinWidth() const { return m_minWidth >= 0.0f; }
    bool HasMaxWidth() const { return m_maxWidth >= 0.0f; }
    bool HasMinHeight() const { return m_minHeight >= 0.0f; }
    bool HasMaxHeight() const { return m_maxHeight >= 0.0f; }
    float FlexGrow() const { return m_flexGrow; }
    float FlexShrink() const { return m_flexShrink; }
    bool HasFlexBasis() const { return m_flexBasis >= 0.0f; }
    float FlexBasis() const { return ScaleValue(m_flexBasis); }
    float MinWidth() const { return ScaleValue(m_minWidth); }
    float MaxWidth() const { return ScaleValue(m_maxWidth); }
    float MinHeight() const { return ScaleValue(m_minHeight); }
    float MaxHeight() const { return ScaleValue(m_maxHeight); }
    SelfAlign AlignSelf() const { return m_alignSelf; }
    void SetContext(UIContext* context) {
        m_context = context;
        for (auto& child : m_children) {
            child->SetContext(context);
        }
    }
    UIContext* Context() const { return m_context; }

    float GetPreferredWidth(float availableWidth) const {
        const float clampedWidth = std::max(0.0f, availableWidth);
        const float preferredWidth = HasFixedWidth() ? ScaleValue(m_fixedWidth) : MeasurePreferredWidth(clampedWidth);
        return std::clamp(ClampWidth(preferredWidth), 0.0f, clampedWidth);
    }

    float GetPreferredHeight(float width) const {
        const float clampedWidth = ClampWidth(std::max(0.0f, width));
        if (HasFixedHeight()) {
            return ClampHeight(std::max(0.0f, ScaleValue(m_fixedHeight)));
        }
        return ClampHeight(std::max(0.0f, MeasurePreferredHeight(clampedWidth)));
    }

    virtual void Measure(float availableWidth, float availableHeight) {
        (void)availableHeight;
        const float width = GetPreferredWidth(availableWidth);
        m_desiredSize.width = width;
        m_desiredSize.height = GetPreferredHeight(width);
    }
    virtual Size MeasureForLayout(float availableWidth, float availableHeight) {
        Measure(availableWidth, availableHeight);
        return m_desiredSize;
    }
    virtual void Arrange(const Rect& finalRect) { m_bounds = finalRect; }
    virtual void Render(IRenderer& renderer) {
        for (auto& child : m_children) {
            child->Render(renderer);
        }
    }

    virtual bool IsFocusable() const { return false; }
    bool HasFocus() const { return m_hasFocus; }
    bool IsHovered() const { return m_hovered; }
    bool IsPressed() const { return m_pressed; }
    bool IsDisabled() const { return m_disabled; }
    void SetEnabled(bool enabled) { m_disabled = !enabled; }

    void SetStyle(ComponentStyle style) { m_style = std::move(style); }
    const ComponentStyle& Style() const { return m_style; }
    ComponentStyle& MutableStyle() { return m_style; }

    virtual StyleState GetCurrentState() const {
        if (m_disabled) return StyleState::Disabled;
        if (m_pressed)  return StyleState::Pressed;
        if (m_hovered)  return StyleState::Hover;
        if (m_hasFocus) return StyleState::Focused;
        return StyleState::Normal;
    }

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

    virtual bool OnTimer(UINT_PTR timerId) {
        bool changed = false;
        for (auto& child : m_children) {
            changed = child->OnTimer(timerId) || changed;
        }
        return changed;
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
    bool IsLayoutLeaf() const { return m_children.empty(); }

public:
    float DpiScale() const {
        return m_context ? std::max(1.0f / 96.0f, m_context->dpiScale) : 1.0f;
    }

    float ScaleValue(float value) const {
        return value * DpiScale();
    }

    Thickness ScaleThickness(const Thickness& value) const {
        return Thickness{
            ScaleValue(value.left),
            ScaleValue(value.top),
            ScaleValue(value.right),
            ScaleValue(value.bottom)
        };
    }

protected:
    float ClampWidth(float width) const {
        float clamped = std::max(0.0f, width);
        if (HasMinWidth()) {
            clamped = std::max(clamped, MinWidth());
        }
        if (HasMaxWidth()) {
            clamped = std::min(clamped, MaxWidth());
        }
        return clamped;
    }

    float ClampHeight(float height) const {
        float clamped = std::max(0.0f, height);
        if (HasMinHeight()) {
            clamped = std::max(clamped, MinHeight());
        }
        if (HasMaxHeight()) {
            clamped = std::min(clamped, MaxHeight());
        }
        return clamped;
    }

    Size MeasureTextValue(const std::wstring& text,
                          float fontSize,
                          float maxWidth,
                          TextWrapMode wrapMode = TextWrapMode::Wrap) const {
        return MeasureTextLayout(
            m_context ? m_context->textMeasurer : nullptr,
            text,
            ScaleValue(fontSize),
            maxWidth,
            wrapMode);
    }

    Rect m_bounds{};
    Size m_desiredSize{};
    std::vector<std::unique_ptr<UIElement>> m_children;
    Thickness m_margin{};
    Thickness m_border{};
    float m_fixedWidth = -1.0f;
    float m_fixedHeight = -1.0f;
    float m_minWidth = -1.0f;
    float m_maxWidth = -1.0f;
    float m_minHeight = -1.0f;
    float m_maxHeight = -1.0f;
    float m_flexGrow = 0.0f;
    float m_flexShrink = 0.0f;
    float m_flexBasis = -1.0f;
    SelfAlign m_alignSelf = SelfAlign::Auto;
    bool m_hasFocus = false;
    bool m_hovered = false;
    bool m_pressed = false;
    bool m_disabled = false;
    ComponentStyle m_style{};
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

    enum class Wrap {
        NoWrap,
        Wrap
    };

    Thickness padding{8, 8, 8, 8};
    float spacing = 6.0f;
    float cornerRadius = 0.0f;
    Color background = ColorFromHex(0x000000, 0.0f);
    Color borderColor = ColorFromHex(0x6A6A6A, 0.0f);
    Direction direction = Direction::Column;
    Wrap wrap = Wrap::NoWrap;
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

        const Thickness scaledPadding = ScaleThickness(padding);
        const float scaledSpacing = ScaleValue(spacing);
        const float contentLeft = m_bounds.left + scaledPadding.left;
        const float contentTop = m_bounds.top + scaledPadding.top;
        const float contentWidth = std::max(0.0f, m_bounds.Width() - scaledPadding.left - scaledPadding.right);
        const float contentHeight = std::max(0.0f, m_bounds.Height() - scaledPadding.top - scaledPadding.bottom);

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
            const Thickness margin = child->ScaleThickness(child->Margin());
            const float availableChildWidth = std::max(0.0f, contentWidth - margin.left - margin.right);
            const float childWidth = (alignItems == AlignItems::Stretch && !child->HasFixedWidth())
                ? availableChildWidth
                : child->GetPreferredWidth(availableChildWidth);
            child->Measure(childWidth, contentHeight);
            const float childHeight = child->DesiredSize().height;
            const float blockHeight = margin.top + childHeight + margin.bottom;

            if (!first) {
                totalHeight += scaledSpacing;
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
        float spacingBetweenChildren = layouts.size() > 1 ? scaledSpacing : 0.0f;

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
                    spacingBetweenChildren = std::max(scaledSpacing, distributedSpacing);
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
        const float resolvedWidth = GetPreferredWidth(availableWidth);

        if (m_context && m_context->layoutEngine) {
            const Size measured = m_context->layoutEngine->MeasureStack(
                ToStackLayoutStyle(),
                BuildLayoutChildren(),
                resolvedWidth,
                availableHeight);
            m_desiredSize.width = resolvedWidth;
            m_desiredSize.height = HasFixedHeight()
                ? GetPreferredHeight(resolvedWidth)
                : ClampHeight(std::max(0.0f, measured.height));
            return;
        }

        if (direction == Direction::Row) {
            MeasureRowFallback(resolvedWidth, availableHeight);
            return;
        }

        const Thickness scaledPadding = ScaleThickness(padding);
        const float scaledSpacing = ScaleValue(spacing);
        const float contentWidth = std::max(0.0f, resolvedWidth - scaledPadding.left - scaledPadding.right);
        float totalHeight = scaledPadding.top + scaledPadding.bottom;
        bool first = true;

        for (auto& child : m_children) {
            const Thickness margin = child->ScaleThickness(child->Margin());
            const float availableChildWidth = std::max(0.0f, contentWidth - margin.left - margin.right);
            const float childWidth = (alignItems == AlignItems::Stretch && !child->HasFixedWidth())
                ? availableChildWidth
                : child->GetPreferredWidth(availableChildWidth);
            child->Measure(childWidth, availableHeight);
            const float childHeight = child->DesiredSize().height;

            if (!first) {
                totalHeight += scaledSpacing;
            }
            totalHeight += margin.top + childHeight + margin.bottom;
            first = false;
        }

        m_desiredSize.width = resolvedWidth;
        m_desiredSize.height = HasFixedHeight()
            ? GetPreferredHeight(resolvedWidth)
            : ClampHeight(totalHeight);
    }

protected:
    StackLayoutStyle ToStackLayoutStyle() const {
        const Thickness scaledPadding = ScaleThickness(padding);
        const Thickness scaledBorder = ScaleThickness(Border());
        return StackLayoutStyle{
            direction == Direction::Row ? StackDirection::Row : StackDirection::Column,
            wrap == Wrap::Wrap ? StackWrap::Wrap : StackWrap::NoWrap,
            LayoutSpacing{scaledPadding.left, scaledPadding.top, scaledPadding.right, scaledPadding.bottom},
            LayoutSpacing{scaledBorder.left, scaledBorder.top, scaledBorder.right, scaledBorder.bottom},
            ScaleValue(spacing),
            static_cast<StackAlignItems>(alignItems),
            static_cast<StackJustifyContent>(justifyContent)
        };
    }

    std::vector<StackLayoutChild> BuildLayoutChildren() const {
        std::vector<StackLayoutChild> children;
        children.reserve(m_children.size());
        for (const auto& child : m_children) {
            const Thickness margin = child->ScaleThickness(child->Margin());
            const Thickness border = child->ScaleThickness(child->Border());
            children.push_back(StackLayoutChild{
                child.get(),
                LayoutSpacing{margin.left, margin.top, margin.right, margin.bottom},
                LayoutSpacing{border.left, border.top, border.right, border.bottom}
            });
        }
        return children;
    }

    float MeasurePreferredHeight(float width) const override {
        if (direction == Direction::Row) {
            float maxHeight = 0.0f;
            bool first = true;
            const Thickness scaledPadding = ScaleThickness(padding);
            const float scaledSpacing = ScaleValue(spacing);
            float totalWidth = scaledPadding.left + scaledPadding.right;
            const float contentHeight = 4096.0f;
            for (const auto& child : m_children) {
                if (!first) {
                    totalWidth += scaledSpacing;
                }
                const Thickness margin = child->ScaleThickness(child->Margin());
                const float preferredWidth = child->HasFixedWidth()
                    ? child->GetPreferredWidth(std::max(0.0f, width))
                    : child->GetPreferredWidth(std::max(0.0f, width));
                const float preferredHeight = child->GetPreferredHeight(preferredWidth);
                totalWidth += margin.left + preferredWidth + margin.right;
                maxHeight = std::max(maxHeight, margin.top + preferredHeight + margin.bottom);
                first = false;
            }
            (void)contentHeight;
            return scaledPadding.top + maxHeight + scaledPadding.bottom;
        }

        const Thickness scaledPadding = ScaleThickness(padding);
        const float scaledSpacing = ScaleValue(spacing);
        float totalHeight = scaledPadding.top + scaledPadding.bottom;
        const float contentWidth = std::max(0.0f, width - scaledPadding.left - scaledPadding.right);
        bool first = true;
        for (const auto& child : m_children) {
            if (!first) {
                totalHeight += scaledSpacing;
            }
            const Thickness margin = child->ScaleThickness(child->Margin());
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
        const Thickness scaledBorder = ScaleThickness(Border());
        const bool hasBorder =
            scaledBorder.left > 0.0f || scaledBorder.top > 0.0f ||
            scaledBorder.right > 0.0f || scaledBorder.bottom > 0.0f;
        if (background.a > 0.0f || hasBorder) {
            BoxDecoration deco;
            deco.background = background;
            deco.border.width = scaledBorder;
            deco.border.color = borderColor;
            deco.radius = CornerRadius::Uniform(ScaleValue(cornerRadius));
            DrawBoxDecoration(renderer, m_bounds, deco);
        }
        for (auto& child : m_children) {
            child->Render(renderer);
        }
    }

private:
    void MeasureRowFallback(float availableWidth, float availableHeight) {
        const Thickness scaledPadding = ScaleThickness(padding);
        const float scaledSpacing = ScaleValue(spacing);
        const float contentHeight = std::max(0.0f, availableHeight - scaledPadding.top - scaledPadding.bottom);
        float totalWidth = scaledPadding.left + scaledPadding.right;
        float maxHeight = 0.0f;
        bool first = true;

        for (auto& child : m_children) {
            const Thickness margin = child->ScaleThickness(child->Margin());
            const float availableChildHeight = std::max(0.0f, contentHeight - margin.top - margin.bottom);
            const float childWidth = child->GetPreferredWidth(std::max(0.0f, availableWidth));
            child->Measure(childWidth, availableChildHeight);
            const float childHeight = child->DesiredSize().height;

            if (!first) {
                totalWidth += scaledSpacing;
            }
            totalWidth += margin.left + childWidth + margin.right;
            maxHeight = std::max(maxHeight, margin.top + childHeight + margin.bottom);
            first = false;
        }

        m_desiredSize.width = totalWidth;
        m_desiredSize.height = scaledPadding.top + maxHeight + scaledPadding.bottom;
    }

    void ArrangeRowFallback() {
        const Thickness scaledPadding = ScaleThickness(padding);
        const float scaledSpacing = ScaleValue(spacing);
        const float contentLeft = m_bounds.left + scaledPadding.left;
        const float contentTop = m_bounds.top + scaledPadding.top;
        const float contentWidth = std::max(0.0f, m_bounds.Width() - scaledPadding.left - scaledPadding.right);
        const float contentHeight = std::max(0.0f, m_bounds.Height() - scaledPadding.top - scaledPadding.bottom);

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
            const Thickness margin = child->ScaleThickness(child->Margin());
            const float availableChildHeight = std::max(0.0f, contentHeight - margin.top - margin.bottom);
            const float childWidth = child->GetPreferredWidth(contentWidth);
            child->Measure(childWidth, availableChildHeight);
            const float childHeight = child->DesiredSize().height;

            if (!first) {
                totalWidth += scaledSpacing;
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
                x += scaledSpacing;
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

        const Thickness scaledPadding = ScaleThickness(padding);
        const float scaledCellSpacing = ScaleValue(cellSpacing);
        const float scaledRowHeight = ScaleValue(rowHeight);
        const float x0 = m_bounds.left + scaledPadding.left;
        float y = m_bounds.top + scaledPadding.top;
        const float contentWidth = m_bounds.Width() - scaledPadding.left - scaledPadding.right;
        const int cols = std::max(1, columns);
        const float cellWidth = (contentWidth - scaledCellSpacing * (cols - 1)) / cols;

        float x = x0;
        int col = 0;
        for (auto& child : m_children) {
            const Thickness margin = child->ScaleThickness(child->Margin());
            const float availableCellWidth = std::max(0.0f, cellWidth - margin.left - margin.right);
            const float childWidth = child->HasFixedWidth() ? child->GetPreferredWidth(availableCellWidth) : availableCellWidth;
            const float childHeight = std::min(
                std::max(0.0f, scaledRowHeight - margin.top - margin.bottom),
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
                y += scaledRowHeight + scaledCellSpacing;
            } else {
                x += cellWidth + scaledCellSpacing;
            }
        }
    }

    void Measure(float availableWidth, float availableHeight) override {
        const float resolvedWidth = GetPreferredWidth(availableWidth);
        const Thickness scaledPadding = ScaleThickness(padding);
        const float scaledCellSpacing = ScaleValue(cellSpacing);
        const float scaledRowHeight = ScaleValue(rowHeight);
        const float contentWidth = std::max(0.0f, resolvedWidth - scaledPadding.left - scaledPadding.right);
        const int cols = std::max(1, columns);
        const float cellWidth = (contentWidth - scaledCellSpacing * (cols - 1)) / cols;
        const int cellCount = static_cast<int>(m_children.size());
        const int rows = (cellCount + cols - 1) / cols;

        for (auto& child : m_children) {
            const Thickness margin = child->ScaleThickness(child->Margin());
            const float availableCellWidth = std::max(0.0f, cellWidth - margin.left - margin.right);
            const float childWidth = child->HasFixedWidth() ? child->GetPreferredWidth(availableCellWidth) : availableCellWidth;
            child->Measure(childWidth, availableHeight);
        }

        const float measuredHeight =
            scaledPadding.top + scaledPadding.bottom + rows * scaledRowHeight + std::max(0, rows - 1) * scaledCellSpacing;
        m_desiredSize.width = resolvedWidth;
        m_desiredSize.height = HasFixedHeight()
            ? GetPreferredHeight(resolvedWidth)
            : ClampHeight(measuredHeight);
    }

protected:
    float MeasurePreferredHeight(float width) const override {
        (void)width;
        const Thickness scaledPadding = ScaleThickness(padding);
        const float scaledCellSpacing = ScaleValue(cellSpacing);
        const float scaledRowHeight = ScaleValue(rowHeight);
        const int cols = std::max(1, columns);
        const int cellCount = static_cast<int>(m_children.size());
        const int rows = (cellCount + cols - 1) / cols;
        float totalHeight = scaledPadding.top + scaledPadding.bottom;
        if (rows > 0) {
            totalHeight += rows * scaledRowHeight;
            totalHeight += (rows - 1) * scaledCellSpacing;
        }
        return totalHeight;
    }

public:
    void Render(IRenderer& renderer) override {
        const float scaledCornerRadius = ScaleValue(cornerRadius);
        if (scaledCornerRadius > 0.0f) {
            renderer.FillRoundedRect(m_bounds, background, scaledCornerRadius);
            renderer.DrawRoundedRect(m_bounds, ColorFromHex(0x333333), 1.0f, scaledCornerRadius);
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
        const TextRenderOptions textOptions{
            TextWrapMode::Wrap,
            TextHorizontalAlign::Start,
            TextVerticalAlign::Start
        };
        renderer.DrawTextW(
            m_text.c_str(),
            static_cast<UINT32>(m_text.size()),
            m_bounds,
            m_color,
            ScaleValue(m_fontSize),
            textOptions
        );
    }

    float m_fontSize = 15.0f;
    Color m_color = ColorFromHex(0xEDEDED);

private:
    std::wstring m_text;
};

class StatCard : public UIElement {
public:
    void SetTitle(std::wstring text) { m_title = std::move(text); }
    void SetValue(std::wstring text) { m_value = std::move(text); }
    void SetDeltaText(std::wstring text) { m_deltaText = std::move(text); }

    Color background = ColorFromHex(0x1E2630);
    Color borderColor = ColorFromHex(0x2F3A46);
    Color accentColor = ColorFromHex(0x4E7BFF);
    Color titleColor = ColorFromHex(0xBFD1E3);
    Color valueColor = ColorFromHex(0xFFFFFF);
    Color deltaColor = ColorFromHex(0x8ED1A5);
    float cornerRadius = 10.0f;
    float titleFontSize = 12.0f;
    float valueFontSize = 24.0f;
    float deltaFontSize = 12.0f;

protected:
    float MeasurePreferredWidth(float availableWidth) const override {
        const float contentLimit = std::max(1.0f, availableWidth - ScaleValue(24.0f));
        const Size titleSize = MeasureTextValue(m_title.empty() ? L"Title" : m_title, titleFontSize, contentLimit, TextWrapMode::NoWrap);
        const Size valueSize = MeasureTextValue(m_value.empty() ? L"0" : m_value, valueFontSize, contentLimit, TextWrapMode::NoWrap);
        const Size deltaSize = MeasureTextValue(m_deltaText.empty() ? L"+0%" : m_deltaText, deltaFontSize, contentLimit, TextWrapMode::NoWrap);
        const float contentWidth = std::max(titleSize.width, std::max(valueSize.width, deltaSize.width));
        return std::min(availableWidth, contentWidth + ScaleValue(24.0f));
    }

    float MeasurePreferredHeight(float width) const override {
        const float contentWidth = std::max(1.0f, width - ScaleValue(24.0f));
        const Size titleSize = MeasureTextValue(m_title.empty() ? L"Title" : m_title, titleFontSize, contentWidth, TextWrapMode::NoWrap);
        const Size valueSize = MeasureTextValue(m_value.empty() ? L"0" : m_value, valueFontSize, contentWidth, TextWrapMode::NoWrap);
        const Size deltaSize = MeasureTextValue(m_deltaText.empty() ? L"+0%" : m_deltaText, deltaFontSize, contentWidth, TextWrapMode::NoWrap);
        return ScaleValue(16.0f) + titleSize.height + ScaleValue(6.0f) + valueSize.height + ScaleValue(6.0f) + deltaSize.height + ScaleValue(12.0f);
    }

public:
    void Render(IRenderer& renderer) override {
        const float scaledCornerRadius = ScaleValue(cornerRadius);
        renderer.FillRoundedRect(m_bounds, background, scaledCornerRadius);
        renderer.DrawRoundedRect(m_bounds, borderColor, 1.0f, scaledCornerRadius);

        const float accentHeight = ScaleValue(3.0f);
        const Rect accentRect = Rect::Make(m_bounds.left, m_bounds.top, m_bounds.right, m_bounds.top + accentHeight);
        renderer.FillRect(accentRect, accentColor);

        const float horizontalPadding = ScaleValue(12.0f);
        const float topPadding = ScaleValue(10.0f);
        const float lineGap = ScaleValue(6.0f);

        Rect titleRect = Rect::Make(
            m_bounds.left + horizontalPadding,
            m_bounds.top + topPadding,
            m_bounds.right - horizontalPadding,
            m_bounds.top + topPadding + ScaleValue(22.0f));
        const TextRenderOptions titleTextOptions{
            TextWrapMode::NoWrap,
            TextHorizontalAlign::Start,
            TextVerticalAlign::Start
        };
        renderer.DrawTextW(
            m_title.c_str(),
            static_cast<UINT32>(m_title.size()),
            titleRect,
            titleColor,
            ScaleValue(titleFontSize),
            titleTextOptions);

        const Size titleSize = MeasureTextValue(
            m_title.empty() ? L"Title" : m_title,
            titleFontSize,
            std::max(1.0f, titleRect.Width()),
            TextWrapMode::NoWrap);
        const float valueTop = titleRect.top + titleSize.height + lineGap;
        Rect valueRect = Rect::Make(
            m_bounds.left + horizontalPadding,
            valueTop,
            m_bounds.right - horizontalPadding,
            valueTop + ScaleValue(38.0f));
        renderer.DrawTextW(
            m_value.c_str(),
            static_cast<UINT32>(m_value.size()),
            valueRect,
            valueColor,
            ScaleValue(valueFontSize),
            titleTextOptions);

        const Size valueSize = MeasureTextValue(
            m_value.empty() ? L"0" : m_value,
            valueFontSize,
            std::max(1.0f, valueRect.Width()),
            TextWrapMode::NoWrap);
        const float deltaTop = valueRect.top + valueSize.height + lineGap;
        Rect deltaRect = Rect::Make(
            m_bounds.left + horizontalPadding,
            deltaTop,
            m_bounds.right - horizontalPadding,
            deltaTop + ScaleValue(20.0f));
        renderer.DrawTextW(
            m_deltaText.c_str(),
            static_cast<UINT32>(m_deltaText.size()),
            deltaRect,
            deltaColor,
            ScaleValue(deltaFontSize),
            titleTextOptions);
    }

private:
    std::wstring m_title = L"Metric";
    std::wstring m_value = L"0";
    std::wstring m_deltaText = L"+0%";
};

class SparklineChart : public UIElement {
public:
    void SetPoints(std::vector<float> points) { m_points = std::move(points); }
    void SetRange(float minValue, float maxValue) {
        m_manualRange = true;
        m_minValue = minValue;
        m_maxValue = maxValue;
    }
    void ClearRange() {
        m_manualRange = false;
        m_minValue = 0.0f;
        m_maxValue = 1.0f;
    }

    Color background = ColorFromHex(0x121B25);
    Color borderColor = ColorFromHex(0x2C3A47);
    Color lineColor = ColorFromHex(0x53B3FF);
    Color baselineColor = ColorFromHex(0x314252);
    float cornerRadius = 10.0f;
    float strokeWidth = 2.0f;

protected:
    float MeasurePreferredWidth(float availableWidth) const override {
        return std::min(availableWidth, ScaleValue(240.0f));
    }

    float MeasurePreferredHeight(float width) const override {
        (void)width;
        return ScaleValue(108.0f);
    }

public:
    void Render(IRenderer& renderer) override {
        const float scaledCornerRadius = ScaleValue(cornerRadius);
        renderer.FillRoundedRect(m_bounds, background, scaledCornerRadius);
        renderer.DrawRoundedRect(m_bounds, borderColor, 1.0f, scaledCornerRadius);

        const float insetX = ScaleValue(10.0f);
        const float insetY = ScaleValue(10.0f);
        const Rect chartRect = Rect::Make(
            m_bounds.left + insetX,
            m_bounds.top + insetY,
            m_bounds.right - insetX,
            m_bounds.bottom - insetY);

        renderer.DrawLine(
            PointF{chartRect.left, chartRect.bottom},
            PointF{chartRect.right, chartRect.bottom},
            baselineColor,
            1.0f);

        if (m_points.size() < 2 || chartRect.Width() <= 1.0f || chartRect.Height() <= 1.0f) {
            return;
        }

        float minValue = m_manualRange ? m_minValue : std::numeric_limits<float>::max();
        float maxValue = m_manualRange ? m_maxValue : std::numeric_limits<float>::lowest();
        if (!m_manualRange) {
            for (float point : m_points) {
                minValue = std::min(minValue, point);
                maxValue = std::max(maxValue, point);
            }
        }
        if (!std::isfinite(minValue) || !std::isfinite(maxValue)) {
            return;
        }
        if (maxValue < minValue) {
            std::swap(maxValue, minValue);
        }

        const float range = std::max(0.001f, maxValue - minValue);
        const float stepX = m_points.size() > 1
            ? chartRect.Width() / static_cast<float>(m_points.size() - 1)
            : 0.0f;

        std::vector<PointF> polyline;
        polyline.reserve(m_points.size());
        for (size_t i = 0; i < m_points.size(); ++i) {
            const float normalized = std::clamp((m_points[i] - minValue) / range, 0.0f, 1.0f);
            const float x = chartRect.left + static_cast<float>(i) * stepX;
            const float y = chartRect.bottom - normalized * chartRect.Height();
            polyline.push_back(PointF{x, y});
        }

        renderer.DrawPolyline(polyline, lineColor, ScaleValue(strokeWidth));
    }

private:
    std::vector<float> m_points;
    bool m_manualRange = false;
    float m_minValue = 0.0f;
    float m_maxValue = 1.0f;
};

class DataTable : public UIElement {
public:
    struct Column {
        std::wstring title;
        float width = -1.0f;
    };

    void SetColumns(std::vector<Column> columns) { m_columns = std::move(columns); }
    void SetRows(std::vector<std::vector<std::wstring>> rows) { m_rows = std::move(rows); }
    void SetColumnWidths(std::vector<float> widths) {
        const size_t count = std::min(widths.size(), m_columns.size());
        for (size_t i = 0; i < count; ++i) {
            m_columns[i].width = widths[i];
        }
    }

    Color background = ColorFromHex(0x121A24);
    Color borderColor = ColorFromHex(0x304053);
    Color headerBackground = ColorFromHex(0x1F2C3A);
    Color rowBackgroundA = ColorFromHex(0x182330);
    Color rowBackgroundB = ColorFromHex(0x14202B);
    Color gridLineColor = ColorFromHex(0x2B3A4A);
    Color headerTextColor = ColorFromHex(0xEAF2FB);
    Color textColor = ColorFromHex(0xC7D4E2);
    float cornerRadius = 10.0f;
    float headerHeight = 34.0f;
    float rowHeight = 30.0f;
    float fontSize = 13.0f;
    float headerFontSize = 13.0f;
    Thickness cellPadding{8, 5, 8, 5};

protected:
    float MeasurePreferredWidth(float availableWidth) const override {
        const float fallbackWidth = std::min(availableWidth, ScaleValue(360.0f));
        if (m_columns.empty()) {
            return fallbackWidth;
        }

        const Thickness scaledCellPadding = ScaleThickness(cellPadding);
        float totalWidth = 0.0f;
        for (size_t col = 0; col < m_columns.size(); ++col) {
            float columnWidth = 0.0f;
            if (m_columns[col].width > 0.0f) {
                columnWidth = ScaleValue(m_columns[col].width);
            } else {
                const Size headerSize = MeasureTextValue(
                    m_columns[col].title.empty() ? L"Column" : m_columns[col].title,
                    headerFontSize,
                    4096.0f,
                    TextWrapMode::NoWrap);
                columnWidth = headerSize.width + scaledCellPadding.left + scaledCellPadding.right + ScaleValue(10.0f);

                const size_t inspectRows = std::min<size_t>(m_rows.size(), 50);
                for (size_t row = 0; row < inspectRows; ++row) {
                    if (col >= m_rows[row].size()) {
                        continue;
                    }
                    const Size cellSize = MeasureTextValue(
                        m_rows[row][col],
                        fontSize,
                        4096.0f,
                        TextWrapMode::NoWrap);
                    columnWidth = std::max(columnWidth, cellSize.width + scaledCellPadding.left + scaledCellPadding.right + ScaleValue(10.0f));
                }
            }
            totalWidth += std::max(ScaleValue(36.0f), columnWidth);
        }
        return std::min(availableWidth, totalWidth + ScaleValue(2.0f));
    }

    float MeasurePreferredHeight(float width) const override {
        (void)width;
        const float scaledHeaderHeight = ScaleValue(headerHeight);
        const float scaledRowHeight = ScaleValue(rowHeight);
        const float rowsHeight = scaledRowHeight * static_cast<float>(m_rows.size());
        return scaledHeaderHeight + rowsHeight + ScaleValue(2.0f);
    }

public:
    void Render(IRenderer& renderer) override {
        const float scaledCornerRadius = ScaleValue(cornerRadius);
        renderer.FillRoundedRect(m_bounds, background, scaledCornerRadius);
        renderer.DrawRoundedRect(m_bounds, borderColor, 1.0f, scaledCornerRadius);

        if (m_columns.empty()) {
            const TextRenderOptions textOptions{
                TextWrapMode::NoWrap,
                TextHorizontalAlign::Center,
                TextVerticalAlign::Center
            };
            const std::wstring message = L"No columns";
            renderer.DrawTextW(
                message.c_str(),
                static_cast<UINT32>(message.size()),
                m_bounds,
                textColor,
                ScaleValue(fontSize),
                textOptions);
            return;
        }

        const std::vector<float> columnWidths = ResolveColumnWidths();
        if (columnWidths.empty()) {
            return;
        }

        renderer.PushRoundedClip(m_bounds, scaledCornerRadius);

        const float scaledHeaderHeight = ScaleValue(headerHeight);
        const float scaledRowHeight = ScaleValue(rowHeight);
        const Thickness scaledCellPadding = ScaleThickness(cellPadding);
        const float tableTop = m_bounds.top;
        const float tableBottom = m_bounds.bottom;
        const float contentHeight = std::max(0.0f, tableBottom - tableTop);
        const float bodyTop = tableTop + scaledHeaderHeight;

        const Rect headerRect = Rect::Make(m_bounds.left, tableTop, m_bounds.right, std::min(tableBottom, tableTop + scaledHeaderHeight));
        renderer.FillRect(headerRect, headerBackground);
        renderer.DrawLine(
            PointF{m_bounds.left, bodyTop},
            PointF{m_bounds.right, bodyTop},
            gridLineColor,
            1.0f);

        float currentX = m_bounds.left;
        for (size_t col = 0; col < m_columns.size(); ++col) {
            const float cellWidth = columnWidths[col];
            const Rect headerCellRect = Rect::Make(
                currentX + scaledCellPadding.left,
                tableTop + scaledCellPadding.top,
                currentX + cellWidth - scaledCellPadding.right,
                tableTop + scaledHeaderHeight - scaledCellPadding.bottom);
            const TextRenderOptions headerTextOptions{
                TextWrapMode::NoWrap,
                TextHorizontalAlign::Start,
                TextVerticalAlign::Center
            };
            renderer.DrawTextW(
                m_columns[col].title.c_str(),
                static_cast<UINT32>(m_columns[col].title.size()),
                headerCellRect,
                headerTextColor,
                ScaleValue(headerFontSize),
                headerTextOptions);

            if (col + 1 < m_columns.size()) {
                const float x = currentX + cellWidth;
                renderer.DrawLine(
                    PointF{x, tableTop},
                    PointF{x, tableBottom},
                    gridLineColor,
                    1.0f);
            }
            currentX += cellWidth;
        }

        if (contentHeight > scaledHeaderHeight + 1.0f && !m_rows.empty()) {
            const float availableBodyHeight = std::max(0.0f, contentHeight - scaledHeaderHeight);
            const size_t visibleRows = static_cast<size_t>(std::floor(availableBodyHeight / std::max(1.0f, scaledRowHeight)));
            const size_t rowCount = std::min(m_rows.size(), visibleRows);

            for (size_t row = 0; row < rowCount; ++row) {
                const float rowTop = bodyTop + static_cast<float>(row) * scaledRowHeight;
                const float rowBottom = std::min(tableBottom, rowTop + scaledRowHeight);
                if (rowTop >= tableBottom) {
                    break;
                }
                const Rect rowRect = Rect::Make(m_bounds.left, rowTop, m_bounds.right, rowBottom);
                renderer.FillRect(rowRect, (row % 2 == 0) ? rowBackgroundA : rowBackgroundB);

                float rowX = m_bounds.left;
                for (size_t col = 0; col < m_columns.size(); ++col) {
                    const float cellWidth = columnWidths[col];
                    const std::wstring cellText =
                        (col < m_rows[row].size()) ? m_rows[row][col] : L"";
                    const Rect textRect = Rect::Make(
                        rowX + scaledCellPadding.left,
                        rowTop + scaledCellPadding.top,
                        rowX + cellWidth - scaledCellPadding.right,
                        rowBottom - scaledCellPadding.bottom);
                    const TextRenderOptions cellTextOptions{
                        TextWrapMode::NoWrap,
                        TextHorizontalAlign::Start,
                        TextVerticalAlign::Center
                    };
                    renderer.DrawTextW(
                        cellText.c_str(),
                        static_cast<UINT32>(cellText.size()),
                        textRect,
                        textColor,
                        ScaleValue(fontSize),
                        cellTextOptions);
                    rowX += cellWidth;
                }

                renderer.DrawLine(
                    PointF{m_bounds.left, rowBottom},
                    PointF{m_bounds.right, rowBottom},
                    gridLineColor,
                    1.0f);
            }
        }

        renderer.PopLayer();
    }

private:
    std::vector<float> ResolveColumnWidths() const {
        std::vector<float> widths;
        if (m_columns.empty()) {
            return widths;
        }
        widths.resize(m_columns.size(), 0.0f);

        const float tableWidth = std::max(1.0f, m_bounds.Width());
        float fixedTotal = 0.0f;
        size_t autoCount = 0;
        for (size_t i = 0; i < m_columns.size(); ++i) {
            if (m_columns[i].width > 0.0f) {
                widths[i] = ScaleValue(m_columns[i].width);
                fixedTotal += widths[i];
            } else {
                ++autoCount;
            }
        }

        float autoWidth = 0.0f;
        if (autoCount > 0) {
            const float remaining = std::max(0.0f, tableWidth - fixedTotal);
            autoWidth = (remaining > 0.0f ? remaining : tableWidth) / static_cast<float>(autoCount);
        }

        float totalWidth = 0.0f;
        for (size_t i = 0; i < widths.size(); ++i) {
            if (widths[i] <= 0.0f) {
                widths[i] = autoWidth;
            }
            widths[i] = std::max(ScaleValue(36.0f), widths[i]);
            totalWidth += widths[i];
        }

        if (totalWidth > tableWidth && totalWidth > 0.0f) {
            const float ratio = tableWidth / totalWidth;
            for (float& width : widths) {
                width = std::max(ScaleValue(28.0f), width * ratio);
            }
        }

        return widths;
    }

    std::vector<Column> m_columns;
    std::vector<std::vector<std::wstring>> m_rows;
};

class SeagullAnimation : public UIElement {
public:
    void SetCount(int count) { m_count = std::max(1, count); }
    void SetSpeed(float speed) { m_speed = std::max(0.05f, speed); }
    void SetWingAmplitude(float amplitude) { m_wingAmplitude = std::max(0.0f, amplitude); }
    void SetPathHeight(float pathHeight) { m_pathHeight = std::max(0.0f, pathHeight); }
    void SetScale(float scale) { m_scale = std::max(0.1f, scale); }
    void SetOpacity(float opacity) { m_opacity = std::clamp(opacity, 0.05f, 1.0f); }

    Color background = ColorFromHex(0x0F1A26);
    Color borderColor = ColorFromHex(0x2B3C4D);
    Color birdColor = ColorFromHex(0xDDEFFF);
    float cornerRadius = 12.0f;

protected:
    float MeasurePreferredWidth(float availableWidth) const override {
        return std::min(availableWidth, ScaleValue(340.0f));
    }

    float MeasurePreferredHeight(float width) const override {
        (void)width;
        return ScaleValue(180.0f);
    }

public:
    bool OnTimer(UINT_PTR timerId) override {
        const bool childChanged = UIElement::OnTimer(timerId);
        m_elapsed += 0.016f;
        return childChanged || true;
    }

    void Render(IRenderer& renderer) override {
        const float scaledCornerRadius = ScaleValue(cornerRadius);
        renderer.FillRoundedRect(m_bounds, background, scaledCornerRadius);
        renderer.DrawRoundedRect(m_bounds, borderColor, 1.0f, scaledCornerRadius);

        renderer.PushRoundedClip(m_bounds, scaledCornerRadius);

        const float width = std::max(1.0f, m_bounds.Width());
        const float height = std::max(1.0f, m_bounds.Height());
        const float left = m_bounds.left + ScaleValue(10.0f);
        const float right = m_bounds.right - ScaleValue(10.0f);
        const float top = m_bounds.top + ScaleValue(14.0f);
        const float usableWidth = std::max(1.0f, right - left);
        const float amplitudeY = std::min(ScaleValue(m_pathHeight), height * 0.35f);
        const float wingAmplitudePx = ScaleValue(m_wingAmplitude);

        for (int i = 0; i < std::max(1, m_count); ++i) {
            const float phase = m_elapsed * m_speed + static_cast<float>(i) * 0.85f;
            const float t = phase - std::floor(phase);
            const float x = left + t * usableWidth;
            const float centerY = top + height * 0.35f + std::sin(phase * 2.0f + static_cast<float>(i) * 0.7f) * amplitudeY;

            const float scale = ScaleValue(m_scale) * (0.9f + 0.15f * std::sin(phase * 1.7f + static_cast<float>(i)));
            const float wingSpan = 16.0f * scale;
            const float wingLift = (4.0f + std::sin(phase * 12.0f) * wingAmplitudePx) * scale;

            Color color = birdColor;
            color.a *= m_opacity * (0.68f + 0.32f * std::clamp(t, 0.0f, 1.0f));

            const PointF center{x, centerY};
            const PointF leftWing{x - wingSpan, centerY - wingLift};
            const PointF rightWing{x + wingSpan, centerY - wingLift};
            const PointF tailLeft{x - ScaleValue(2.0f), centerY + ScaleValue(1.0f)};
            const PointF tailRight{x + ScaleValue(2.0f), centerY + ScaleValue(1.0f)};

            renderer.DrawLine(center, leftWing, color, ScaleValue(2.0f));
            renderer.DrawLine(center, rightWing, color, ScaleValue(2.0f));
            renderer.DrawLine(tailLeft, tailRight, color, ScaleValue(1.5f));
        }

        renderer.PopLayer();
    }

private:
    int m_count = 5;
    float m_speed = 0.8f;
    float m_wingAmplitude = 3.5f;
    float m_pathHeight = 20.0f;
    float m_scale = 1.0f;
    float m_opacity = 0.85f;
    float m_elapsed = 0.0f;
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
    explicit Button(std::wstring text) : m_text(std::move(text)) {
        m_style = DefaultStyle();
    }

    void SetOnClick(std::function<void()> onClick) { m_onClick = std::move(onClick); }
    void SetText(std::wstring text) { m_text = std::move(text); }

    float cornerRadius = 4.0f;
    float fontSize = 14.0f;
    Color background = ColorFromHex(0x2D2D30);
    Color foreground = ColorFromHex(0xFFFFFF);

    static ComponentStyle DefaultStyle() {
        ComponentStyle s;
        BoxDecoration normalDeco;
        normalDeco.background = ColorFromHex(0x2D2D30);
        normalDeco.border.width = Thickness{1.0f, 1.0f, 1.0f, 1.0f};
        normalDeco.border.color = ColorFromHex(0x6A6A6A);
        normalDeco.radius = CornerRadius::Uniform(4.0f);
        s.base.decoration = normalDeco;
        s.base.foreground = ColorFromHex(0xFFFFFF);

        BoxDecoration hoverDeco = normalDeco;
        hoverDeco.background = ColorFromHex(0x3E3E42);
        s.overrides[static_cast<std::size_t>(StyleState::Hover)].decoration = hoverDeco;

        BoxDecoration pressedDeco = normalDeco;
        pressedDeco.background = ColorFromHex(0x0E639C);
        s.overrides[static_cast<std::size_t>(StyleState::Pressed)].decoration = pressedDeco;

        BoxDecoration focusedDeco = normalDeco;
        focusedDeco.border.width = Thickness{2.0f, 2.0f, 2.0f, 2.0f};
        focusedDeco.border.color = ColorFromHex(0xFFFFFF);
        s.overrides[static_cast<std::size_t>(StyleState::Focused)].decoration = focusedDeco;

        BoxDecoration disabledDeco = normalDeco;
        disabledDeco.background = ColorFromHex(0x1F1F1F);
        disabledDeco.border.color = ColorFromHex(0x3A3A3A);
        s.overrides[static_cast<std::size_t>(StyleState::Disabled)].decoration = disabledDeco;
        s.overrides[static_cast<std::size_t>(StyleState::Disabled)].opacity = 0.6f;

        return s;
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
        const Size measured = MeasureTextValue(
            m_text,
            fontSize,
            availableWidth > 0.0f ? availableWidth : 4096.0f,
            TextWrapMode::NoWrap);
        return std::min(availableWidth, measured.width + ScaleValue(24.0f));
    }

    float MeasurePreferredHeight(float width) const override {
        const Size measured = MeasureTextValue(
            m_text,
            fontSize,
            width > 0.0f ? width : 4096.0f,
            TextWrapMode::NoWrap);
        return measured.height + ScaleValue(14.0f);
    }

public:
    void Render(IRenderer& renderer) override {
        SyncQuickSetToStyle();
        const StyleSpec resolved = m_style.Resolve(GetCurrentState());
        BoxDecoration deco = resolved.decoration.value_or(BoxDecoration{});
        deco.radius = CornerRadius::Uniform(ScaleValue(cornerRadius));
        deco.border.width = Thickness{
            ScaleValue(deco.border.width.left),
            ScaleValue(deco.border.width.top),
            ScaleValue(deco.border.width.right),
            ScaleValue(deco.border.width.bottom)
        };
        if (resolved.opacity.has_value()) {
            deco.opacity = *resolved.opacity;
        }
        DrawBoxDecoration(renderer, m_bounds, deco);

        Rect textRect = m_bounds;
        textRect.left += ScaleValue(10.0f);
        const TextRenderOptions textOptions{
            TextWrapMode::NoWrap,
            TextHorizontalAlign::Center,
            TextVerticalAlign::Center
        };
        const Color fg = resolved.foreground.value_or(foreground);
        const float fs = resolved.fontSize.value_or(fontSize);
        renderer.DrawTextW(
            m_text.c_str(),
            static_cast<UINT32>(m_text.size()),
            textRect,
            fg,
            ScaleValue(fs),
            textOptions
        );
    }

private:
    void SyncQuickSetToStyle() {
        if (!m_style.base.decoration.has_value()) {
            m_style.base.decoration = BoxDecoration{};
        }
        m_style.base.decoration->background = background;
        m_style.base.foreground = foreground;
        m_style.base.fontSize  = fontSize;
    }

public:

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
    std::function<void()> m_onClick;
};

class TextInput : public UIElement {
public:
    explicit TextInput(std::wstring text = L"") : m_text(std::move(text)), m_caretPosition(m_text.size()) {
        m_style = DefaultStyle();
    }

    static ComponentStyle DefaultStyle() {
        ComponentStyle s;
        BoxDecoration normalDeco;
        normalDeco.background = ColorFromHex(0x1F1F1F);
        normalDeco.border.width = Thickness{1.0f, 1.0f, 1.0f, 1.0f};
        normalDeco.border.color = ColorFromHex(0x6A6A6A);
        normalDeco.radius = CornerRadius::Uniform(4.0f);
        s.base.decoration = normalDeco;
        s.base.foreground = ColorFromHex(0xEDEDED);

        BoxDecoration focusedDeco = normalDeco;
        focusedDeco.border.width = Thickness{2.0f, 2.0f, 2.0f, 2.0f};
        focusedDeco.border.color = ColorFromHex(0xFFFFFF);
        s.overrides[static_cast<std::size_t>(StyleState::Focused)].decoration = focusedDeco;

        BoxDecoration disabledDeco = normalDeco;
        disabledDeco.background = ColorFromHex(0x171717);
        disabledDeco.border.color = ColorFromHex(0x3A3A3A);
        s.overrides[static_cast<std::size_t>(StyleState::Disabled)].decoration = disabledDeco;
        s.overrides[static_cast<std::size_t>(StyleState::Disabled)].opacity = 0.6f;

        return s;
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
        const float horizontalInset = ScaleValue(8.0f);
        const float verticalInset = ScaleValue(6.0f);
        const Rect textRect = Rect::Make(m_bounds.left + horizontalInset, m_bounds.top + verticalInset, m_bounds.right - horizontalInset, m_bounds.bottom - verticalInset);
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

        const float horizontalInset = ScaleValue(8.0f);
        const float verticalInset = ScaleValue(6.0f);
        const Rect textRect = Rect::Make(m_bounds.left + horizontalInset, m_bounds.top + verticalInset, m_bounds.right - horizontalInset, m_bounds.bottom - verticalInset);
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
            const Size metrics = MeasureTextValue(
                prefix.empty() ? L" " : prefix,
                fontSize,
                std::max(1.0f, textRect.right - textRect.left),
                TextWrapMode::NoWrap);
            if (localX < metrics.width + ScaleValue(4.0f)) {
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
        const float inset = ScaleValue(16.0f);
        const Size measured = MeasureTextValue(
            measureText,
            fontSize,
            std::max(1.0f, availableWidth - inset),
            TextWrapMode::NoWrap);
        return std::min(availableWidth, measured.width + inset);
    }

    float MeasurePreferredHeight(float width) const override {
        const std::wstring measureText = m_text.empty() ? L" " : m_text;
        const float inset = ScaleValue(16.0f);
        const Size measured = MeasureTextValue(
            measureText,
            fontSize,
            std::max(1.0f, width - inset),
            TextWrapMode::NoWrap);
        return measured.height + ScaleValue(12.0f);
    }

public:
    void Render(IRenderer& renderer) override {
        SyncQuickSetToStyle();
        const StyleSpec resolved = m_style.Resolve(GetCurrentState());
        BoxDecoration deco = resolved.decoration.value_or(BoxDecoration{});
        deco.radius = CornerRadius::Uniform(ScaleValue(cornerRadius));
        deco.border.width = Thickness{
            ScaleValue(deco.border.width.left),
            ScaleValue(deco.border.width.top),
            ScaleValue(deco.border.width.right),
            ScaleValue(deco.border.width.bottom)
        };
        if (resolved.opacity.has_value()) {
            deco.opacity = *resolved.opacity;
        }
        DrawBoxDecoration(renderer, m_bounds, deco);

        Rect textRect = m_bounds;
        textRect.left += ScaleValue(8.0f);
        textRect.top += ScaleValue(6.0f);
        if (HasSelection()) {
            const auto [selStart, selEnd] = GetSelectionRange();
            std::wstring prefix = m_text.substr(0, selStart);
            std::wstring selection = m_text.substr(selStart, selEnd - selStart);
            const Size prefixMetrics = MeasureTextValue(
                prefix.empty() ? L" " : prefix,
                fontSize,
                std::max(1.0f, textRect.right - textRect.left),
                TextWrapMode::NoWrap);
            const Size selectionMetrics = MeasureTextValue(
                selection.empty() ? L" " : selection,
                fontSize,
                std::max(1.0f, textRect.right - textRect.left),
                TextWrapMode::NoWrap);
            Rect selectionRect = Rect::Make(
                textRect.left + prefixMetrics.width,
                textRect.top,
                textRect.left + prefixMetrics.width + selectionMetrics.width,
                textRect.top + selectionMetrics.height
            );
            renderer.FillRect(selectionRect, ColorFromHex(0x3A86FF));
        }
        const TextRenderOptions textOptions{
            TextWrapMode::NoWrap,
            TextHorizontalAlign::Start,
            TextVerticalAlign::Center
        };
        const Color fg = resolved.foreground.value_or(textColor);
        renderer.DrawTextW(
            m_text.c_str(),
            static_cast<UINT32>(m_text.size()),
            textRect,
            fg,
            ScaleValue(fontSize),
            textOptions);

        if (m_hasFocus) {
            const std::wstring prefix = m_text.substr(0, m_caretPosition);
            const Size caretMetrics = MeasureTextValue(
                prefix.empty() ? L" " : prefix,
                fontSize,
                std::max(1.0f, textRect.right - textRect.left),
                TextWrapMode::NoWrap);
            const float caretX = textRect.left + caretMetrics.width;
            const float caretTop = textRect.top;
            const float caretBottom = textRect.top + MeasureTextValue(
                L"|",
                fontSize,
                std::max(1.0f, textRect.right - textRect.left),
                TextWrapMode::NoWrap).height;
            if (m_showCaret) {
                renderer.DrawRect(Rect::Make(caretX, caretTop, caretX + ScaleValue(1.0f), caretBottom), fg, 1.0f);
            }
        }
    }

private:
    void SyncQuickSetToStyle() {
        if (!m_style.base.decoration.has_value()) {
            m_style.base.decoration = BoxDecoration{};
        }
        m_style.base.decoration->background = background;
        m_style.base.foreground = textColor;
        m_style.base.fontSize  = fontSize;
    }

public:

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
    explicit Checkbox(std::wstring text = L"") : m_text(std::move(text)) {
        m_style = DefaultStyle();
    }

    static ComponentStyle DefaultStyle() {
        ComponentStyle s;
        BoxDecoration normalDeco;
        normalDeco.background = ColorFromHex(0x2D2D30);
        normalDeco.border.width = Thickness{1.0f, 1.0f, 1.0f, 1.0f};
        normalDeco.border.color = ColorFromHex(0x6A6A6A);
        s.base.decoration = normalDeco;
        s.base.foreground = ColorFromHex(0xEDEDED);

        BoxDecoration focusedDeco = normalDeco;
        focusedDeco.border.width = Thickness{2.0f, 2.0f, 2.0f, 2.0f};
        focusedDeco.border.color = ColorFromHex(0xFFFFFF);
        s.overrides[static_cast<std::size_t>(StyleState::Focused)].decoration = focusedDeco;

        BoxDecoration disabledDeco = normalDeco;
        disabledDeco.background = ColorFromHex(0x1F1F1F);
        disabledDeco.border.color = ColorFromHex(0x3A3A3A);
        s.overrides[static_cast<std::size_t>(StyleState::Disabled)].decoration = disabledDeco;
        s.overrides[static_cast<std::size_t>(StyleState::Disabled)].opacity = 0.6f;
        return s;
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
            m_checked = !m_checked;
            return true;
        }
        return false;
    }

    float MeasurePreferredWidth(float availableWidth) const override {
        const float reserve = ScaleValue(30.0f);
        const Size textSize = MeasureTextValue(
            m_text.empty() ? L" " : m_text,
            fontSize,
            std::max(1.0f, availableWidth - reserve),
            TextWrapMode::NoWrap);
        return std::min(availableWidth, textSize.width + reserve);
    }

    float MeasurePreferredHeight(float width) const override {
        const float reserve = ScaleValue(30.0f);
        const Size textSize = MeasureTextValue(
            m_text.empty() ? L" " : m_text,
            fontSize,
            std::max(1.0f, width - reserve),
            TextWrapMode::NoWrap);
        return textSize.height + ScaleValue(10.0f);
    }

    void Render(IRenderer& renderer) override {
        SyncQuickSetToStyle();
        const StyleSpec resolved = m_style.Resolve(GetCurrentState());
        BoxDecoration deco = resolved.decoration.value_or(BoxDecoration{});
        deco.border.width = Thickness{
            ScaleValue(deco.border.width.left),
            ScaleValue(deco.border.width.top),
            ScaleValue(deco.border.width.right),
            ScaleValue(deco.border.width.bottom)
        };
        if (resolved.opacity.has_value()) {
            deco.opacity = *resolved.opacity;
        }

        const Rect box = Rect::Make(
            m_bounds.left,
            m_bounds.top + ScaleValue(2.0f),
            m_bounds.left + ScaleValue(18.0f),
            m_bounds.top + ScaleValue(20.0f));
        DrawBoxDecoration(renderer, box, deco);
        if (m_checked) {
            const float inset = ScaleValue(4.0f);
            const Rect mark = Rect::Make(box.left + inset, box.top + inset, box.right - inset, box.bottom - inset);
            renderer.FillRect(mark, ColorFromHex(0x2D6CDF));
        }

        Rect textRect = m_bounds;
        textRect.left += ScaleValue(26.0f);
        const TextRenderOptions textOptions{
            TextWrapMode::NoWrap,
            TextHorizontalAlign::Start,
            TextVerticalAlign::Center
        };
        const Color fg = resolved.foreground.value_or(textColor);
        renderer.DrawTextW(
            m_text.c_str(),
            static_cast<UINT32>(m_text.size()),
            textRect,
            fg,
            ScaleValue(fontSize),
            textOptions);
    }

private:
    void SyncQuickSetToStyle() {
        if (!m_style.base.decoration.has_value()) {
            m_style.base.decoration = BoxDecoration{};
        }
        m_style.base.foreground = textColor;
    }

public:

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
    Color textColor = ColorFromHex(0xEDEDED);
    float fontSize = 14.0f;
};

class RadioButton : public UIElement {
public:
    explicit RadioButton(std::wstring text = L"", std::wstring group = L"default")
        : m_text(std::move(text)), m_group(std::move(group)) {
        s_groups[m_group].push_back(this);
        m_style = DefaultStyle();
    }

    static ComponentStyle DefaultStyle() {
        ComponentStyle s;
        BoxDecoration normalDeco;
        normalDeco.background = ColorFromHex(0x000000, 0.0f);  // transparent
        normalDeco.border.width = Thickness{1.0f, 1.0f, 1.0f, 1.0f};
        normalDeco.border.color = ColorFromHex(0x6A6A6A);
        normalDeco.radius = CornerRadius::Uniform(9.0f);
        s.base.decoration = normalDeco;
        s.base.foreground = ColorFromHex(0xEDEDED);

        BoxDecoration focusedDeco = normalDeco;
        focusedDeco.border.width = Thickness{2.0f, 2.0f, 2.0f, 2.0f};
        focusedDeco.border.color = ColorFromHex(0xFFFFFF);
        s.overrides[static_cast<std::size_t>(StyleState::Focused)].decoration = focusedDeco;

        BoxDecoration disabledDeco = normalDeco;
        disabledDeco.border.color = ColorFromHex(0x3A3A3A);
        s.overrides[static_cast<std::size_t>(StyleState::Disabled)].decoration = disabledDeco;
        s.overrides[static_cast<std::size_t>(StyleState::Disabled)].opacity = 0.6f;
        return s;
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
        const float reserve = ScaleValue(28.0f);
        const Size textSize = MeasureTextValue(
            m_text.empty() ? L" " : m_text,
            fontSize,
            std::max(1.0f, availableWidth - reserve),
            TextWrapMode::NoWrap);
        return std::min(availableWidth, textSize.width + reserve);
    }

    float MeasurePreferredHeight(float width) const override {
        const float reserve = ScaleValue(28.0f);
        const Size textSize = MeasureTextValue(
            m_text.empty() ? L" " : m_text,
            fontSize,
            std::max(1.0f, width - reserve),
            TextWrapMode::NoWrap);
        return textSize.height + ScaleValue(10.0f);
    }

    void Render(IRenderer& renderer) override {
        SyncQuickSetToStyle();
        const StyleSpec resolved = m_style.Resolve(GetCurrentState());
        BoxDecoration deco = resolved.decoration.value_or(BoxDecoration{});
        deco.radius = CornerRadius::Uniform(ScaleValue(deco.radius.MaxRadius()));
        deco.border.width = Thickness{
            ScaleValue(deco.border.width.left),
            ScaleValue(deco.border.width.top),
            ScaleValue(deco.border.width.right),
            ScaleValue(deco.border.width.bottom)
        };
        if (resolved.opacity.has_value()) {
            deco.opacity = *resolved.opacity;
        }

        const Rect circle = Rect::Make(
            m_bounds.left,
            m_bounds.top + ScaleValue(2.0f),
            m_bounds.left + ScaleValue(18.0f),
            m_bounds.top + ScaleValue(20.0f));
        DrawBoxDecoration(renderer, circle, deco);
        if (m_checked) {
            const float inset = ScaleValue(5.0f);
            const Rect dot = Rect::Make(circle.left + inset, circle.top + inset, circle.right - inset, circle.bottom - inset);
            renderer.FillRoundedRect(dot, ColorFromHex(0x2D6CDF), ScaleValue(5.0f));
        }

        Rect textRect = m_bounds;
        textRect.left += ScaleValue(26.0f);
        const TextRenderOptions textOptions{
            TextWrapMode::NoWrap,
            TextHorizontalAlign::Start,
            TextVerticalAlign::Center
        };
        const Color fg = resolved.foreground.value_or(textColor);
        renderer.DrawTextW(
            m_text.c_str(),
            static_cast<UINT32>(m_text.size()),
            textRect,
            fg,
            ScaleValue(fontSize),
            textOptions);
    }

private:
    void SyncQuickSetToStyle() {
        if (!m_style.base.decoration.has_value()) {
            m_style.base.decoration = BoxDecoration{};
        }
        m_style.base.foreground = textColor;
    }

public:

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
        return std::min(availableWidth, ScaleValue(220.0f));
    }

    float MeasurePreferredHeight(float width) const override {
        (void)width;
        return ScaleValue(42.0f);
    }

    void Render(IRenderer& renderer) override {
        const float scaledCornerRadius = ScaleValue(cornerRadius);
        renderer.FillRoundedRect(m_bounds, background, scaledCornerRadius);
        renderer.DrawRoundedRect(m_bounds, borderColor, 1.0f, scaledCornerRadius);
        if (m_hasFocus) {
            renderer.DrawRoundedRect(m_bounds, focusBorderColor, 2.0f, scaledCornerRadius);
        }

        const float trackLeft = m_bounds.left + ScaleValue(14.0f);
        const float trackRight = m_bounds.right - ScaleValue(14.0f);
        const float trackTop = m_bounds.top + (m_bounds.bottom - m_bounds.top) * 0.5f - ScaleValue(4.0f);
        const float trackBottom = trackTop + ScaleValue(8.0f);
        const Rect trackRect = Rect::Make(trackLeft, trackTop, trackRight, trackBottom);
        renderer.FillRect(trackRect, ColorFromHex(0x333333));

        const float position = trackLeft + (trackRight - trackLeft) * ((m_value - m_min) / std::max(1.0f, m_max - m_min));
        const Rect thumbRect = Rect::Make(position - ScaleValue(8.0f), trackTop - ScaleValue(6.0f), position + ScaleValue(8.0f), trackBottom + ScaleValue(6.0f));
        renderer.FillRoundedRect(thumbRect, highlightColor, ScaleValue(8.0f));
        renderer.DrawRoundedRect(thumbRect, ColorFromHex(0x6A6A6A), 1.0f, ScaleValue(8.0f));

        if (!m_label.empty()) {
            Rect labelRect = m_bounds;
            labelRect.right = trackLeft - ScaleValue(8.0f);
            const TextRenderOptions textOptions{
                TextWrapMode::NoWrap,
                TextHorizontalAlign::Start,
                TextVerticalAlign::Center
            };
            renderer.DrawTextW(
                m_label.c_str(),
                static_cast<UINT32>(m_label.size()),
                labelRect,
                textColor,
                ScaleValue(fontSize),
                textOptions);
        }
    }

    void SetText(std::wstring text) { m_label = std::move(text); }
    void SetRange(float minValue, float maxValue) { m_min = minValue; m_max = maxValue; SetValue(m_value); }
    void SetValue(float value) { m_value = std::clamp(value, m_min, m_max); }
    void SetStep(float step) { m_step = std::max(0.001f, step); }

private:
    void UpdateValueFromPoint(float x) {
        const float trackLeft = m_bounds.left + ScaleValue(14.0f);
        const float trackRight = m_bounds.right - ScaleValue(14.0f);
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

class ProgressBar : public UIElement {
public:
    explicit ProgressBar(std::wstring label = L"") : m_label(std::move(label)) {}

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
            m_dragging = false;
            return true;
        }
        return false;
    }

    bool OnKeyDown(WPARAM keyCode, LPARAM /*lParam*/) override {
        if (keyCode == VK_LEFT || keyCode == VK_DOWN) {
            return SetValueInternal(m_value - m_step);
        }
        if (keyCode == VK_RIGHT || keyCode == VK_UP) {
            return SetValueInternal(m_value + m_step);
        }
        if (keyCode == VK_HOME) {
            return SetValueInternal(m_min);
        }
        if (keyCode == VK_END) {
            return SetValueInternal(m_max);
        }
        return false;
    }

    bool OnMouseDown(float x, float y) override {
        if (!HitTest(x, y)) {
            return false;
        }
        m_dragging = true;
        return UpdateValueFromPoint(x);
    }

    bool OnMouseMove(float x, float y) override {
        if (!m_dragging) {
            return false;
        }
        return UpdateValueFromPoint(x);
    }

    bool OnMouseUp(float x, float y) override {
        if (!m_dragging) {
            return false;
        }
        m_dragging = false;
        return HitTest(x, y);
    }

    bool OnMouseLeave() override {
        if (!m_dragging) {
            return false;
        }
        m_dragging = false;
        return true;
    }

protected:
    float MeasurePreferredWidth(float availableWidth) const override {
        return std::min(availableWidth, ScaleValue(260.0f));
    }

    float MeasurePreferredHeight(float width) const override {
        (void)width;
        return ScaleValue(42.0f);
    }

public:
    void Render(IRenderer& renderer) override {
        const float scaledCornerRadius = ScaleValue(cornerRadius);
        renderer.FillRoundedRect(m_bounds, background, scaledCornerRadius);
        renderer.DrawRoundedRect(m_bounds, borderColor, 1.0f, scaledCornerRadius);
        if (m_hasFocus) {
            renderer.DrawRoundedRect(m_bounds, focusBorderColor, 2.0f, scaledCornerRadius);
        }

        const float insetX = ScaleValue(12.0f);
        const float insetY = ScaleValue(8.0f);
        const float barHeightPx = std::max(ScaleValue(4.0f), ScaleValue(barHeight));
        const float trackTop = m_bounds.top + insetY + std::max(0.0f, (m_bounds.Height() - insetY * 2.0f - barHeightPx) * 0.5f);
        const Rect trackRect = Rect::Make(
            m_bounds.left + insetX,
            trackTop,
            m_bounds.right - insetX,
            trackTop + barHeightPx);
        renderer.FillRoundedRect(trackRect, trackColor, barHeightPx * 0.5f);

        const float progress = GetNormalizedValue();
        const float filledWidth = trackRect.Width() * progress;
        const Rect fillRect = Rect::Make(trackRect.left, trackRect.top, trackRect.left + filledWidth, trackRect.bottom);
        if (filledWidth > 0.5f) {
            renderer.FillRoundedRect(fillRect, fillColor, barHeightPx * 0.5f);
        }

        std::wstring overlayText = m_label;
        if (m_showValueText) {
            const int percent = static_cast<int>(std::round(progress * 100.0f));
            const std::wstring percentText = std::to_wstring(percent) + L"%";
            overlayText = overlayText.empty() ? percentText : (overlayText + L" " + percentText);
        }
        if (!overlayText.empty()) {
            const TextRenderOptions textOptions{
                TextWrapMode::NoWrap,
                TextHorizontalAlign::Center,
                TextVerticalAlign::Center
            };
            renderer.DrawTextW(
                overlayText.c_str(),
                static_cast<UINT32>(overlayText.size()),
                m_bounds,
                textColor,
                ScaleValue(fontSize),
                textOptions);
        }
    }

    void SetLabel(std::wstring label) { m_label = std::move(label); }
    void SetRange(float minValue, float maxValue) {
        m_min = minValue;
        m_max = std::max(minValue + 0.001f, maxValue);
        SetValue(m_value);
    }
    void SetValue(float value) { m_value = std::clamp(value, m_min, m_max); }
    void SetStep(float step) { m_step = std::max(0.001f, step); }
    void SetShowValueText(bool showValueText) { m_showValueText = showValueText; }

    Color background = ColorFromHex(0x1A2431);
    Color borderColor = ColorFromHex(0x3A4B5D);
    Color focusBorderColor = ColorFromHex(0xFFFFFF);
    Color trackColor = ColorFromHex(0x2A3747);
    Color fillColor = ColorFromHex(0x4E7BFF);
    Color textColor = ColorFromHex(0xEAF3FF);
    float cornerRadius = 8.0f;
    float fontSize = 12.0f;
    float barHeight = 10.0f;

private:
    bool SetValueInternal(float value) {
        const float clamped = std::clamp(value, m_min, m_max);
        if (std::abs(clamped - m_value) <= 0.0001f) {
            return false;
        }
        m_value = clamped;
        return true;
    }

    bool UpdateValueFromPoint(float x) {
        const float insetX = ScaleValue(12.0f);
        const float left = m_bounds.left + insetX;
        const float right = m_bounds.right - insetX;
        const float width = std::max(1.0f, right - left);
        const float ratio = std::clamp((x - left) / width, 0.0f, 1.0f);
        return SetValueInternal(m_min + ratio * (m_max - m_min));
    }

    float GetNormalizedValue() const {
        return std::clamp((m_value - m_min) / std::max(0.001f, m_max - m_min), 0.0f, 1.0f);
    }

    std::wstring m_label;
    float m_min = 0.0f;
    float m_max = 100.0f;
    float m_value = 42.0f;
    float m_step = 1.0f;
    bool m_showValueText = true;
    bool m_dragging = false;
};

class ListBox : public UIElement {
public:
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
            m_hoveredIndex = -1;
            return true;
        }
        return false;
    }

    bool OnKeyDown(WPARAM keyCode, LPARAM /*lParam*/) override {
        if (m_items.empty()) {
            return false;
        }

        switch (keyCode) {
            case VK_UP:
                return MoveSelectionBy(-1);
            case VK_DOWN:
                return MoveSelectionBy(1);
            case VK_HOME:
                return SetSelectedIndex(0);
            case VK_END:
                return SetSelectedIndex(static_cast<int>(m_items.size()) - 1);
            case VK_PRIOR:
                return MoveSelectionBy(-VisibleRowCount());
            case VK_NEXT:
                return MoveSelectionBy(VisibleRowCount());
            default:
                break;
        }
        return false;
    }

    bool OnMouseDown(float x, float y) override {
        if (!HitTest(x, y)) {
            return false;
        }
        const int index = IndexFromPoint(y);
        if (index >= 0) {
            return SetSelectedIndex(index);
        }
        return true;
    }

    bool OnMouseMove(float x, float y) override {
        if (!HitTest(x, y)) {
            if (m_hoveredIndex == -1) {
                return false;
            }
            m_hoveredIndex = -1;
            return true;
        }
        const int nextHovered = IndexFromPoint(y);
        if (nextHovered == m_hoveredIndex) {
            return false;
        }
        m_hoveredIndex = nextHovered;
        return true;
    }

    bool OnMouseLeave() override {
        if (m_hoveredIndex == -1) {
            return false;
        }
        m_hoveredIndex = -1;
        return true;
    }

protected:
    float MeasurePreferredWidth(float availableWidth) const override {
        float preferred = ScaleValue(240.0f);
        if (!m_items.empty()) {
            const float horizontalPadding = ScaleValue(16.0f);
            float widest = 0.0f;
            for (size_t i = 0; i < std::min<size_t>(m_items.size(), 24); ++i) {
                const Size measured = MeasureTextValue(
                    m_items[i],
                    fontSize,
                    4096.0f,
                    TextWrapMode::NoWrap);
                widest = std::max(widest, measured.width);
            }
            preferred = std::max(preferred, widest + horizontalPadding);
        }
        return std::min(availableWidth, preferred);
    }

    float MeasurePreferredHeight(float width) const override {
        (void)width;
        const int rowCount = std::clamp(static_cast<int>(m_items.size()), 4, 8);
        return ScaleValue(8.0f) + ScaleValue(itemHeight) * static_cast<float>(rowCount) + ScaleValue(8.0f);
    }

public:
    void Render(IRenderer& renderer) override {
        const float scaledCornerRadius = ScaleValue(cornerRadius);
        renderer.FillRoundedRect(m_bounds, background, scaledCornerRadius);
        renderer.DrawRoundedRect(m_bounds, borderColor, 1.0f, scaledCornerRadius);
        if (m_hasFocus) {
            renderer.DrawRoundedRect(m_bounds, focusBorderColor, 2.0f, scaledCornerRadius);
        }

        if (m_items.empty()) {
            const std::wstring message = L"(empty)";
            const TextRenderOptions textOptions{
                TextWrapMode::NoWrap,
                TextHorizontalAlign::Center,
                TextVerticalAlign::Center
            };
            renderer.DrawTextW(
                message.c_str(),
                static_cast<UINT32>(message.size()),
                m_bounds,
                mutedTextColor,
                ScaleValue(fontSize),
                textOptions);
            return;
        }

        const float inset = ScaleValue(4.0f);
        const float rowHeight = ScaleValue(itemHeight);
        const Rect contentRect = Rect::Make(
            m_bounds.left + inset,
            m_bounds.top + inset,
            m_bounds.right - inset,
            m_bounds.bottom - inset);

        const int visibleRows = std::max(1, static_cast<int>(std::floor(contentRect.Height() / std::max(1.0f, rowHeight))));
        const int maxStart = std::max(0, static_cast<int>(m_items.size()) - visibleRows);
        m_scrollOffset = std::clamp(m_scrollOffset, 0, maxStart);
        const int endIndex = std::min(static_cast<int>(m_items.size()), m_scrollOffset + visibleRows);

        for (int i = m_scrollOffset; i < endIndex; ++i) {
            const float rowTop = contentRect.top + static_cast<float>(i - m_scrollOffset) * rowHeight;
            const float rowBottom = std::min(contentRect.bottom, rowTop + rowHeight);
            const Rect rowRect = Rect::Make(contentRect.left, rowTop, contentRect.right, rowBottom);

            if (i == m_selectedIndex) {
                renderer.FillRoundedRect(rowRect, selectedBackground, ScaleValue(4.0f));
            } else if (i == m_hoveredIndex) {
                renderer.FillRoundedRect(rowRect, hoverBackground, ScaleValue(4.0f));
            }

            const Rect textRect = Rect::Make(
                rowRect.left + ScaleValue(8.0f),
                rowRect.top,
                rowRect.right - ScaleValue(8.0f),
                rowRect.bottom);
            const TextRenderOptions textOptions{
                TextWrapMode::NoWrap,
                TextHorizontalAlign::Start,
                TextVerticalAlign::Center
            };
            renderer.DrawTextW(
                m_items[i].c_str(),
                static_cast<UINT32>(m_items[i].size()),
                textRect,
                textColor,
                ScaleValue(fontSize),
                textOptions);
        }

        if (visibleRows < static_cast<int>(m_items.size())) {
            const float barWidth = ScaleValue(4.0f);
            const float barRight = m_bounds.right - ScaleValue(3.0f);
            const float barLeft = barRight - barWidth;
            const float ratio = static_cast<float>(visibleRows) / static_cast<float>(m_items.size());
            const float thumbHeight = std::max(ScaleValue(14.0f), contentRect.Height() * ratio);
            const float trackTop = contentRect.top;
            const float trackBottom = contentRect.bottom;
            const float maxOffset = static_cast<float>(std::max(1, static_cast<int>(m_items.size()) - visibleRows));
            const float offsetRatio = static_cast<float>(m_scrollOffset) / maxOffset;
            const float thumbTop = trackTop + (contentRect.Height() - thumbHeight) * offsetRatio;
            const Rect thumbRect = Rect::Make(barLeft, thumbTop, barRight, thumbTop + thumbHeight);
            renderer.FillRoundedRect(thumbRect, scrollThumbColor, ScaleValue(2.0f));
        }
    }

    void SetItems(std::vector<std::wstring> items) {
        m_items = std::move(items);
        if (m_items.empty()) {
            m_selectedIndex = -1;
            m_hoveredIndex = -1;
            m_scrollOffset = 0;
            return;
        }
        m_selectedIndex = std::clamp(m_selectedIndex, 0, static_cast<int>(m_items.size()) - 1);
        EnsureSelectionVisible();
    }

    bool SetSelectedIndex(int index) {
        if (m_items.empty()) {
            if (m_selectedIndex == -1) {
                return false;
            }
            m_selectedIndex = -1;
            return true;
        }
        const int clamped = std::clamp(index, 0, static_cast<int>(m_items.size()) - 1);
        if (clamped == m_selectedIndex) {
            return false;
        }
        m_selectedIndex = clamped;
        EnsureSelectionVisible();
        return true;
    }

    int SelectedIndex() const { return m_selectedIndex; }
    const std::vector<std::wstring>& Items() const { return m_items; }

    Color background = ColorFromHex(0x14202D);
    Color borderColor = ColorFromHex(0x34485C);
    Color focusBorderColor = ColorFromHex(0xFFFFFF);
    Color selectedBackground = ColorFromHex(0x2F4F77);
    Color hoverBackground = ColorFromHex(0x1F3247);
    Color scrollThumbColor = ColorFromHex(0x536A82);
    Color textColor = ColorFromHex(0xE7EFFA);
    Color mutedTextColor = ColorFromHex(0x95A7BA);
    float cornerRadius = 8.0f;
    float fontSize = 13.0f;
    float itemHeight = 28.0f;

private:
    int VisibleRowCount() const {
        const float innerHeight = std::max(1.0f, m_bounds.Height() - ScaleValue(8.0f));
        const float rowHeight = std::max(1.0f, ScaleValue(itemHeight));
        return std::max(1, static_cast<int>(std::floor(innerHeight / rowHeight)));
    }

    int IndexFromPoint(float y) const {
        if (m_items.empty()) {
            return -1;
        }
        const float inset = ScaleValue(4.0f);
        const float rowHeight = std::max(1.0f, ScaleValue(itemHeight));
        const float localY = y - (m_bounds.top + inset);
        if (localY < 0.0f) {
            return -1;
        }
        const int row = static_cast<int>(std::floor(localY / rowHeight));
        if (row < 0 || row >= VisibleRowCount()) {
            return -1;
        }
        const int index = m_scrollOffset + row;
        return index < static_cast<int>(m_items.size()) ? index : -1;
    }

    bool MoveSelectionBy(int delta) {
        if (m_items.empty()) {
            return false;
        }
        if (m_selectedIndex < 0) {
            return SetSelectedIndex(0);
        }
        return SetSelectedIndex(m_selectedIndex + delta);
    }

    void EnsureSelectionVisible() {
        if (m_selectedIndex < 0 || m_items.empty()) {
            m_scrollOffset = 0;
            return;
        }
        const int visibleRows = VisibleRowCount();
        if (m_selectedIndex < m_scrollOffset) {
            m_scrollOffset = m_selectedIndex;
        } else if (m_selectedIndex >= m_scrollOffset + visibleRows) {
            m_scrollOffset = m_selectedIndex - visibleRows + 1;
        }
        const int maxStart = std::max(0, static_cast<int>(m_items.size()) - visibleRows);
        m_scrollOffset = std::clamp(m_scrollOffset, 0, maxStart);
    }

    std::vector<std::wstring> m_items;
    int m_selectedIndex = -1;
    int m_hoveredIndex = -1;
    int m_scrollOffset = 0;
};

class ComboBox : public UIElement {
public:
    bool IsFocusable() const override { return true; }

    bool OnFocus() override {
        if (!m_hasFocus) {
            m_hasFocus = true;
            return true;
        }
        return false;
    }

    bool OnBlur() override {
        bool changed = false;
        if (m_hasFocus) {
            m_hasFocus = false;
            changed = true;
        }
        if (m_expanded) {
            m_expanded = false;
            changed = true;
        }
        m_hoveredIndex = -1;
        return changed;
    }

    bool OnKeyDown(WPARAM keyCode, LPARAM /*lParam*/) override {
        if (m_items.empty()) {
            return false;
        }

        if (keyCode == VK_ESCAPE && m_expanded) {
            m_expanded = false;
            m_hoveredIndex = -1;
            return true;
        }

        if (keyCode == VK_RETURN || keyCode == VK_SPACE || keyCode == VK_F4) {
            if (m_expanded) {
                if (m_hoveredIndex >= 0) {
                    SetSelectedIndex(m_hoveredIndex);
                }
                m_expanded = false;
                m_hoveredIndex = -1;
            } else {
                m_expanded = true;
                m_hoveredIndex = std::max(0, m_selectedIndex);
            }
            return true;
        }

        if (keyCode == VK_UP || keyCode == VK_LEFT) {
            if (m_expanded) {
                return MoveHoverBy(-1);
            }
            return MoveSelectionBy(-1);
        }
        if (keyCode == VK_DOWN || keyCode == VK_RIGHT) {
            if (m_expanded) {
                return MoveHoverBy(1);
            }
            return MoveSelectionBy(1);
        }
        if (keyCode == VK_HOME) {
            return m_expanded ? SetHoverIndex(0) : SetSelectedIndex(0);
        }
        if (keyCode == VK_END) {
            const int last = static_cast<int>(m_items.size()) - 1;
            return m_expanded ? SetHoverIndex(last) : SetSelectedIndex(last);
        }
        return false;
    }

    bool OnMouseDown(float x, float y) override {
        if (!HitTest(x, y)) {
            return false;
        }

        if (HeaderRect().Contains(x, y)) {
            m_expanded = !m_expanded;
            m_hoveredIndex = m_expanded ? std::max(0, m_selectedIndex) : -1;
            return true;
        }

        if (m_expanded) {
            const int index = DropdownIndexFromPoint(y);
            if (index >= 0) {
                SetSelectedIndex(index);
                m_expanded = false;
                m_hoveredIndex = -1;
                return true;
            }
            m_expanded = false;
            m_hoveredIndex = -1;
            return true;
        }
        return false;
    }

    bool OnMouseMove(float x, float y) override {
        if (!m_expanded) {
            if (m_hoveredIndex == -1) {
                return false;
            }
            m_hoveredIndex = -1;
            return true;
        }
        if (!HitTest(x, y)) {
            if (m_hoveredIndex == -1) {
                return false;
            }
            m_hoveredIndex = -1;
            return true;
        }

        const int nextHover = DropdownIndexFromPoint(y);
        if (nextHover == m_hoveredIndex) {
            return false;
        }
        m_hoveredIndex = nextHover;
        return true;
    }

    bool OnMouseLeave() override {
        if (m_hoveredIndex == -1) {
            return false;
        }
        m_hoveredIndex = -1;
        return true;
    }

protected:
    float MeasurePreferredWidth(float availableWidth) const override {
        float preferred = ScaleValue(230.0f);
        if (!m_items.empty()) {
            float widest = 0.0f;
            for (const auto& item : m_items) {
                const Size measured = MeasureTextValue(item, fontSize, 4096.0f, TextWrapMode::NoWrap);
                widest = std::max(widest, measured.width);
            }
            preferred = std::max(preferred, widest + ScaleValue(40.0f));
        }
        return std::min(availableWidth, preferred);
    }

    float MeasurePreferredHeight(float width) const override {
        (void)width;
        return ScaleValue(headerHeight) + ScaleValue(itemHeight) * static_cast<float>(std::max(2, maxVisibleItems));
    }

public:
    void Render(IRenderer& renderer) override {
        const float scaledCornerRadius = ScaleValue(cornerRadius);
        renderer.FillRoundedRect(m_bounds, background, scaledCornerRadius);
        renderer.DrawRoundedRect(m_bounds, borderColor, 1.0f, scaledCornerRadius);
        if (m_hasFocus) {
            renderer.DrawRoundedRect(m_bounds, focusBorderColor, 2.0f, scaledCornerRadius);
        }

        const Rect header = HeaderRect();
        renderer.FillRoundedRect(header, headerBackground, ScaleValue(std::max(2.0f, cornerRadius - 1.0f)));

        const std::wstring selectedText = (m_selectedIndex >= 0 && m_selectedIndex < static_cast<int>(m_items.size()))
            ? m_items[m_selectedIndex]
            : std::wstring(L"(none)");
        const Rect textRect = Rect::Make(
            header.left + ScaleValue(10.0f),
            header.top,
            header.right - ScaleValue(28.0f),
            header.bottom);
        const TextRenderOptions textOptions{
            TextWrapMode::NoWrap,
            TextHorizontalAlign::Start,
            TextVerticalAlign::Center
        };
        renderer.DrawTextW(
            selectedText.c_str(),
            static_cast<UINT32>(selectedText.size()),
            textRect,
            textColor,
            ScaleValue(fontSize),
            textOptions);

        const float arrowCenterX = header.right - ScaleValue(14.0f);
        const float arrowCenterY = header.top + header.Height() * 0.5f;
        const float arrowSize = ScaleValue(4.0f);
        if (m_expanded) {
            renderer.DrawLine(
                PointF{arrowCenterX - arrowSize, arrowCenterY + arrowSize * 0.5f},
                PointF{arrowCenterX, arrowCenterY - arrowSize * 0.5f},
                arrowColor,
                1.5f);
            renderer.DrawLine(
                PointF{arrowCenterX, arrowCenterY - arrowSize * 0.5f},
                PointF{arrowCenterX + arrowSize, arrowCenterY + arrowSize * 0.5f},
                arrowColor,
                1.5f);
        } else {
            renderer.DrawLine(
                PointF{arrowCenterX - arrowSize, arrowCenterY - arrowSize * 0.5f},
                PointF{arrowCenterX, arrowCenterY + arrowSize * 0.5f},
                arrowColor,
                1.5f);
            renderer.DrawLine(
                PointF{arrowCenterX, arrowCenterY + arrowSize * 0.5f},
                PointF{arrowCenterX + arrowSize, arrowCenterY - arrowSize * 0.5f},
                arrowColor,
                1.5f);
        }

        if (!m_expanded) {
            return;
        }

        const Rect dropdownRect = DropdownRect();
        if (dropdownRect.Height() <= 1.0f) {
            return;
        }

        renderer.FillRect(dropdownRect, dropdownBackground);
        renderer.DrawRect(dropdownRect, borderColor, 1.0f);

        const float rowHeight = std::max(1.0f, ScaleValue(itemHeight));
        const int visibleRows = std::max(1, static_cast<int>(std::floor(dropdownRect.Height() / rowHeight)));
        const int count = std::min(static_cast<int>(m_items.size()), visibleRows);
        for (int i = 0; i < count; ++i) {
            const float top = dropdownRect.top + static_cast<float>(i) * rowHeight;
            const float bottom = std::min(dropdownRect.bottom, top + rowHeight);
            const Rect rowRect = Rect::Make(dropdownRect.left, top, dropdownRect.right, bottom);

            if (i == m_selectedIndex) {
                renderer.FillRect(rowRect, selectedBackground);
            } else if (i == m_hoveredIndex) {
                renderer.FillRect(rowRect, hoverBackground);
            }

            const Rect rowTextRect = Rect::Make(
                rowRect.left + ScaleValue(10.0f),
                rowRect.top,
                rowRect.right - ScaleValue(10.0f),
                rowRect.bottom);
            renderer.DrawTextW(
                m_items[i].c_str(),
                static_cast<UINT32>(m_items[i].size()),
                rowTextRect,
                textColor,
                ScaleValue(fontSize),
                textOptions);
        }
    }

    void SetItems(std::vector<std::wstring> items) {
        m_items = std::move(items);
        if (m_items.empty()) {
            m_selectedIndex = -1;
            m_hoveredIndex = -1;
            m_expanded = false;
            return;
        }
        m_selectedIndex = std::clamp(m_selectedIndex, 0, static_cast<int>(m_items.size()) - 1);
    }

    bool SetSelectedIndex(int index) {
        if (m_items.empty()) {
            if (m_selectedIndex == -1) {
                return false;
            }
            m_selectedIndex = -1;
            return true;
        }
        const int clamped = std::clamp(index, 0, static_cast<int>(m_items.size()) - 1);
        if (clamped == m_selectedIndex) {
            return false;
        }
        m_selectedIndex = clamped;
        return true;
    }

    void SetExpanded(bool expanded) {
        m_expanded = expanded && !m_items.empty();
        if (!m_expanded) {
            m_hoveredIndex = -1;
        }
    }

    int SelectedIndex() const { return m_selectedIndex; }

    Color background = ColorFromHex(0x13202D);
    Color headerBackground = ColorFromHex(0x1B2C3D);
    Color dropdownBackground = ColorFromHex(0x112130);
    Color borderColor = ColorFromHex(0x36506A);
    Color focusBorderColor = ColorFromHex(0xFFFFFF);
    Color selectedBackground = ColorFromHex(0x2F4F77);
    Color hoverBackground = ColorFromHex(0x20374F);
    Color textColor = ColorFromHex(0xE9F2FD);
    Color arrowColor = ColorFromHex(0xC8D9EC);
    float cornerRadius = 8.0f;
    float fontSize = 13.0f;
    float headerHeight = 34.0f;
    float itemHeight = 28.0f;
    int maxVisibleItems = 5;

private:
    Rect HeaderRect() const {
        const float height = std::min(m_bounds.Height(), ScaleValue(headerHeight));
        return Rect::Make(m_bounds.left, m_bounds.top, m_bounds.right, m_bounds.top + height);
    }

    Rect DropdownRect() const {
        const Rect header = HeaderRect();
        return Rect::Make(m_bounds.left, header.bottom, m_bounds.right, m_bounds.bottom);
    }

    int DropdownIndexFromPoint(float y) const {
        if (!m_expanded || m_items.empty()) {
            return -1;
        }
        const Rect dropdown = DropdownRect();
        if (!dropdown.Contains(m_bounds.left + 1.0f, y)) {
            return -1;
        }
        const float rowHeight = std::max(1.0f, ScaleValue(itemHeight));
        const int row = static_cast<int>(std::floor((y - dropdown.top) / rowHeight));
        if (row < 0 || row >= std::min(static_cast<int>(m_items.size()), maxVisibleItems)) {
            return -1;
        }
        return row;
    }

    bool MoveSelectionBy(int delta) {
        if (m_items.empty()) {
            return false;
        }
        if (m_selectedIndex < 0) {
            return SetSelectedIndex(0);
        }
        return SetSelectedIndex(m_selectedIndex + delta);
    }

    bool SetHoverIndex(int index) {
        if (m_items.empty()) {
            return false;
        }
        const int clamped = std::clamp(index, 0, static_cast<int>(m_items.size()) - 1);
        if (clamped == m_hoveredIndex) {
            return false;
        }
        m_hoveredIndex = clamped;
        return true;
    }

    bool MoveHoverBy(int delta) {
        if (m_items.empty()) {
            return false;
        }
        if (m_hoveredIndex < 0) {
            return SetHoverIndex(std::max(0, m_selectedIndex));
        }
        return SetHoverIndex(m_hoveredIndex + delta);
    }

    std::vector<std::wstring> m_items;
    int m_selectedIndex = -1;
    int m_hoveredIndex = -1;
    bool m_expanded = false;
};

class TabControl : public UIElement {
public:
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
            m_hoveredTab = -1;
            return true;
        }
        return false;
    }

    bool OnKeyDown(WPARAM keyCode, LPARAM /*lParam*/) override {
        if (TabCount() <= 0) {
            return false;
        }
        if (keyCode == VK_LEFT || keyCode == VK_UP) {
            return SetSelectedIndex(m_selectedIndex - 1);
        }
        if (keyCode == VK_RIGHT || keyCode == VK_DOWN) {
            return SetSelectedIndex(m_selectedIndex + 1);
        }
        if (keyCode == VK_HOME) {
            return SetSelectedIndex(0);
        }
        if (keyCode == VK_END) {
            return SetSelectedIndex(TabCount() - 1);
        }
        if (UIElement* child = SelectedChild()) {
            return child->OnKeyDown(keyCode, 0);
        }
        return false;
    }

    bool OnMouseDown(float x, float y) override {
        if (!HitTest(x, y)) {
            return false;
        }
        const int tabIndex = TabIndexFromPoint(x, y);
        if (tabIndex >= 0) {
            SetSelectedIndex(tabIndex);
            return true;
        }
        if (UIElement* child = SelectedChild()) {
            if (child->HitTest(x, y)) {
                return child->OnMouseDown(x, y);
            }
        }
        return true;
    }

    bool OnMouseMove(float x, float y) override {
        bool changed = false;
        const int nextHovered = TabIndexFromPoint(x, y);
        if (nextHovered != m_hoveredTab) {
            m_hoveredTab = nextHovered;
            changed = true;
        }

        if (UIElement* child = SelectedChild()) {
            if (child->HitTest(x, y)) {
                changed = child->OnMouseMove(x, y) || changed;
            } else {
                changed = child->OnMouseLeave() || changed;
            }
        }
        return changed;
    }

    bool OnMouseUp(float x, float y) override {
        if (UIElement* child = SelectedChild()) {
            return child->OnMouseUp(x, y);
        }
        return false;
    }

    bool OnMouseLeave() override {
        bool changed = false;
        if (m_hoveredTab != -1) {
            m_hoveredTab = -1;
            changed = true;
        }
        if (UIElement* child = SelectedChild()) {
            changed = child->OnMouseLeave() || changed;
        }
        return changed;
    }

    UIElement* FindFocusableAt(float x, float y) override {
        if (!HitTest(x, y)) {
            return nullptr;
        }
        if (UIElement* child = SelectedChild()) {
            if (child->HitTest(x, y)) {
                if (auto* focusable = child->FindFocusableAt(x, y)) {
                    return focusable;
                }
            }
        }
        return const_cast<TabControl*>(this);
    }

    UIElement* FindHitElementAt(float x, float y) override {
        if (!HitTest(x, y)) {
            return nullptr;
        }
        if (TabIndexFromPoint(x, y) >= 0) {
            return this;
        }
        if (UIElement* child = SelectedChild()) {
            if (child->HitTest(x, y)) {
                if (auto* nested = child->FindHitElementAt(x, y)) {
                    return nested;
                }
                return child;
            }
        }
        return this;
    }

    void CollectFocusable(std::vector<UIElement*>& out) override {
        out.push_back(this);
        if (UIElement* child = SelectedChild()) {
            child->CollectFocusable(out);
        }
    }

    void Arrange(const Rect& finalRect) override {
        UIElement::Arrange(finalRect);

        const Rect content = ContentRect();
        const Rect hidden = Rect::Make(-10000.0f, -10000.0f, -9990.0f, -9990.0f);
        NormalizeSelectedIndex();
        for (size_t i = 0; i < m_children.size(); ++i) {
            if (static_cast<int>(i) == m_selectedIndex) {
                m_children[i]->Arrange(content);
            } else {
                m_children[i]->Arrange(hidden);
            }
        }
    }

protected:
    float MeasurePreferredWidth(float availableWidth) const override {
        return std::min(availableWidth, ScaleValue(480.0f));
    }

    float MeasurePreferredHeight(float width) const override {
        (void)width;
        return ScaleValue(300.0f);
    }

public:
    void Render(IRenderer& renderer) override {
        const float scaledCornerRadius = ScaleValue(cornerRadius);
        renderer.FillRoundedRect(m_bounds, background, scaledCornerRadius);
        renderer.DrawRoundedRect(m_bounds, borderColor, 1.0f, scaledCornerRadius);
        if (m_hasFocus) {
            renderer.DrawRoundedRect(m_bounds, focusBorderColor, 2.0f, scaledCornerRadius);
        }

        const Rect header = HeaderRect();
        renderer.FillRect(header, headerBackground);
        renderer.DrawLine(
            PointF{header.left, header.bottom},
            PointF{header.right, header.bottom},
            borderColor,
            1.0f);

        const std::vector<Rect> tabs = BuildTabRects();
        for (size_t i = 0; i < tabs.size(); ++i) {
            const bool selected = static_cast<int>(i) == m_selectedIndex;
            const bool hovered = static_cast<int>(i) == m_hoveredTab;

            Color tabBg = tabBackground;
            if (selected) {
                tabBg = selectedTabBackground;
            } else if (hovered) {
                tabBg = hoverTabBackground;
            }

            renderer.FillRect(tabs[i], tabBg);
            renderer.DrawRect(tabs[i], borderColor, 1.0f);

            const std::wstring title = TabTitle(i);
            const TextRenderOptions tabTextOptions{
                TextWrapMode::NoWrap,
                TextHorizontalAlign::Center,
                TextVerticalAlign::Center
            };
            renderer.DrawTextW(
                title.c_str(),
                static_cast<UINT32>(title.size()),
                tabs[i],
                selected ? selectedTabTextColor : tabTextColor,
                ScaleValue(fontSize),
                tabTextOptions);
        }

        const Rect content = ContentRect();
        renderer.FillRect(content, contentBackground);
        renderer.DrawRect(content, borderColor, 1.0f);

        if (UIElement* child = SelectedChild()) {
            child->Render(renderer);
        }
    }

    void SetTabs(std::vector<std::wstring> tabs) {
        m_tabs = std::move(tabs);
        NormalizeSelectedIndex();
    }

    bool SetSelectedIndex(int index) {
        const int count = TabCount();
        if (count <= 0) {
            if (m_selectedIndex == -1) {
                return false;
            }
            m_selectedIndex = -1;
            return true;
        }
        const int clamped = std::clamp(index, 0, count - 1);
        if (clamped == m_selectedIndex) {
            return false;
        }
        m_selectedIndex = clamped;
        return true;
    }

    int SelectedIndex() const { return m_selectedIndex; }

    Color background = ColorFromHex(0x121F2E);
    Color headerBackground = ColorFromHex(0x1A2A3B);
    Color contentBackground = ColorFromHex(0x0F1A26);
    Color borderColor = ColorFromHex(0x3B536B);
    Color focusBorderColor = ColorFromHex(0xFFFFFF);
    Color tabBackground = ColorFromHex(0x1F3247);
    Color hoverTabBackground = ColorFromHex(0x2A415A);
    Color selectedTabBackground = ColorFromHex(0x355A82);
    Color tabTextColor = ColorFromHex(0xD6E4F2);
    Color selectedTabTextColor = ColorFromHex(0xFFFFFF);
    float cornerRadius = 10.0f;
    float fontSize = 13.0f;
    float headerHeight = 34.0f;

private:
    int TabCount() const {
        return static_cast<int>(std::max(m_tabs.size(), m_children.size()));
    }

    std::wstring TabTitle(size_t index) const {
        if (index < m_tabs.size() && !m_tabs[index].empty()) {
            return m_tabs[index];
        }
        return L"Tab " + std::to_wstring(index + 1);
    }

    Rect HeaderRect() const {
        const float height = std::min(m_bounds.Height(), ScaleValue(headerHeight));
        return Rect::Make(m_bounds.left, m_bounds.top, m_bounds.right, m_bounds.top + height);
    }

    Rect ContentRect() const {
        const Rect header = HeaderRect();
        const float inset = ScaleValue(1.0f);
        return Rect::Make(
            m_bounds.left + inset,
            std::min(m_bounds.bottom, header.bottom + inset),
            m_bounds.right - inset,
            m_bounds.bottom - inset);
    }

    std::vector<Rect> BuildTabRects() const {
        std::vector<Rect> rects;
        const int count = TabCount();
        if (count <= 0) {
            return rects;
        }

        const Rect header = HeaderRect();
        const float width = std::max(1.0f, header.Width());
        const float tabWidth = width / static_cast<float>(count);
        rects.reserve(static_cast<size_t>(count));
        float x = header.left;
        for (int i = 0; i < count; ++i) {
            const float right = (i == count - 1) ? header.right : x + tabWidth;
            rects.push_back(Rect::Make(x, header.top, right, header.bottom));
            x = right;
        }
        return rects;
    }

    int TabIndexFromPoint(float x, float y) const {
        const std::vector<Rect> rects = BuildTabRects();
        for (size_t i = 0; i < rects.size(); ++i) {
            if (rects[i].Contains(x, y)) {
                return static_cast<int>(i);
            }
        }
        return -1;
    }

    UIElement* SelectedChild() {
        NormalizeSelectedIndex();
        if (m_selectedIndex < 0 || m_selectedIndex >= static_cast<int>(m_children.size())) {
            return nullptr;
        }
        return m_children[static_cast<size_t>(m_selectedIndex)].get();
    }

    void NormalizeSelectedIndex() {
        const int count = TabCount();
        if (count <= 0) {
            m_selectedIndex = -1;
            return;
        }
        m_selectedIndex = std::clamp(m_selectedIndex < 0 ? 0 : m_selectedIndex, 0, count - 1);
    }

    std::vector<std::wstring> m_tabs;
    int m_selectedIndex = 0;
    int m_hoveredTab = -1;
};

class ListView : public UIElement {
public:
    struct Column {
        std::wstring title;
        float width = -1.0f;
    };

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
            m_hoveredRow = -1;
            return true;
        }
        return false;
    }

    bool OnKeyDown(WPARAM keyCode, LPARAM /*lParam*/) override {
        if (m_rows.empty()) {
            return false;
        }
        switch (keyCode) {
            case VK_UP:
                return SetSelectedIndex(m_selectedRow - 1);
            case VK_DOWN:
                return SetSelectedIndex(m_selectedRow + 1);
            case VK_HOME:
                return SetSelectedIndex(0);
            case VK_END:
                return SetSelectedIndex(static_cast<int>(m_rows.size()) - 1);
            case VK_PRIOR:
                return SetSelectedIndex(m_selectedRow - VisibleRowCount());
            case VK_NEXT:
                return SetSelectedIndex(m_selectedRow + VisibleRowCount());
            default:
                return false;
        }
    }

    bool OnMouseDown(float x, float y) override {
        if (!HitTest(x, y)) {
            return false;
        }
        const int rowIndex = RowIndexFromPoint(y);
        if (rowIndex >= 0) {
            SetSelectedIndex(rowIndex);
            return true;
        }
        return true;
    }

    bool OnMouseMove(float x, float y) override {
        if (!HitTest(x, y)) {
            if (m_hoveredRow == -1) {
                return false;
            }
            m_hoveredRow = -1;
            return true;
        }
        const int nextHovered = RowIndexFromPoint(y);
        if (nextHovered == m_hoveredRow) {
            return false;
        }
        m_hoveredRow = nextHovered;
        return true;
    }

    bool OnMouseLeave() override {
        if (m_hoveredRow == -1) {
            return false;
        }
        m_hoveredRow = -1;
        return true;
    }

protected:
    float MeasurePreferredWidth(float availableWidth) const override {
        if (m_columns.empty()) {
            return std::min(availableWidth, ScaleValue(420.0f));
        }

        const Thickness scaledCellPadding = ScaleThickness(cellPadding);
        float total = 0.0f;
        for (size_t col = 0; col < m_columns.size(); ++col) {
            if (m_columns[col].width > 0.0f) {
                total += ScaleValue(m_columns[col].width);
                continue;
            }

            float width = 0.0f;
            const Size header = MeasureTextValue(
                m_columns[col].title.empty() ? L"Column" : m_columns[col].title,
                headerFontSize,
                4096.0f,
                TextWrapMode::NoWrap);
            width = std::max(width, header.width);

            const size_t inspectRows = std::min<size_t>(m_rows.size(), 30);
            for (size_t row = 0; row < inspectRows; ++row) {
                if (col >= m_rows[row].size()) {
                    continue;
                }
                const Size cell = MeasureTextValue(
                    m_rows[row][col],
                    fontSize,
                    4096.0f,
                    TextWrapMode::NoWrap);
                width = std::max(width, cell.width);
            }
            total += width + scaledCellPadding.left + scaledCellPadding.right + ScaleValue(10.0f);
        }
        return std::min(availableWidth, total + ScaleValue(2.0f));
    }

    float MeasurePreferredHeight(float width) const override {
        (void)width;
        const float header = ScaleValue(headerHeight);
        const float row = ScaleValue(rowHeight);
        const int rows = std::clamp(static_cast<int>(m_rows.size()), 5, 10);
        return header + row * static_cast<float>(rows) + ScaleValue(2.0f);
    }

public:
    void Render(IRenderer& renderer) override {
        const float scaledCornerRadius = ScaleValue(cornerRadius);
        renderer.FillRoundedRect(m_bounds, background, scaledCornerRadius);
        renderer.DrawRoundedRect(m_bounds, borderColor, 1.0f, scaledCornerRadius);
        if (m_hasFocus) {
            renderer.DrawRoundedRect(m_bounds, focusBorderColor, 2.0f, scaledCornerRadius);
        }

        if (m_columns.empty()) {
            const std::wstring message = L"No columns";
            const TextRenderOptions textOptions{
                TextWrapMode::NoWrap,
                TextHorizontalAlign::Center,
                TextVerticalAlign::Center
            };
            renderer.DrawTextW(
                message.c_str(),
                static_cast<UINT32>(message.size()),
                m_bounds,
                textColor,
                ScaleValue(fontSize),
                textOptions);
            return;
        }

        const std::vector<float> widths = ResolveColumnWidths();
        if (widths.empty()) {
            return;
        }

        const Rect headerRect = HeaderRect();
        const Rect bodyRect = BodyRect();
        const float scaledHeaderHeight = ScaleValue(headerHeight);
        const float scaledRowHeight = std::max(1.0f, ScaleValue(rowHeight));
        const Thickness scaledCellPadding = ScaleThickness(cellPadding);

        renderer.PushRoundedClip(m_bounds, scaledCornerRadius);
        renderer.FillRect(headerRect, headerBackground);
        renderer.DrawLine(
            PointF{m_bounds.left, headerRect.bottom},
            PointF{m_bounds.right, headerRect.bottom},
            gridLineColor,
            1.0f);

        float x = m_bounds.left;
        for (size_t col = 0; col < m_columns.size(); ++col) {
            const Rect cellRect = Rect::Make(
                x + scaledCellPadding.left,
                headerRect.top + scaledCellPadding.top,
                x + widths[col] - scaledCellPadding.right,
                headerRect.bottom - scaledCellPadding.bottom);
            const TextRenderOptions textOptions{
                TextWrapMode::NoWrap,
                TextHorizontalAlign::Start,
                TextVerticalAlign::Center
            };
            renderer.DrawTextW(
                m_columns[col].title.c_str(),
                static_cast<UINT32>(m_columns[col].title.size()),
                cellRect,
                headerTextColor,
                ScaleValue(headerFontSize),
                textOptions);

            if (col + 1 < m_columns.size()) {
                renderer.DrawLine(
                    PointF{x + widths[col], m_bounds.top},
                    PointF{x + widths[col], m_bounds.bottom},
                    gridLineColor,
                    1.0f);
            }
            x += widths[col];
        }

        const int visibleRows = std::max(1, static_cast<int>(std::floor(bodyRect.Height() / scaledRowHeight)));
        const int maxStart = std::max(0, static_cast<int>(m_rows.size()) - visibleRows);
        m_scrollOffset = std::clamp(m_scrollOffset, 0, maxStart);
        const int endIndex = std::min(static_cast<int>(m_rows.size()), m_scrollOffset + visibleRows);

        for (int row = m_scrollOffset; row < endIndex; ++row) {
            const float rowTop = bodyRect.top + static_cast<float>(row - m_scrollOffset) * scaledRowHeight;
            const float rowBottom = std::min(bodyRect.bottom, rowTop + scaledRowHeight);
            const Rect rowRect = Rect::Make(m_bounds.left, rowTop, m_bounds.right, rowBottom);

            if (row == m_selectedRow) {
                renderer.FillRect(rowRect, selectedRowBackground);
            } else if (row == m_hoveredRow) {
                renderer.FillRect(rowRect, hoverRowBackground);
            } else {
                renderer.FillRect(rowRect, (row % 2 == 0) ? rowBackgroundA : rowBackgroundB);
            }

            float rowX = m_bounds.left;
            for (size_t col = 0; col < m_columns.size(); ++col) {
                const std::wstring value = (col < m_rows[row].size()) ? m_rows[row][col] : L"";
                const Rect textRect = Rect::Make(
                    rowX + scaledCellPadding.left,
                    rowTop + scaledCellPadding.top,
                    rowX + widths[col] - scaledCellPadding.right,
                    rowBottom - scaledCellPadding.bottom);
                const TextRenderOptions textOptions{
                    TextWrapMode::NoWrap,
                    TextHorizontalAlign::Start,
                    TextVerticalAlign::Center
                };
                renderer.DrawTextW(
                    value.c_str(),
                    static_cast<UINT32>(value.size()),
                    textRect,
                    textColor,
                    ScaleValue(fontSize),
                    textOptions);
                rowX += widths[col];
            }

            renderer.DrawLine(
                PointF{m_bounds.left, rowBottom},
                PointF{m_bounds.right, rowBottom},
                gridLineColor,
                1.0f);
        }

        if (visibleRows < static_cast<int>(m_rows.size())) {
            const float thumbWidth = ScaleValue(4.0f);
            const float thumbRight = m_bounds.right - ScaleValue(3.0f);
            const float thumbLeft = thumbRight - thumbWidth;
            const float ratio = static_cast<float>(visibleRows) / static_cast<float>(m_rows.size());
            const float thumbHeight = std::max(ScaleValue(16.0f), bodyRect.Height() * ratio);
            const float maxOffset = static_cast<float>(std::max(1, static_cast<int>(m_rows.size()) - visibleRows));
            const float offsetRatio = static_cast<float>(m_scrollOffset) / maxOffset;
            const float thumbTop = bodyRect.top + (bodyRect.Height() - thumbHeight) * offsetRatio;
            const Rect thumbRect = Rect::Make(thumbLeft, thumbTop, thumbRight, thumbTop + thumbHeight);
            renderer.FillRoundedRect(thumbRect, scrollThumbColor, ScaleValue(2.0f));
        }

        renderer.PopLayer();
        (void)scaledHeaderHeight;
    }

    void SetColumns(std::vector<Column> columns) { m_columns = std::move(columns); }
    void SetRows(std::vector<std::vector<std::wstring>> rows) {
        m_rows = std::move(rows);
        if (m_rows.empty()) {
            m_selectedRow = -1;
            m_scrollOffset = 0;
        } else if (m_selectedRow < 0) {
            m_selectedRow = 0;
        } else {
            m_selectedRow = std::clamp(m_selectedRow, 0, static_cast<int>(m_rows.size()) - 1);
        }
        EnsureSelectionVisible();
    }

    void SetColumnWidths(std::vector<float> widths) {
        const size_t count = std::min(widths.size(), m_columns.size());
        for (size_t i = 0; i < count; ++i) {
            m_columns[i].width = widths[i];
        }
    }

    bool SetSelectedIndex(int index) {
        if (m_rows.empty()) {
            if (m_selectedRow == -1) {
                return false;
            }
            m_selectedRow = -1;
            return true;
        }
        const int clamped = std::clamp(index, 0, static_cast<int>(m_rows.size()) - 1);
        if (clamped == m_selectedRow) {
            return false;
        }
        m_selectedRow = clamped;
        EnsureSelectionVisible();
        return true;
    }

    int SelectedIndex() const { return m_selectedRow; }

    Color background = ColorFromHex(0x101C29);
    Color borderColor = ColorFromHex(0x35506A);
    Color focusBorderColor = ColorFromHex(0xFFFFFF);
    Color headerBackground = ColorFromHex(0x1D3247);
    Color rowBackgroundA = ColorFromHex(0x142536);
    Color rowBackgroundB = ColorFromHex(0x102132);
    Color hoverRowBackground = ColorFromHex(0x203B56);
    Color selectedRowBackground = ColorFromHex(0x2D567C);
    Color gridLineColor = ColorFromHex(0x2D445A);
    Color headerTextColor = ColorFromHex(0xEAF4FF);
    Color textColor = ColorFromHex(0xD0DDEC);
    Color scrollThumbColor = ColorFromHex(0x58718C);
    float cornerRadius = 10.0f;
    float headerHeight = 34.0f;
    float rowHeight = 30.0f;
    float fontSize = 13.0f;
    float headerFontSize = 13.0f;
    Thickness cellPadding{8, 5, 8, 5};

private:
    Rect HeaderRect() const {
        const float h = std::min(m_bounds.Height(), ScaleValue(headerHeight));
        return Rect::Make(m_bounds.left, m_bounds.top, m_bounds.right, m_bounds.top + h);
    }

    Rect BodyRect() const {
        const Rect header = HeaderRect();
        return Rect::Make(m_bounds.left, header.bottom, m_bounds.right, m_bounds.bottom);
    }

    int VisibleRowCount() const {
        const Rect body = BodyRect();
        const float row = std::max(1.0f, ScaleValue(rowHeight));
        return std::max(1, static_cast<int>(std::floor(body.Height() / row)));
    }

    void EnsureSelectionVisible() {
        if (m_selectedRow < 0 || m_rows.empty()) {
            m_scrollOffset = 0;
            return;
        }
        const int visible = VisibleRowCount();
        if (m_selectedRow < m_scrollOffset) {
            m_scrollOffset = m_selectedRow;
        } else if (m_selectedRow >= m_scrollOffset + visible) {
            m_scrollOffset = m_selectedRow - visible + 1;
        }
        const int maxStart = std::max(0, static_cast<int>(m_rows.size()) - visible);
        m_scrollOffset = std::clamp(m_scrollOffset, 0, maxStart);
    }

    int RowIndexFromPoint(float y) const {
        if (m_rows.empty()) {
            return -1;
        }
        const Rect body = BodyRect();
        if (y < body.top || y > body.bottom) {
            return -1;
        }
        const float row = std::max(1.0f, ScaleValue(rowHeight));
        const int displayRow = static_cast<int>(std::floor((y - body.top) / row));
        const int index = m_scrollOffset + displayRow;
        return (index >= 0 && index < static_cast<int>(m_rows.size())) ? index : -1;
    }

    std::vector<float> ResolveColumnWidths() const {
        std::vector<float> widths;
        if (m_columns.empty()) {
            return widths;
        }
        widths.resize(m_columns.size(), 0.0f);
        const float totalWidth = std::max(1.0f, m_bounds.Width());

        float fixed = 0.0f;
        size_t autoCount = 0;
        for (size_t i = 0; i < m_columns.size(); ++i) {
            if (m_columns[i].width > 0.0f) {
                widths[i] = ScaleValue(m_columns[i].width);
                fixed += widths[i];
            } else {
                ++autoCount;
            }
        }

        const float autoWidth = autoCount > 0
            ? std::max(ScaleValue(52.0f), (std::max(0.0f, totalWidth - fixed) / static_cast<float>(autoCount)))
            : 0.0f;
        float sum = 0.0f;
        for (size_t i = 0; i < widths.size(); ++i) {
            if (widths[i] <= 0.0f) {
                widths[i] = autoWidth;
            }
            widths[i] = std::max(ScaleValue(36.0f), widths[i]);
            sum += widths[i];
        }

        if (sum > totalWidth && sum > 0.0f) {
            const float ratio = totalWidth / sum;
            for (float& w : widths) {
                w = std::max(ScaleValue(28.0f), w * ratio);
            }
        }
        return widths;
    }

    std::vector<Column> m_columns;
    std::vector<std::vector<std::wstring>> m_rows;
    int m_selectedRow = -1;
    int m_hoveredRow = -1;
    int m_scrollOffset = 0;
};

class TreeView : public UIElement {
public:
    bool IsFocusable() const override { return true; }

    bool OnFocus() override {
        if (!m_hasFocus) {
            m_hasFocus = true;
            return true;
        }
        return false;
    }

    bool OnBlur() override {
        bool changed = false;
        if (m_hasFocus) {
            m_hasFocus = false;
            changed = true;
        }
        if (m_hasHoveredPath) {
            m_hasHoveredPath = false;
            m_hoveredPath.clear();
            changed = true;
        }
        return changed;
    }

    bool OnKeyDown(WPARAM keyCode, LPARAM /*lParam*/) override {
        auto visible = BuildVisibleNodes();
        if (visible.empty()) {
            return false;
        }

        int selectedIndex = SelectedVisibleIndex(visible);
        if (selectedIndex < 0) {
            selectedIndex = 0;
            SetSelectedPath(visible[0].path);
        }

        if (keyCode == VK_UP) {
            if (selectedIndex > 0) {
                SetSelectedPath(visible[static_cast<size_t>(selectedIndex - 1)].path);
                EnsureSelectedVisible(visible);
                return true;
            }
            return false;
        }
        if (keyCode == VK_DOWN) {
            if (selectedIndex + 1 < static_cast<int>(visible.size())) {
                SetSelectedPath(visible[static_cast<size_t>(selectedIndex + 1)].path);
                EnsureSelectedVisible(visible);
                return true;
            }
            return false;
        }
        if (keyCode == VK_HOME) {
            SetSelectedPath(visible.front().path);
            EnsureSelectedVisible(visible);
            return true;
        }
        if (keyCode == VK_END) {
            SetSelectedPath(visible.back().path);
            EnsureSelectedVisible(visible);
            return true;
        }

        Node* selected = GetNode(m_selectedPath);
        if (!selected) {
            return false;
        }

        if (keyCode == VK_LEFT) {
            if (!selected->children.empty() && selected->expanded) {
                selected->expanded = false;
                return true;
            }
            if (m_selectedPath.size() > 1) {
                auto parentPath = m_selectedPath;
                parentPath.pop_back();
                SetSelectedPath(parentPath);
                EnsureSelectedVisible(BuildVisibleNodes());
                return true;
            }
            return false;
        }

        if (keyCode == VK_RIGHT) {
            if (!selected->children.empty() && !selected->expanded) {
                selected->expanded = true;
                return true;
            }
            if (!selected->children.empty()) {
                auto childPath = m_selectedPath;
                childPath.push_back(0);
                SetSelectedPath(childPath);
                EnsureSelectedVisible(BuildVisibleNodes());
                return true;
            }
            return false;
        }

        if (keyCode == VK_SPACE || keyCode == VK_RETURN) {
            if (!selected->children.empty()) {
                selected->expanded = !selected->expanded;
                return true;
            }
            return false;
        }

        return false;
    }

    bool OnMouseDown(float x, float y) override {
        if (!HitTest(x, y)) {
            return false;
        }

        auto visible = BuildVisibleNodes();
        if (visible.empty()) {
            return true;
        }
        const int visibleIndex = VisibleIndexFromPoint(y, static_cast<int>(visible.size()));
        if (visibleIndex < 0 || visibleIndex >= static_cast<int>(visible.size())) {
            return true;
        }

        const auto& entry = visible[static_cast<size_t>(visibleIndex)];
        const int rowIndex = m_scrollOffset + visibleIndex;
        SetSelectedPath(entry.path);
        EnsureSelectedVisible(visible);

        if (entry.node && !entry.node->children.empty() && ToggleGlyphRect(entry.depth, rowIndex).Contains(x, y)) {
            entry.node->expanded = !entry.node->expanded;
        }
        return true;
    }

    bool OnMouseMove(float x, float y) override {
        auto visible = BuildVisibleNodes();
        if (!HitTest(x, y) || visible.empty()) {
            if (!m_hasHoveredPath) {
                return false;
            }
            m_hasHoveredPath = false;
            m_hoveredPath.clear();
            return true;
        }

        const int visibleIndex = VisibleIndexFromPoint(y, static_cast<int>(visible.size()));
        if (visibleIndex < 0 || visibleIndex >= static_cast<int>(visible.size())) {
            if (!m_hasHoveredPath) {
                return false;
            }
            m_hasHoveredPath = false;
            m_hoveredPath.clear();
            return true;
        }

        const auto& path = visible[static_cast<size_t>(visibleIndex)].path;
        if (m_hasHoveredPath && path == m_hoveredPath) {
            return false;
        }
        m_hoveredPath = path;
        m_hasHoveredPath = true;
        return true;
    }

    bool OnMouseLeave() override {
        if (!m_hasHoveredPath) {
            return false;
        }
        m_hasHoveredPath = false;
        m_hoveredPath.clear();
        return true;
    }

protected:
    float MeasurePreferredWidth(float availableWidth) const override {
        float preferred = ScaleValue(320.0f);
        const float maxWidth = std::max(availableWidth, preferred);
        return std::min(availableWidth, maxWidth);
    }

    float MeasurePreferredHeight(float width) const override {
        (void)width;
        return ScaleValue(280.0f);
    }

public:
    void Render(IRenderer& renderer) override {
        const float scaledCornerRadius = ScaleValue(cornerRadius);
        renderer.FillRoundedRect(m_bounds, background, scaledCornerRadius);
        renderer.DrawRoundedRect(m_bounds, borderColor, 1.0f, scaledCornerRadius);
        if (m_hasFocus) {
            renderer.DrawRoundedRect(m_bounds, focusBorderColor, 2.0f, scaledCornerRadius);
        }

        auto visible = BuildVisibleNodes();
        if (visible.empty()) {
            const std::wstring message = L"(empty tree)";
            const TextRenderOptions textOptions{
                TextWrapMode::NoWrap,
                TextHorizontalAlign::Center,
                TextVerticalAlign::Center
            };
            renderer.DrawTextW(
                message.c_str(),
                static_cast<UINT32>(message.size()),
                m_bounds,
                mutedTextColor,
                ScaleValue(fontSize),
                textOptions);
            return;
        }

        const Rect content = ContentRect();
        const float row = std::max(1.0f, ScaleValue(itemHeight));
        const int visibleRows = std::max(1, static_cast<int>(std::floor(content.Height() / row)));
        const int maxStart = std::max(0, static_cast<int>(visible.size()) - visibleRows);
        m_scrollOffset = std::clamp(m_scrollOffset, 0, maxStart);
        const int count = std::min(visibleRows, static_cast<int>(visible.size()) - m_scrollOffset);

        renderer.PushRoundedClip(m_bounds, scaledCornerRadius);
        for (int i = 0; i < count; ++i) {
            const int globalIndex = m_scrollOffset + i;
            const auto& entry = visible[static_cast<size_t>(globalIndex)];
            const float top = content.top + static_cast<float>(i) * row;
            const float bottom = std::min(content.bottom, top + row);
            const Rect rowRect = Rect::Make(content.left, top, content.right, bottom);

            const bool selected = entry.path == m_selectedPath;
            const bool hovered = m_hasHoveredPath && entry.path == m_hoveredPath;
            if (selected) {
                renderer.FillRoundedRect(rowRect, selectedBackground, ScaleValue(4.0f));
            } else if (hovered) {
                renderer.FillRoundedRect(rowRect, hoverBackground, ScaleValue(4.0f));
            }

            const float indent = ScaleValue(indentWidth) * static_cast<float>(entry.depth);
            const float glyphLeft = content.left + ScaleValue(4.0f) + indent;
            const float glyphCenterY = top + row * 0.5f;

            if (!entry.node->children.empty()) {
                const Rect glyphRect = ToggleGlyphRect(entry.depth, globalIndex);
                renderer.DrawRect(glyphRect, glyphColor, 1.0f);
                renderer.DrawLine(
                    PointF{glyphRect.left + ScaleValue(2.0f), glyphCenterY},
                    PointF{glyphRect.right - ScaleValue(2.0f), glyphCenterY},
                    glyphColor,
                    1.0f);
                if (!entry.node->expanded) {
                    renderer.DrawLine(
                        PointF{glyphRect.left + glyphRect.Width() * 0.5f, glyphRect.top + ScaleValue(2.0f)},
                        PointF{glyphRect.left + glyphRect.Width() * 0.5f, glyphRect.bottom - ScaleValue(2.0f)},
                        glyphColor,
                        1.0f);
                }
            }

            const float textLeft = glyphLeft + ScaleValue(16.0f);
            const Rect textRect = Rect::Make(textLeft, top, content.right - ScaleValue(6.0f), bottom);
            const TextRenderOptions textOptions{
                TextWrapMode::NoWrap,
                TextHorizontalAlign::Start,
                TextVerticalAlign::Center
            };
            renderer.DrawTextW(
                entry.node->label.c_str(),
                static_cast<UINT32>(entry.node->label.size()),
                textRect,
                textColor,
                ScaleValue(fontSize),
                textOptions);
        }

        if (visibleRows < static_cast<int>(visible.size())) {
            const float thumbWidth = ScaleValue(4.0f);
            const float thumbRight = m_bounds.right - ScaleValue(3.0f);
            const float thumbLeft = thumbRight - thumbWidth;
            const float ratio = static_cast<float>(visibleRows) / static_cast<float>(visible.size());
            const float thumbHeight = std::max(ScaleValue(16.0f), content.Height() * ratio);
            const float maxOffset = static_cast<float>(std::max(1, static_cast<int>(visible.size()) - visibleRows));
            const float offsetRatio = static_cast<float>(m_scrollOffset) / maxOffset;
            const float thumbTop = content.top + (content.Height() - thumbHeight) * offsetRatio;
            const Rect thumbRect = Rect::Make(thumbLeft, thumbTop, thumbRight, thumbTop + thumbHeight);
            renderer.FillRoundedRect(thumbRect, scrollThumbColor, ScaleValue(2.0f));
        }

        renderer.PopLayer();
    }

    void SetPaths(std::vector<std::wstring> paths) {
        m_roots.clear();
        for (const auto& path : paths) {
            AddPath(path);
        }
        if (!m_roots.empty()) {
            m_selectedPath = std::vector<int>{0};
        } else {
            m_selectedPath.clear();
        }
        m_scrollOffset = 0;
    }

    void SetExpandedAll(bool expanded) {
        for (auto& root : m_roots) {
            SetExpandedAllRecursive(root, expanded);
        }
    }

    Color background = ColorFromHex(0x101B28);
    Color borderColor = ColorFromHex(0x35506A);
    Color focusBorderColor = ColorFromHex(0xFFFFFF);
    Color selectedBackground = ColorFromHex(0x2E577D);
    Color hoverBackground = ColorFromHex(0x1F3B55);
    Color textColor = ColorFromHex(0xDCE8F5);
    Color mutedTextColor = ColorFromHex(0x96A9BC);
    Color glyphColor = ColorFromHex(0xAFC3D9);
    Color scrollThumbColor = ColorFromHex(0x59728C);
    float cornerRadius = 10.0f;
    float fontSize = 13.0f;
    float itemHeight = 28.0f;
    float indentWidth = 18.0f;

private:
    struct Node {
        std::wstring label;
        std::vector<Node> children;
        bool expanded = true;
    };

    struct VisibleNode {
        Node* node = nullptr;
        int depth = 0;
        std::vector<int> path;
    };

    static std::wstring TrimWide(const std::wstring& value) {
        size_t start = 0;
        while (start < value.size() && (value[start] == L' ' || value[start] == L'\t' || value[start] == L'\r' || value[start] == L'\n')) {
            ++start;
        }
        size_t end = value.size();
        while (end > start && (value[end - 1] == L' ' || value[end - 1] == L'\t' || value[end - 1] == L'\r' || value[end - 1] == L'\n')) {
            --end;
        }
        return value.substr(start, end - start);
    }

    static std::vector<std::wstring> SplitPath(const std::wstring& path) {
        std::vector<std::wstring> segments;
        std::wstring current;
        for (wchar_t ch : path) {
            if (ch == L'/' || ch == L'>' || ch == L'\\') {
                const std::wstring trimmed = TrimWide(current);
                if (!trimmed.empty()) {
                    segments.push_back(trimmed);
                }
                current.clear();
            } else {
                current.push_back(ch);
            }
        }
        const std::wstring trimmed = TrimWide(current);
        if (!trimmed.empty()) {
            segments.push_back(trimmed);
        }
        return segments;
    }

    void AddPath(const std::wstring& path) {
        const auto segments = SplitPath(path);
        if (segments.empty()) {
            return;
        }

        std::vector<Node>* level = &m_roots;
        for (const auto& segment : segments) {
            auto it = std::find_if(level->begin(), level->end(), [&](const Node& node) {
                return node.label == segment;
            });
            if (it == level->end()) {
                level->push_back(Node{segment, {}, true});
                it = level->end() - 1;
            }
            level = &it->children;
        }
    }

    void SetExpandedAllRecursive(Node& node, bool expanded) {
        node.expanded = expanded;
        for (auto& child : node.children) {
            SetExpandedAllRecursive(child, expanded);
        }
    }

    Rect ContentRect() const {
        const float inset = ScaleValue(4.0f);
        return Rect::Make(
            m_bounds.left + inset,
            m_bounds.top + inset,
            m_bounds.right - inset,
            m_bounds.bottom - inset);
    }

    Rect ToggleGlyphRect(int depth, int globalIndex) const {
        const Rect content = ContentRect();
        const float row = std::max(1.0f, ScaleValue(itemHeight));
        const int localIndex = globalIndex - m_scrollOffset;
        const float rowTop = content.top + static_cast<float>(localIndex) * row;
        const float centerY = rowTop + row * 0.5f;
        const float left = content.left + ScaleValue(4.0f) + ScaleValue(indentWidth) * static_cast<float>(depth);
        return Rect::Make(left, centerY - ScaleValue(5.0f), left + ScaleValue(10.0f), centerY + ScaleValue(5.0f));
    }

    std::vector<VisibleNode> BuildVisibleNodes() {
        std::vector<VisibleNode> result;
        std::vector<int> path;
        for (size_t i = 0; i < m_roots.size(); ++i) {
            path.clear();
            path.push_back(static_cast<int>(i));
            AppendVisibleNode(m_roots[i], 0, path, result);
        }
        return result;
    }

    void AppendVisibleNode(Node& node, int depth, std::vector<int>& path, std::vector<VisibleNode>& out) {
        out.push_back(VisibleNode{&node, depth, path});
        if (!node.expanded) {
            return;
        }
        for (size_t i = 0; i < node.children.size(); ++i) {
            path.push_back(static_cast<int>(i));
            AppendVisibleNode(node.children[i], depth + 1, path, out);
            path.pop_back();
        }
    }

    Node* GetNode(const std::vector<int>& path) {
        if (path.empty()) {
            return nullptr;
        }
        if (path[0] < 0 || path[0] >= static_cast<int>(m_roots.size())) {
            return nullptr;
        }

        Node* node = &m_roots[static_cast<size_t>(path[0])];
        for (size_t i = 1; i < path.size(); ++i) {
            const int idx = path[i];
            if (idx < 0 || idx >= static_cast<int>(node->children.size())) {
                return nullptr;
            }
            node = &node->children[static_cast<size_t>(idx)];
        }
        return node;
    }

    int SelectedVisibleIndex(const std::vector<VisibleNode>& visible) const {
        for (size_t i = 0; i < visible.size(); ++i) {
            if (visible[i].path == m_selectedPath) {
                return static_cast<int>(i);
            }
        }
        return -1;
    }

    int VisibleRowCount(int totalVisible) const {
        const Rect content = ContentRect();
        const float row = std::max(1.0f, ScaleValue(itemHeight));
        const int count = std::max(1, static_cast<int>(std::floor(content.Height() / row)));
        return std::min(count, std::max(1, totalVisible));
    }

    void EnsureSelectedVisible(const std::vector<VisibleNode>& visible) {
        if (visible.empty()) {
            m_scrollOffset = 0;
            return;
        }
        const int index = SelectedVisibleIndex(visible);
        if (index < 0) {
            return;
        }
        const int visibleRows = VisibleRowCount(static_cast<int>(visible.size()));
        if (index < m_scrollOffset) {
            m_scrollOffset = index;
        } else if (index >= m_scrollOffset + visibleRows) {
            m_scrollOffset = index - visibleRows + 1;
        }
        const int maxStart = std::max(0, static_cast<int>(visible.size()) - visibleRows);
        m_scrollOffset = std::clamp(m_scrollOffset, 0, maxStart);
    }

    int VisibleIndexFromPoint(float y, int totalVisible) const {
        const Rect content = ContentRect();
        if (y < content.top || y > content.bottom) {
            return -1;
        }
        const float row = std::max(1.0f, ScaleValue(itemHeight));
        const int local = static_cast<int>(std::floor((y - content.top) / row));
        const int visibleRows = VisibleRowCount(totalVisible);
        if (local < 0 || local >= visibleRows) {
            return -1;
        }
        return local;
    }

    bool SetSelectedPath(std::vector<int> path) {
        if (path == m_selectedPath) {
            return false;
        }
        m_selectedPath = std::move(path);
        return true;
    }

    std::vector<Node> m_roots;
    std::vector<int> m_selectedPath;
    std::vector<int> m_hoveredPath;
    bool m_hasHoveredPath = false;
    int m_scrollOffset = 0;
};
