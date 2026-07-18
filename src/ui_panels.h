#pragma once
// Component module - part of the ai-win-ui library.
// See ui.h for the full umbrella include.
#include "ui_element.h"

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
    // Wave2 R7: optional panel-level gradient / soft shadow (also via style.decoration).
    LinearGradientSpec gradient{};
    SoftShadowSpec shadow{};
    Direction direction = Direction::Column;
    Wrap wrap = Wrap::NoWrap;
    AlignItems alignItems = AlignItems::Stretch;
    JustifyContent justifyContent = JustifyContent::Start;

    // S4: when background came from $color.key, rebind on theme switch.
    void SetBackgroundToken(std::string key) { m_backgroundToken = std::move(key); }
    void ApplyThemeDefaults() override {
        if (m_backgroundToken.empty() || !m_context || !m_context->theme) {
            return;
        }
        if (auto c = m_context->theme->ResolveColor(m_backgroundToken)) {
            background = *c;
        }
    }

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
        Color drawBackground = background;
        Color drawBorderColor = borderColor;
        float drawCornerRadius = cornerRadius;
        Thickness drawBorder = Border();
        LinearGradientSpec drawGradient = gradient;
        SoftShadowSpec drawShadow = shadow;

        // Catalog / layout styles: style="$style.surfaceCard" stores decoration in
        // m_style; fold it into paint when the panel was given a layout style.
        if (m_styleFromLayout) {
            const StyleSpec shell = m_style.Resolve(StyleState::Normal);
            if (shell.decoration.has_value()) {
                if (shell.decoration->background.a > 0.0f || drawBackground.a <= 0.0f) {
                    drawBackground = shell.decoration->background;
                }
                if (shell.decoration->border.color.a > 0.0f) {
                    drawBorderColor = shell.decoration->border.color;
                }
                const Thickness bw = shell.decoration->border.width;
                if (bw.left > 0.0f || bw.top > 0.0f || bw.right > 0.0f || bw.bottom > 0.0f) {
                    drawBorder = bw;
                }
                if (!shell.decoration->radius.IsZero()) {
                    drawCornerRadius = shell.decoration->radius.MaxRadius();
                }
                if (shell.decoration->gradient.enabled) {
                    drawGradient = shell.decoration->gradient;
                }
                if (shell.decoration->shadow.enabled) {
                    drawShadow = shell.decoration->shadow;
                }
            }
        }

        const Thickness scaledBorder = ScaleThickness(drawBorder);
        const bool hasBorder =
            scaledBorder.left > 0.0f || scaledBorder.top > 0.0f ||
            scaledBorder.right > 0.0f || scaledBorder.bottom > 0.0f;
        const float scaledRadius = ScaleValue(drawCornerRadius);
        const bool hasFill = drawBackground.a > 0.0f || drawGradient.enabled || drawShadow.enabled;
        if (hasFill || hasBorder) {
            BoxDecoration deco;
            deco.background = drawBackground;
            deco.border.width = scaledBorder;
            deco.border.color = drawBorderColor;
            deco.radius = CornerRadius::Uniform(scaledRadius);
            deco.gradient = drawGradient;
            if (drawGradient.enabled) {
                // Scale does not apply to colors; angle is unitless.
            }
            deco.shadow = drawShadow;
            if (deco.shadow.enabled) {
                deco.shadow.offsetX = ScaleValue(deco.shadow.offsetX);
                deco.shadow.offsetY = ScaleValue(deco.shadow.offsetY);
                deco.shadow.blur = ScaleValue(deco.shadow.blur);
            }
            DrawBoxDecoration(renderer, m_bounds, deco);
        }
        // Do not clip children to the panel radius. Overlay controls (e.g. ComboBox
        // dropdown) must be able to paint outside their layout bounds. Background is
        // already rounded via DrawBoxDecoration / FillRoundedRect above.
        for (auto& child : m_children) {
            child->Render(renderer);
        }
    }

    std::string m_backgroundToken;

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

// Uniform multi-column grid. Prefer Yoga flex-wrap via ILayoutEngine::MeasureGrid/ArrangeGrid.
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

        if (m_context && m_context->layoutEngine) {
            m_context->layoutEngine->ArrangeGrid(ToGridLayoutStyle(), BuildGridChildren(), m_bounds);
            return;
        }
        ArrangeFallback();
    }

    void Measure(float availableWidth, float availableHeight) override {
        const float resolvedWidth = GetPreferredWidth(availableWidth);

        if (m_context && m_context->layoutEngine) {
            const Size measured = m_context->layoutEngine->MeasureGrid(
                ToGridLayoutStyle(),
                BuildGridChildren(),
                resolvedWidth,
                availableHeight);
            m_desiredSize.width = resolvedWidth;
            m_desiredSize.height = HasFixedHeight()
                ? GetPreferredHeight(resolvedWidth)
                : ClampHeight(std::max(0.0f, measured.height));
            return;
        }

        MeasureFallback(resolvedWidth, availableHeight);
    }

protected:
    float MeasurePreferredHeight(float width) const override {
        if (m_context && m_context->layoutEngine) {
            const Size measured = m_context->layoutEngine->MeasureGrid(
                ToGridLayoutStyle(),
                BuildGridChildren(),
                width > 0.0f ? width : 4096.0f,
                100000.0f);
            return measured.height;
        }
        return MeasurePreferredHeightFallback(width);
    }

    GridLayoutStyle ToGridLayoutStyle() const {
        const Thickness scaledPadding = ScaleThickness(padding);
        const Thickness scaledBorder = ScaleThickness(Border());
        return GridLayoutStyle{
            std::max(1, columns),
            LayoutSpacing{scaledPadding.left, scaledPadding.top, scaledPadding.right, scaledPadding.bottom},
            LayoutSpacing{scaledBorder.left, scaledBorder.top, scaledBorder.right, scaledBorder.bottom},
            ScaleValue(cellSpacing),
            ScaleValue(rowHeight)
        };
    }

    std::vector<GridLayoutChild> BuildGridChildren() const {
        std::vector<GridLayoutChild> children;
        children.reserve(m_children.size());
        for (const auto& child : m_children) {
            const Thickness margin = child->ScaleThickness(child->Margin());
            const Thickness border = child->ScaleThickness(child->Border());
            children.push_back(GridLayoutChild{
                child.get(),
                LayoutSpacing{margin.left, margin.top, margin.right, margin.bottom},
                LayoutSpacing{border.left, border.top, border.right, border.bottom}
            });
        }
        return children;
    }

    void MeasureFallback(float resolvedWidth, float availableHeight) {
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

    void ArrangeFallback() {
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

    float MeasurePreferredHeightFallback(float width) const {
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
        } else if (background.a > 0.0f) {
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
    const std::wstring& Text() const { return m_text; }
    float FontSize() const { return m_fontSize; }
    void SetColorToken(std::string key) { m_colorToken = std::move(key); }
    void SetEllipsis(bool enabled) { m_ellipsis = enabled; }
    bool Ellipsis() const { return m_ellipsis; }

    void ApplyThemeDefaults() override {
        if (m_colorToken.empty() || !m_context || !m_context->theme) {
            return;
        }
        if (auto c = m_context->theme->ResolveColor(m_colorToken)) {
            m_color = *c;
        }
    }

protected:
    float MeasurePreferredWidth(float availableWidth) const override {
        const TextWrapMode wrap = m_ellipsis ? TextWrapMode::NoWrap : TextWrapMode::Wrap;
        const Size measured = MeasureTextValue(
            m_text, m_fontSize, availableWidth > 0.0f ? availableWidth : 4096.0f, wrap);
        return std::min(availableWidth, measured.width + 4.0f);
    }

    float MeasurePreferredHeight(float width) const override {
        const TextWrapMode wrap = m_ellipsis ? TextWrapMode::NoWrap : TextWrapMode::Wrap;
        const Size measured = MeasureTextValue(
            m_text, m_fontSize, width > 0.0f ? width : 4096.0f, wrap);
        return measured.height + 8.0f;
    }

public:
    void Render(IRenderer& renderer) override {
        TextRenderOptions textOptions{};
        textOptions.wrap = m_ellipsis ? TextWrapMode::NoWrap : TextWrapMode::Wrap;
        textOptions.horizontalAlign = TextHorizontalAlign::Start;
        textOptions.verticalAlign = TextVerticalAlign::Start;
        textOptions.ellipsis = m_ellipsis;
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
    std::string m_colorToken;
    bool m_ellipsis = false;
};

class StatCard : public UIElement {
public:
    StatCard() { m_style = DefaultStyle(nullptr); }

    static ComponentStyle DefaultStyle(const Theme* theme = nullptr) {
        ComponentStyle s;
        BoxDecoration shell;
        shell.background = ComponentStyle::ThemeColor(theme, "surface-2", ColorFromHex(0x1E2630));
        shell.border.width = Thickness{1, 1, 1, 1};
        shell.border.color = ComponentStyle::ThemeColor(theme, "border-subtle", ColorFromHex(0x2F3A46));
        shell.radius = CornerRadius::Uniform(
            ComponentStyle::ThemeNumber(theme, Theme::NumberCategory::Radius, "lg", 10.0f));
        s.base.decoration = shell;
        s.base.foreground = ComponentStyle::ThemeColor(theme, "fg-strong", ColorFromHex(0xFFFFFF));
        s.base.fontSize = ComponentStyle::ThemeNumber(theme, Theme::NumberCategory::FontSize, "2xl", 24.0f);
        // Accent bar uses fill; muted title uses hover.foreground convention.
        BoxDecoration accent;
        accent.background = ComponentStyle::ThemeColor(theme, "accent", ColorFromHex(0x4E7BFF));
        s.base.fill = accent;
        s.overrides[static_cast<std::size_t>(StyleState::Hover)].foreground =
            ComponentStyle::ThemeColor(theme, "fg-muted", ColorFromHex(0xBFD1E3));
        s.overrides[static_cast<std::size_t>(StyleState::Selected)].foreground =
            ComponentStyle::ThemeColor(theme, "success", ColorFromHex(0x8ED1A5));
        return s;
    }

    void ApplyThemeDefaults() override {
        if (m_styleFromLayout) {
            return;
        }
        const Theme* theme = (m_context && m_context->theme) ? m_context->theme : nullptr;
        m_style = DefaultStyle(theme);
        // Keep layout-authored colors (e.g. per-card accent); only fill still-default fields.
        SyncDefaultsFromStyle();
    }

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
        // Public color fields are authoritative (theme-filled when still default; layout may override).
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
    static bool ColorsNearlyEqual(const Color& a, const Color& b) {
        return std::abs(a.r - b.r) < 0.002f && std::abs(a.g - b.g) < 0.002f &&
               std::abs(a.b - b.b) < 0.002f && std::abs(a.a - b.a) < 0.002f;
    }

    void SyncDefaultsFromStyle() {
        // Factory (pre-theme) defaults used to detect layout overrides.
        static const Color kBg = ColorFromHex(0x1E2630);
        static const Color kBorder = ColorFromHex(0x2F3A46);
        static const Color kAccent = ColorFromHex(0x4E7BFF);
        static const Color kTitle = ColorFromHex(0xBFD1E3);
        static const Color kValue = ColorFromHex(0xFFFFFF);
        static const Color kDelta = ColorFromHex(0x8ED1A5);

        const StyleSpec shell = m_style.Resolve(StyleState::Normal);
        const StyleSpec titleSpec = m_style.Resolve(StyleState::Hover);
        const StyleSpec deltaSpec = m_style.Resolve(StyleState::Selected);

        if (ColorsNearlyEqual(background, kBg) && shell.decoration.has_value()) {
            background = shell.decoration->background;
            borderColor = shell.decoration->border.color;
            if (!shell.decoration->radius.IsZero()) {
                cornerRadius = shell.decoration->radius.MaxRadius();
            }
        } else if (ColorsNearlyEqual(borderColor, kBorder) && shell.decoration.has_value()) {
            borderColor = shell.decoration->border.color;
        }
        if (ColorsNearlyEqual(accentColor, kAccent) && shell.fill.has_value()) {
            accentColor = shell.fill->background;
        }
        if (ColorsNearlyEqual(valueColor, kValue) && shell.foreground.has_value()) {
            valueColor = *shell.foreground;
        }
        if (shell.fontSize.has_value() && std::abs(valueFontSize - 24.0f) < 0.01f) {
            valueFontSize = *shell.fontSize;
        }
        if (ColorsNearlyEqual(titleColor, kTitle) && titleSpec.foreground.has_value()) {
            titleColor = *titleSpec.foreground;
        }
        if (ColorsNearlyEqual(deltaColor, kDelta) && deltaSpec.foreground.has_value()) {
            deltaColor = *deltaSpec.foreground;
        }
    }

    std::wstring m_title = L"Metric";
    std::wstring m_value = L"0";
    std::wstring m_deltaText = L"+0%";
};

class SparklineChart : public UIElement {
public:
    SparklineChart() { m_style = DefaultStyle(nullptr); }

    static ComponentStyle DefaultStyle(const Theme* theme = nullptr) {
        ComponentStyle s;
        BoxDecoration shell;
        shell.background = ComponentStyle::ThemeColor(theme, "surface-1", ColorFromHex(0x121B25));
        shell.border.width = Thickness{1, 1, 1, 1};
        shell.border.color = ComponentStyle::ThemeColor(theme, "border-subtle", ColorFromHex(0x2C3A47));
        shell.radius = CornerRadius::Uniform(
            ComponentStyle::ThemeNumber(theme, Theme::NumberCategory::Radius, "lg", 10.0f));
        s.base.decoration = shell;
        // Line series uses fill; baseline uses track.
        BoxDecoration line;
        line.background = ComponentStyle::ThemeColor(theme, "accent", ColorFromHex(0x53B3FF));
        s.base.fill = line;
        BoxDecoration baseline;
        baseline.background = ComponentStyle::ThemeColor(theme, "border-subtle", ColorFromHex(0x314252));
        s.base.track = baseline;
        return s;
    }

    void ApplyThemeDefaults() override {
        if (m_styleFromLayout) {
            return;
        }
        const Theme* theme = (m_context && m_context->theme) ? m_context->theme : nullptr;
        m_style = DefaultStyle(theme);
        SyncDefaultsFromStyle();
    }

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
    static bool ColorsNearlyEqual(const Color& a, const Color& b) {
        return std::abs(a.r - b.r) < 0.002f && std::abs(a.g - b.g) < 0.002f &&
               std::abs(a.b - b.b) < 0.002f && std::abs(a.a - b.a) < 0.002f;
    }

    void SyncDefaultsFromStyle() {
        static const Color kBg = ColorFromHex(0x121B25);
        static const Color kBorder = ColorFromHex(0x2C3A47);
        static const Color kLine = ColorFromHex(0x53B3FF);
        static const Color kBase = ColorFromHex(0x314252);

        const StyleSpec shell = m_style.Resolve(StyleState::Normal);
        if (ColorsNearlyEqual(background, kBg) && shell.decoration.has_value()) {
            background = shell.decoration->background;
            borderColor = shell.decoration->border.color;
            if (!shell.decoration->radius.IsZero()) {
                cornerRadius = shell.decoration->radius.MaxRadius();
            }
        } else if (ColorsNearlyEqual(borderColor, kBorder) && shell.decoration.has_value()) {
            borderColor = shell.decoration->border.color;
        }
        if (ColorsNearlyEqual(lineColor, kLine) && shell.fill.has_value()) {
            lineColor = shell.fill->background;
        }
        if (ColorsNearlyEqual(baselineColor, kBase) && shell.track.has_value()) {
            baselineColor = shell.track->background;
        }
    }

    std::vector<float> m_points;
    bool m_manualRange = false;
    float m_minValue = 0.0f;
    float m_maxValue = 1.0f;
};

// Filled parametric polygon (heart / petal / oval / star) for decorative shapes
// and irregular layered window bodies. Uses IRenderer::FillPolygon.
class ShapePanel : public UIElement {
public:
    enum class Kind {
        Heart,
        Petal,
        Oval,
        Star
    };

    Kind kind = Kind::Heart;
    Color fill = ColorFromHex(0xE85D75);
    Color stroke = ColorFromHex(0x000000, 0.0f);
    float strokeWidth = 0.0f;
    int segments = 96;
    Color textColor = ColorFromHex(0xFFFFFF);
    float fontSize = 14.0f;

    void SetText(std::wstring text) { m_text = std::move(text); }
    const std::wstring& Text() const { return m_text; }

    void SetKindFromString(const std::string& value) {
        std::string lower = value;
        for (char& ch : lower) {
            if (ch >= 'A' && ch <= 'Z') {
                ch = static_cast<char>(ch - 'A' + 'a');
            }
        }
        if (lower == "petal" || lower == "flower") {
            kind = Kind::Petal;
        } else if (lower == "oval" || lower == "ellipse" || lower == "circle") {
            kind = Kind::Oval;
        } else if (lower == "star") {
            kind = Kind::Star;
        } else {
            kind = Kind::Heart;
        }
    }

protected:
    float MeasurePreferredWidth(float availableWidth) const override {
        return std::min(availableWidth, ScaleValue(220.0f));
    }

    float MeasurePreferredHeight(float width) const override {
        (void)width;
        return ScaleValue(200.0f);
    }

public:
    void Render(IRenderer& renderer) override {
        if (m_bounds.Width() <= 1.0f || m_bounds.Height() <= 1.0f) {
            return;
        }

        const int count = std::clamp(segments, 12, 256);
        std::vector<PointF> poly = BuildPolygon(m_bounds, kind, count);
        if (poly.size() >= 3 && fill.a > 0.0f) {
            renderer.FillPolygon(poly, fill);
        }
        if (strokeWidth > 0.0f && stroke.a > 0.0f && poly.size() >= 2) {
            // Close the outline for DrawPolyline consumers.
            poly.push_back(poly.front());
            renderer.DrawPolyline(poly, stroke, ScaleValue(strokeWidth));
        }

        if (!m_text.empty()) {
            const float scaledFont = ScaleValue(fontSize);
            const float textH = scaledFont * 1.35f;
            const float pad = ScaleValue(12.0f);
            const Rect textRect = Rect::Make(
                m_bounds.left + pad,
                m_bounds.top + (m_bounds.Height() - textH) * 0.5f,
                m_bounds.right - pad,
                m_bounds.top + (m_bounds.Height() - textH) * 0.5f + textH);
            const TextRenderOptions opts{
                TextWrapMode::NoWrap,
                TextHorizontalAlign::Center,
                TextVerticalAlign::Center,
                false,
                false
            };
            renderer.DrawTextW(m_text.c_str(), static_cast<UINT32>(m_text.size()), textRect, textColor, scaledFont, opts);
        }

        for (auto& child : m_children) {
            child->Render(renderer);
        }
    }

private:
    static std::vector<PointF> BuildPolygon(const Rect& bounds, Kind kind, int segments) {
        std::vector<PointF> points;
        points.reserve(static_cast<size_t>(segments) + 2);

        const float cx = bounds.left + bounds.Width() * 0.5f;
        const float cy = bounds.top + bounds.Height() * 0.5f;
        const float halfW = bounds.Width() * 0.5f;
        const float halfH = bounds.Height() * 0.5f;
        constexpr float kPi = 3.14159265358979323846f;

        auto push = [&](float nx, float ny) {
            // nx, ny in [-1, 1] relative to center.
            points.push_back(PointF{cx + nx * halfW, cy + ny * halfH});
        };

        if (kind == Kind::Oval) {
            for (int i = 0; i < segments; ++i) {
                const float t = (static_cast<float>(i) / static_cast<float>(segments)) * 2.0f * kPi;
                push(std::cos(t), std::sin(t));
            }
            return points;
        }

        if (kind == Kind::Heart) {
            // Classic heart parametric curve, normalized into [-1, 1].
            float minX = 0.0f, maxX = 0.0f, minY = 0.0f, maxY = 0.0f;
            std::vector<PointF> raw;
            raw.reserve(static_cast<size_t>(segments));
            for (int i = 0; i < segments; ++i) {
                const float t = (static_cast<float>(i) / static_cast<float>(segments)) * 2.0f * kPi;
                const float st = std::sin(t);
                const float x = 16.0f * st * st * st;
                const float y = 13.0f * std::cos(t) - 5.0f * std::cos(2.0f * t)
                    - 2.0f * std::cos(3.0f * t) - std::cos(4.0f * t);
                raw.push_back(PointF{x, -y}); // flip Y so tip points down-ish like a card
                if (i == 0) {
                    minX = maxX = x;
                    minY = maxY = -y;
                } else {
                    minX = std::min(minX, x);
                    maxX = std::max(maxX, x);
                    minY = std::min(minY, -y);
                    maxY = std::max(maxY, -y);
                }
            }
            const float spanX = std::max(0.001f, maxX - minX);
            const float spanY = std::max(0.001f, maxY - minY);
            for (const auto& p : raw) {
                const float nx = ((p.x - minX) / spanX) * 2.0f - 1.0f;
                const float ny = ((p.y - minY) / spanY) * 2.0f - 1.0f;
                push(nx * 0.92f, ny * 0.92f);
            }
            return points;
        }

        if (kind == Kind::Petal) {
            // 5-petal rose: r = cos(k theta), mapped into unit box.
            constexpr int kPetals = 5;
            float minX = 0.0f, maxX = 0.0f, minY = 0.0f, maxY = 0.0f;
            std::vector<PointF> raw;
            raw.reserve(static_cast<size_t>(segments));
            for (int i = 0; i < segments; ++i) {
                const float t = (static_cast<float>(i) / static_cast<float>(segments)) * 2.0f * kPi;
                const float r = std::cos(static_cast<float>(kPetals) * t);
                const float x = r * std::cos(t);
                const float y = r * std::sin(t);
                raw.push_back(PointF{x, y});
                if (i == 0) {
                    minX = maxX = x;
                    minY = maxY = y;
                } else {
                    minX = std::min(minX, x);
                    maxX = std::max(maxX, x);
                    minY = std::min(minY, y);
                    maxY = std::max(maxY, y);
                }
            }
            const float spanX = std::max(0.001f, maxX - minX);
            const float spanY = std::max(0.001f, maxY - minY);
            for (const auto& p : raw) {
                const float nx = ((p.x - minX) / spanX) * 2.0f - 1.0f;
                const float ny = ((p.y - minY) / spanY) * 2.0f - 1.0f;
                push(nx * 0.95f, ny * 0.95f);
            }
            return points;
        }

        // Star (5-point)
        {
            constexpr int kPoints = 5;
            const float outer = 1.0f;
            const float inner = 0.42f;
            for (int i = 0; i < kPoints * 2; ++i) {
                const float ang = -kPi * 0.5f + (static_cast<float>(i) * kPi / static_cast<float>(kPoints));
                const float r = (i % 2 == 0) ? outer : inner;
                push(std::cos(ang) * r, std::sin(ang) * r);
            }
            return points;
        }
    }

    std::wstring m_text;
};

// Interactive data table: selection, sort, multi-select, edit, resize, freeze, callbacks.
class DataTable : public UIElement {
public:
    struct Column {
        std::wstring title;
        float width = -1.0f;
    };

    DataTable() {
        m_style = DefaultStyle(nullptr);
    }

    static ComponentStyle DefaultStyle(const Theme* theme = nullptr) {
        ComponentStyle s;
        BoxDecoration shell;
        shell.background = ComponentStyle::ThemeColor(theme, "surface-1", ColorFromHex(0x121A24));
        shell.border.width = Thickness{1, 1, 1, 1};
        shell.border.color = ComponentStyle::ThemeColor(theme, "border-subtle", ColorFromHex(0x304053));
        shell.radius = CornerRadius::Uniform(
            ComponentStyle::ThemeNumber(theme, Theme::NumberCategory::Radius, "lg", 10.0f));
        s.base.decoration = shell;
        s.base.foreground = ComponentStyle::ThemeColor(theme, "fg", ColorFromHex(0xC7D4E2));
        s.base.fontSize = ComponentStyle::ThemeNumber(theme, Theme::NumberCategory::FontSize, "sm", 13.0f);
        return s;
    }

    void ApplyThemeDefaults() override {
        if (m_styleFromLayout) {
            return;
        }
        const Theme* theme = (m_context && m_context->theme) ? m_context->theme : nullptr;
        m_style = DefaultStyle(theme);
        if (m_style.base.decoration.has_value()) {
            background = m_style.base.decoration->background;
            borderColor = m_style.base.decoration->border.color;
            if (!m_style.base.decoration->radius.IsZero()) {
                cornerRadius = m_style.base.decoration->radius.MaxRadius();
            }
        }
        if (m_style.base.foreground.has_value()) {
            textColor = *m_style.base.foreground;
        }
        if (m_style.base.fontSize.has_value()) {
            fontSize = *m_style.base.fontSize;
        }
    }

    void SetColumns(std::vector<Column> columns) {
        CommitEdit(false);
        m_columns = std::move(columns);
        if (m_sortColumn >= static_cast<int>(m_columns.size())) {
            m_sortColumn = -1;
        }
        RebuildViewOrder();
    }
    void SetRows(std::vector<std::vector<std::wstring>> rows) {
        CommitEdit(false);
        m_rows = std::move(rows);
        if (m_selectedIndex >= static_cast<int>(m_rows.size())) {
            m_selectedIndex = m_rows.empty() ? -1 : 0;
        }
        SanitizeSelection();
        RebuildViewOrder();
    }
    void SetColumnWidths(std::vector<float> widths) {
        const size_t count = std::min(widths.size(), m_columns.size());
        for (size_t i = 0; i < count; ++i) {
            m_columns[i].width = widths[i];
        }
    }
    void SetSelectable(bool selectable) { m_selectable = selectable; }
    void SetSortable(bool sortable) { m_sortable = sortable; }
    void SetMultiSelect(bool multi) {
        m_multiSelect = multi;
        if (!multi) {
            m_selectedSet.clear();
            if (m_selectedIndex >= 0) {
                m_selectedSet.insert(m_selectedIndex);
            }
        }
    }
    void SetEditable(bool editable) {
        if (!editable) {
            CommitEdit(false);
        }
        m_editable = editable;
    }
    void SetResizableColumns(bool resizable) {
        if (!resizable) {
            EndColumnResize();
        }
        m_resizableColumns = resizable;
    }
    void SetFrozenColumnCount(int count) {
        m_frozenColumnCount = std::max(0, count);
        ClampHorizontalScroll();
    }
    int FrozenColumnCount() const { return m_frozenColumnCount; }

    void SetOnSelectionChanged(std::function<void(const std::vector<int>&)> callback) {
        m_onSelectionChanged = std::move(callback);
    }
    void SetOnCellChanged(std::function<void(int row, int col, const std::wstring& value)> callback) {
        m_onCellChanged = std::move(callback);
    }

    bool MultiSelect() const { return m_multiSelect; }
    bool Editable() const { return m_editable; }
    bool ResizableColumns() const { return m_resizableColumns; }

    void SetSelectedIndex(int index) {
        if (m_rows.empty()) {
            m_selectedIndex = -1;
            m_selectedSet.clear();
            NotifySelectionChanged();
            return;
        }
        m_selectedIndex = std::clamp(index, 0, static_cast<int>(m_rows.size()) - 1);
        m_selectedSet.clear();
        m_selectedSet.insert(m_selectedIndex);
        m_selectionAnchor = m_selectedIndex;
        EnsureSelectedVisible();
        NotifySelectionChanged();
    }
    int SelectedIndex() const { return m_selectedIndex; }
    std::vector<int> SelectedIndices() const {
        return std::vector<int>(m_selectedSet.begin(), m_selectedSet.end());
    }
    bool IsRowSelected(int dataRow) const {
        return m_selectedSet.find(dataRow) != m_selectedSet.end();
    }

    bool IsFocusable() const override { return m_selectable || m_sortable || m_editable || m_resizableColumns; }

    bool OnFocus() override {
        if (!m_hasFocus) {
            m_hasFocus = true;
            return true;
        }
        return false;
    }

    bool OnBlur() override {
        bool changed = false;
        if (m_editing) {
            CommitEdit(true);
            changed = true;
        }
        if (m_resizingColumn >= 0) {
            EndColumnResize();
            changed = true;
        }
        if (!m_hasFocus && m_hoveredRow < 0 && m_hoveredHeader < 0 && m_hoveredResizeEdge < 0) {
            return changed;
        }
        m_hasFocus = false;
        m_hoveredRow = -1;
        m_hoveredHeader = -1;
        m_hoveredResizeEdge = -1;
        return true;
    }

    bool OnKeyDown(WPARAM keyCode, LPARAM /*lParam*/) override {
        if (m_editing) {
            switch (keyCode) {
                case VK_ESCAPE:
                    CommitEdit(false);
                    return true;
                case VK_RETURN:
                    CommitEdit(true);
                    return true;
                case VK_LEFT:
                    if (m_editCaret > 0) {
                        --m_editCaret;
                        return true;
                    }
                    return true;
                case VK_RIGHT:
                    if (m_editCaret < static_cast<int>(m_editBuffer.size())) {
                        ++m_editCaret;
                        return true;
                    }
                    return true;
                case VK_HOME:
                    m_editCaret = 0;
                    return true;
                case VK_END:
                    m_editCaret = static_cast<int>(m_editBuffer.size());
                    return true;
                case VK_BACK:
                    if (m_editCaret > 0 && !m_editBuffer.empty()) {
                        m_editBuffer.erase(static_cast<size_t>(m_editCaret - 1), 1);
                        --m_editCaret;
                        return true;
                    }
                    return true;
                case VK_DELETE:
                    if (m_editCaret < static_cast<int>(m_editBuffer.size())) {
                        m_editBuffer.erase(static_cast<size_t>(m_editCaret), 1);
                        return true;
                    }
                    return true;
                default:
                    return false;
            }
        }

        if (!m_selectable || m_rows.empty()) {
            if (m_editable && keyCode == VK_F2 && m_selectedIndex >= 0) {
                return BeginEdit(m_selectedIndex, std::max(0, m_editFocusCol));
            }
            return false;
        }

        const bool shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
        const bool ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;

        // Horizontal pan when columns overflow (Ctrl+Left/Right).
        if (ctrl && (keyCode == VK_LEFT || keyCode == VK_RIGHT)) {
            const float step = ScaleValue(48.0f);
            return ScrollHorizontal(keyCode == VK_LEFT ? -step : step);
        }

        switch (keyCode) {
            case VK_UP:
                return MoveSelection(-1, shift, ctrl);
            case VK_DOWN:
                return MoveSelection(1, shift, ctrl);
            case VK_HOME:
                return SetSelectedIndexInternal(ViewToDataRow(0), shift, ctrl);
            case VK_END:
                return SetSelectedIndexInternal(
                    ViewToDataRow(static_cast<int>(m_viewOrder.size()) - 1), shift, ctrl);
            case VK_PRIOR:
                return MoveSelection(-std::max(1, VisibleRowCount()), shift, ctrl);
            case VK_NEXT:
                return MoveSelection(std::max(1, VisibleRowCount()), shift, ctrl);
            case VK_SPACE:
                if (m_multiSelect && m_selectedIndex >= 0) {
                    ToggleRowSelection(m_selectedIndex);
                    NotifySelectionChanged();
                    return true;
                }
                return false;
            case VK_F2:
                if (m_editable && m_selectedIndex >= 0) {
                    return BeginEdit(m_selectedIndex, std::max(0, m_editFocusCol));
                }
                return false;
            default:
                return false;
        }
    }

    bool OnChar(wchar_t ch) override {
        if (!m_editing) {
            return false;
        }
        if (ch < 32 && ch != L'\t') {
            return false;
        }
        if (ch == L'\t') {
            CommitEdit(true);
            return true;
        }
        m_editBuffer.insert(static_cast<size_t>(m_editCaret), 1, ch);
        ++m_editCaret;
        return true;
    }

    bool OnMouseMove(float x, float y) override {
        if (m_resizingColumn >= 0) {
            const float delta = (x - m_resizeStartX) / std::max(0.001f, DpiScale());
            const float minW = 36.0f;
            m_columns[static_cast<size_t>(m_resizingColumn)].width =
                std::max(minW, m_resizeStartWidth + delta);
            return true;
        }

        if (!HitTest(x, y)) {
            bool changed = false;
            if (m_hoveredRow >= 0 || m_hoveredHeader >= 0 || m_hoveredResizeEdge >= 0) {
                m_hoveredRow = -1;
                m_hoveredHeader = -1;
                m_hoveredResizeEdge = -1;
                changed = true;
            }
            return changed;
        }

        bool changed = false;
        const int resizeEdge = m_resizableColumns ? ResizeEdgeFromPoint(x, y) : -1;
        if (resizeEdge != m_hoveredResizeEdge) {
            m_hoveredResizeEdge = resizeEdge;
            changed = true;
        }

        const int header = (resizeEdge >= 0) ? -1 : HeaderIndexFromPoint(x, y);
        if (header != m_hoveredHeader) {
            m_hoveredHeader = header;
            changed = true;
        }
        const int row = (header >= 0 || resizeEdge >= 0) ? -1 : RowIndexFromPoint(x, y);
        if (row != m_hoveredRow) {
            m_hoveredRow = row;
            changed = true;
        }
        return changed;
    }

    bool OnMouseLeave() override {
        if (m_hoveredRow < 0 && m_hoveredHeader < 0 && m_hoveredResizeEdge < 0 && m_resizingColumn < 0) {
            return false;
        }
        m_hoveredRow = -1;
        m_hoveredHeader = -1;
        m_hoveredResizeEdge = -1;
        return true;
    }

    bool OnMouseDown(float x, float y) override {
        if (!HitTest(x, y)) {
            return false;
        }

        if (m_editing) {
            int row = -1;
            int col = -1;
            if (!CellFromPoint(x, y, row, col) || row != m_editRow || col != m_editCol) {
                CommitEdit(true);
            }
        }

        if (m_resizableColumns) {
            const int edge = ResizeEdgeFromPoint(x, y);
            if (edge >= 0) {
                m_resizingColumn = edge;
                m_resizeStartX = x;
                float startW = m_columns[static_cast<size_t>(edge)].width;
                if (startW <= 0.0f) {
                    const auto widths = ResolveColumnWidths();
                    startW = (edge < static_cast<int>(widths.size()))
                        ? (widths[static_cast<size_t>(edge)] / std::max(0.001f, DpiScale()))
                        : 80.0f;
                }
                m_resizeStartWidth = startW;
                m_columns[static_cast<size_t>(edge)].width = startW;
                return true;
            }
        }

        const int header = HeaderIndexFromPoint(x, y);
        if (header >= 0) {
            if (m_sortable) {
                ToggleSort(header);
            }
            return true;
        }

        int row = -1;
        int col = -1;
        const bool hitCell = CellFromPoint(x, y, row, col);
        if (hitCell && row >= 0) {
            m_editFocusCol = col;

            const DWORD now = GetTickCount();
            const bool isDoubleClick =
                m_editable &&
                row == m_lastClickRow &&
                col == m_lastClickCol &&
                (now - m_lastClickTime) <= GetDoubleClickTime();
            m_lastClickRow = row;
            m_lastClickCol = col;
            m_lastClickTime = now;

            if (isDoubleClick) {
                if (m_selectable) {
                    SetSelectedIndexInternal(row, false, false);
                }
                return BeginEdit(row, col);
            }

            if (m_selectable) {
                const bool shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
                const bool ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
                SetSelectedIndexInternal(row, shift, ctrl);
                return true;
            }
        }
        return true;
    }

    // Nested-scroll: vertical when rows overflow; Shift+wheel pans columns (same as ListView).
    bool OnMouseWheel(float delta, float x, float y, bool shiftHeld) override {
        if (!HitTest(x, y)) {
            return false;
        }
        if (shiftHeld) {
            return ScrollHorizontal(-delta * ScaleValue(48.0f));
        }
        if (m_viewOrder.empty()) {
            return false;
        }
        const int before = m_scrollRow;
        m_scrollRow -= static_cast<int>(std::lround(delta * 3.0f));
        ClampScroll();
        return m_scrollRow != before;
    }

    bool OnMouseUp(float x, float y) override {
        (void)x;
        (void)y;
        if (m_resizingColumn >= 0) {
            EndColumnResize();
            return true;
        }
        return false;
    }

    Color background = ColorFromHex(0x121A24);
    Color borderColor = ColorFromHex(0x304053);
    Color headerBackground = ColorFromHex(0x1F2C3A);
    Color headerHoverBackground = ColorFromHex(0x2A3C4F);
    Color rowBackgroundA = ColorFromHex(0x182330);
    Color rowBackgroundB = ColorFromHex(0x14202B);
    Color rowHoverBackground = ColorFromHex(0x1E3348);
    Color selectedRowBackground = ColorFromHex(0x2D6CDF);
    Color gridLineColor = ColorFromHex(0x2B3A4A);
    Color headerTextColor = ColorFromHex(0xEAF2FB);
    Color textColor = ColorFromHex(0xC7D4E2);
    Color selectedTextColor = ColorFromHex(0xFFFFFF);
    Color editBackground = ColorFromHex(0x0F2438);
    Color editBorderColor = ColorFromHex(0x4E8FEA);
    Color resizeGuideColor = ColorFromHex(0x6AA8FF);
    Color freezeGuideColor = ColorFromHex(0x8AB4F8);
    Color frozenHeaderBackground = ColorFromHex(0x243748);
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
                columnWidth = headerSize.width + scaledCellPadding.left + scaledCellPadding.right + ScaleValue(18.0f);

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
        if (m_hasFocus) {
            renderer.DrawRoundedRect(m_bounds, ColorFromHex(0xFFFFFF), 2.0f, scaledCornerRadius);
        }

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

        const int frozenCount = std::clamp(m_frozenColumnCount, 0, static_cast<int>(columnWidths.size()));
        const float frozenWidth = FrozenWidth(columnWidths);
        // Non-frozen headers first (may scroll under frozen strip).
        for (size_t col = static_cast<size_t>(frozenCount); col < m_columns.size(); ++col) {
            const float cellWidth = columnWidths[col];
            const float currentX = ColumnLeftX(columnWidths, col);
            if (currentX + cellWidth < m_bounds.left || currentX > m_bounds.right) {
                continue;
            }
            const Rect headerHit = Rect::Make(currentX, tableTop, currentX + cellWidth, tableTop + scaledHeaderHeight);
            if (m_sortable && m_hoveredHeader == static_cast<int>(col)) {
                renderer.FillRect(headerHit, headerHoverBackground);
            }

            std::wstring headerLabel = m_columns[col].title;
            if (m_sortable && m_sortColumn == static_cast<int>(col)) {
                headerLabel += m_sortAscending ? L"  \u25B2" : L"  \u25BC";
            }

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
                headerLabel.c_str(),
                static_cast<UINT32>(headerLabel.size()),
                headerCellRect,
                headerTextColor,
                ScaleValue(headerFontSize),
                headerTextOptions);

            if (col + 1 < m_columns.size()) {
                const float x = currentX + cellWidth;
                const bool resizeHot =
                    m_hoveredResizeEdge == static_cast<int>(col) || m_resizingColumn == static_cast<int>(col);
                renderer.DrawLine(
                    PointF{x, tableTop},
                    PointF{x, tableBottom},
                    resizeHot ? resizeGuideColor : gridLineColor,
                    resizeHot ? 2.0f : 1.0f);
            }
        }

        // Frozen headers last so they stay above scrolled content.
        for (size_t col = 0; col < static_cast<size_t>(frozenCount); ++col) {
            const float cellWidth = columnWidths[col];
            const float currentX = ColumnLeftX(columnWidths, col);
            const Rect headerHit = Rect::Make(currentX, tableTop, currentX + cellWidth, tableTop + scaledHeaderHeight);
            renderer.FillRect(headerHit, frozenHeaderBackground);
            if (m_sortable && m_hoveredHeader == static_cast<int>(col)) {
                renderer.FillRect(headerHit, headerHoverBackground);
            }
            std::wstring headerLabel = m_columns[col].title;
            if (m_sortable && m_sortColumn == static_cast<int>(col)) {
                headerLabel += m_sortAscending ? L"  \u25B2" : L"  \u25BC";
            }
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
                headerLabel.c_str(),
                static_cast<UINT32>(headerLabel.size()),
                headerCellRect,
                headerTextColor,
                ScaleValue(headerFontSize),
                headerTextOptions);
            if (col + 1 < static_cast<size_t>(frozenCount)) {
                const float x = currentX + cellWidth;
                renderer.DrawLine(PointF{x, tableTop}, PointF{x, tableBottom}, gridLineColor, 1.0f);
            }
        }
        if (frozenCount > 0 && frozenCount < static_cast<int>(m_columns.size())) {
            const float freezeX = m_bounds.left + frozenWidth;
            renderer.DrawLine(
                PointF{freezeX, tableTop},
                PointF{freezeX, tableBottom},
                freezeGuideColor,
                2.0f);
        }

        if (contentHeight > scaledHeaderHeight + 1.0f && !m_rows.empty()) {
            const int visibleRows = VisibleRowCount();
            const int first = std::clamp(m_scrollRow, 0, std::max(0, static_cast<int>(m_rows.size()) - 1));
            const int last = std::min(static_cast<int>(m_rows.size()), first + std::max(1, visibleRows));

            for (int viewRow = first; viewRow < last; ++viewRow) {
                const int dataRow = ViewToDataRow(viewRow);
                if (dataRow < 0 || dataRow >= static_cast<int>(m_rows.size())) {
                    continue;
                }
                const float rowTop = bodyTop + static_cast<float>(viewRow - first) * scaledRowHeight;
                const float rowBottom = std::min(tableBottom, rowTop + scaledRowHeight);
                if (rowTop >= tableBottom) {
                    break;
                }
                const Rect rowRect = Rect::Make(m_bounds.left, rowTop, m_bounds.right, rowBottom);

                Color rowColor = ((viewRow - first) % 2 == 0) ? rowBackgroundA : rowBackgroundB;
                const bool selected = m_selectable && IsRowSelected(dataRow);
                if (selected) {
                    rowColor = selectedRowBackground;
                } else if (m_selectable && dataRow == m_hoveredRow) {
                    rowColor = rowHoverBackground;
                }
                renderer.FillRect(rowRect, rowColor);

                const Color rowTextColor = selected ? selectedTextColor : textColor;

                auto paintCell = [&](size_t col, bool frozenPass) {
                    const bool isFrozen = static_cast<int>(col) < frozenCount;
                    if (frozenPass != isFrozen) {
                        return;
                    }
                    const float cellWidth = columnWidths[col];
                    const float rowX = ColumnLeftX(columnWidths, col);
                    if (rowX + cellWidth < m_bounds.left || rowX > m_bounds.right) {
                        return;
                    }
                    if (isFrozen) {
                        renderer.FillRect(
                            Rect::Make(rowX, rowTop, rowX + cellWidth, rowBottom),
                            selected ? selectedRowBackground : frozenHeaderBackground);
                    }
                    const bool isEditCell =
                        m_editing && dataRow == m_editRow && static_cast<int>(col) == m_editCol;
                    std::wstring cellText =
                        (col < m_rows[static_cast<size_t>(dataRow)].size())
                            ? m_rows[static_cast<size_t>(dataRow)][col]
                            : L"";
                    if (isEditCell) {
                        cellText = m_editBuffer;
                        renderer.FillRect(
                            Rect::Make(rowX + 1.0f, rowTop + 1.0f, rowX + cellWidth - 1.0f, rowBottom - 1.0f),
                            editBackground);
                        renderer.DrawRect(
                            Rect::Make(rowX + 1.0f, rowTop + 1.0f, rowX + cellWidth - 1.0f, rowBottom - 1.0f),
                            editBorderColor,
                            1.5f);
                    }

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
                        isEditCell ? selectedTextColor : rowTextColor,
                        ScaleValue(fontSize),
                        cellTextOptions);

                    if (isEditCell) {
                        const std::wstring prefix = m_editBuffer.substr(0, static_cast<size_t>(m_editCaret));
                        const Size prefixSize = MeasureTextValue(prefix, fontSize, 4096.0f, TextWrapMode::NoWrap);
                        const float caretX = textRect.left + prefixSize.width;
                        renderer.FillRect(
                            Rect::Make(caretX, textRect.top + 2.0f, caretX + ScaleValue(1.0f), textRect.bottom - 2.0f),
                            selectedTextColor);
                    }
                };
                for (size_t col = 0; col < m_columns.size(); ++col) {
                    paintCell(col, false);
                }
                for (size_t col = 0; col < m_columns.size(); ++col) {
                    paintCell(col, true);
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

        // Only shrink-to-fit when not resizing/freezing (freeze + Ctrl+Left/Right use overflow scroll).
        if (m_resizingColumn < 0 && m_frozenColumnCount <= 0 && totalWidth > tableWidth && totalWidth > 0.0f) {
            const float ratio = tableWidth / totalWidth;
            for (float& width : widths) {
                width = std::max(ScaleValue(28.0f), width * ratio);
            }
        }

        return widths;
    }

    int VisibleRowCount() const {
        const float bodyHeight = std::max(0.0f, m_bounds.Height() - ScaleValue(headerHeight));
        return std::max(1, static_cast<int>(std::floor(bodyHeight / std::max(1.0f, ScaleValue(rowHeight)))));
    }

    int HeaderIndexFromPoint(float x, float y) const {
        const float headerBottom = m_bounds.top + ScaleValue(headerHeight);
        if (y < m_bounds.top || y > headerBottom) {
            return -1;
        }
        const auto widths = ResolveColumnWidths();
        for (size_t col = 0; col < widths.size(); ++col) {
            const float left = ColumnLeftX(widths, col);
            if (x >= left && x <= left + widths[col]) {
                return static_cast<int>(col);
            }
        }
        return -1;
    }

    // Returns the left column index of the resize edge under the pointer, or -1.
    int ResizeEdgeFromPoint(float x, float y) const {
        if (m_columns.size() < 2) {
            return -1;
        }
        if (y < m_bounds.top || y > m_bounds.bottom) {
            return -1;
        }
        const auto widths = ResolveColumnWidths();
        const float hitSlop = ScaleValue(4.0f);
        for (size_t col = 0; col + 1 < widths.size(); ++col) {
            const float edgeX = ColumnLeftX(widths, col) + widths[col];
            if (std::abs(x - edgeX) <= hitSlop) {
                return static_cast<int>(col);
            }
        }
        return -1;
    }

    int RowIndexFromPoint(float x, float y) const {
        (void)x;
        const float bodyTop = m_bounds.top + ScaleValue(headerHeight);
        if (y < bodyTop || y > m_bounds.bottom || m_rows.empty()) {
            return -1;
        }
        const float scaledRowHeight = std::max(1.0f, ScaleValue(rowHeight));
        const int visibleIndex = static_cast<int>(std::floor((y - bodyTop) / scaledRowHeight));
        const int viewRow = m_scrollRow + visibleIndex;
        if (viewRow < 0 || viewRow >= static_cast<int>(m_viewOrder.size())) {
            return -1;
        }
        return ViewToDataRow(viewRow);
    }

    bool CellFromPoint(float x, float y, int& outRow, int& outCol) const {
        outRow = RowIndexFromPoint(x, y);
        if (outRow < 0) {
            outCol = -1;
            return false;
        }
        const auto widths = ResolveColumnWidths();
        for (size_t col = 0; col < widths.size(); ++col) {
            const float left = ColumnLeftX(widths, col);
            if (x >= left && x <= left + widths[col]) {
                outCol = static_cast<int>(col);
                return true;
            }
        }
        outCol = -1;
        return false;
    }

    int ViewToDataRow(int viewRow) const {
        if (viewRow < 0 || viewRow >= static_cast<int>(m_viewOrder.size())) {
            return -1;
        }
        return m_viewOrder[static_cast<size_t>(viewRow)];
    }

    int DataToViewRow(int dataRow) const {
        for (size_t i = 0; i < m_viewOrder.size(); ++i) {
            if (m_viewOrder[i] == dataRow) {
                return static_cast<int>(i);
            }
        }
        return -1;
    }

    void RebuildViewOrder() {
        m_viewOrder.resize(m_rows.size());
        for (size_t i = 0; i < m_rows.size(); ++i) {
            m_viewOrder[i] = static_cast<int>(i);
        }
        if (m_sortColumn >= 0 && m_sortColumn < static_cast<int>(m_columns.size())) {
            ApplySort();
        }
        ClampScroll();
    }

    void ToggleSort(int column) {
        if (column < 0 || column >= static_cast<int>(m_columns.size())) {
            return;
        }
        if (m_sortColumn == column) {
            m_sortAscending = !m_sortAscending;
        } else {
            m_sortColumn = column;
            m_sortAscending = true;
        }
        ApplySort();
        EnsureSelectedVisible();
    }

    void ApplySort() {
        if (m_sortColumn < 0 || m_rows.empty()) {
            return;
        }
        const int col = m_sortColumn;
        const bool ascending = m_sortAscending;
        std::stable_sort(m_viewOrder.begin(), m_viewOrder.end(), [&](int a, int b) {
            const std::wstring& left = CellText(a, col);
            const std::wstring& right = CellText(b, col);
            double leftNum = 0.0;
            double rightNum = 0.0;
            const bool leftIsNum = TryParseNumber(left, leftNum);
            const bool rightIsNum = TryParseNumber(right, rightNum);
            int cmp = 0;
            if (leftIsNum && rightIsNum) {
                cmp = (leftNum < rightNum) ? -1 : (leftNum > rightNum ? 1 : 0);
            } else {
                cmp = left.compare(right);
            }
            return ascending ? (cmp < 0) : (cmp > 0);
        });
    }

    const std::wstring& CellText(int row, int col) const {
        static const std::wstring empty;
        if (row < 0 || row >= static_cast<int>(m_rows.size())) {
            return empty;
        }
        if (col < 0 || col >= static_cast<int>(m_rows[static_cast<size_t>(row)].size())) {
            return empty;
        }
        return m_rows[static_cast<size_t>(row)][static_cast<size_t>(col)];
    }

    static bool TryParseNumber(const std::wstring& text, double& out) {
        if (text.empty()) {
            return false;
        }
        wchar_t* end = nullptr;
        out = wcstod(text.c_str(), &end);
        return end && end != text.c_str() && *end == L'\0';
    }

    bool MoveSelection(int delta, bool shift, bool ctrl) {
        if (m_rows.empty()) {
            return false;
        }
        int view = DataToViewRow(m_selectedIndex);
        if (view < 0) {
            view = (delta >= 0) ? 0 : static_cast<int>(m_viewOrder.size()) - 1;
            return SetSelectedIndexInternal(ViewToDataRow(view), shift, ctrl);
        }
        view = std::clamp(view + delta, 0, static_cast<int>(m_viewOrder.size()) - 1);
        return SetSelectedIndexInternal(ViewToDataRow(view), shift, ctrl);
    }

    bool SetSelectedIndexInternal(int dataRow, bool shift, bool ctrl) {
        if (m_rows.empty()) {
            m_selectedIndex = -1;
            m_selectedSet.clear();
            NotifySelectionChanged();
            return false;
        }
        dataRow = std::clamp(dataRow, 0, static_cast<int>(m_rows.size()) - 1);

        if (m_multiSelect && shift && m_selectionAnchor >= 0) {
            SelectRange(m_selectionAnchor, dataRow);
            m_selectedIndex = dataRow;
            EnsureSelectedVisible();
            NotifySelectionChanged();
            return true;
        }

        if (m_multiSelect && ctrl) {
            ToggleRowSelection(dataRow);
            m_selectedIndex = dataRow;
            m_selectionAnchor = dataRow;
            EnsureSelectedVisible();
            NotifySelectionChanged();
            return true;
        }

        const bool same = (dataRow == m_selectedIndex) &&
            m_selectedSet.size() == 1 &&
            IsRowSelected(dataRow);
        m_selectedIndex = dataRow;
        m_selectedSet.clear();
        m_selectedSet.insert(dataRow);
        m_selectionAnchor = dataRow;
        EnsureSelectedVisible();
        if (!same) {
            NotifySelectionChanged();
        }
        return !same;
    }

    void NotifySelectionChanged() {
        if (m_onSelectionChanged) {
            m_onSelectionChanged(SelectedIndices());
        }
    }

    bool ScrollHorizontal(float deltaPx) {
        const float before = m_hScroll;
        m_hScroll += deltaPx;
        ClampHorizontalScroll();
        return m_hScroll != before;
    }

    void ClampHorizontalScroll() {
        const auto widths = ResolveColumnWidths();
        float total = 0.0f;
        float frozen = 0.0f;
        const int frozenCount = std::clamp(m_frozenColumnCount, 0, static_cast<int>(widths.size()));
        for (size_t i = 0; i < widths.size(); ++i) {
            total += widths[i];
            if (static_cast<int>(i) < frozenCount) {
                frozen += widths[i];
            }
        }
        const float viewport = std::max(0.0f, m_bounds.Width() - frozen);
        const float scrollable = std::max(0.0f, total - frozen);
        const float maxScroll = std::max(0.0f, scrollable - viewport);
        m_hScroll = std::clamp(m_hScroll, 0.0f, maxScroll);
    }

    float FrozenWidth(const std::vector<float>& widths) const {
        float w = 0.0f;
        const int frozenCount = std::clamp(m_frozenColumnCount, 0, static_cast<int>(widths.size()));
        for (int i = 0; i < frozenCount; ++i) {
            w += widths[static_cast<size_t>(i)];
        }
        return w;
    }

    float ColumnLeftX(const std::vector<float>& widths, size_t col) const {
        const int frozenCount = std::clamp(m_frozenColumnCount, 0, static_cast<int>(widths.size()));
        float x = m_bounds.left;
        if (static_cast<int>(col) < frozenCount) {
            for (size_t i = 0; i < col; ++i) {
                x += widths[i];
            }
            return x;
        }
        x += FrozenWidth(widths);
        for (size_t i = static_cast<size_t>(frozenCount); i < col; ++i) {
            x += widths[i];
        }
        return x - m_hScroll;
    }

    void ToggleRowSelection(int dataRow) {
        if (dataRow < 0 || dataRow >= static_cast<int>(m_rows.size())) {
            return;
        }
        auto it = m_selectedSet.find(dataRow);
        if (it != m_selectedSet.end()) {
            m_selectedSet.erase(it);
        } else {
            m_selectedSet.insert(dataRow);
        }
    }

    void SelectRange(int fromData, int toData) {
        const int fromView = DataToViewRow(fromData);
        const int toView = DataToViewRow(toData);
        if (fromView < 0 || toView < 0) {
            m_selectedSet.clear();
            m_selectedSet.insert(toData);
            return;
        }
        const int lo = std::min(fromView, toView);
        const int hi = std::max(fromView, toView);
        m_selectedSet.clear();
        for (int v = lo; v <= hi; ++v) {
            const int data = ViewToDataRow(v);
            if (data >= 0) {
                m_selectedSet.insert(data);
            }
        }
    }

    void SanitizeSelection() {
        std::set<int> next;
        for (int idx : m_selectedSet) {
            if (idx >= 0 && idx < static_cast<int>(m_rows.size())) {
                next.insert(idx);
            }
        }
        m_selectedSet.swap(next);
        if (m_selectedIndex >= static_cast<int>(m_rows.size())) {
            m_selectedIndex = m_rows.empty() ? -1 : static_cast<int>(m_rows.size()) - 1;
        }
        if (m_selectedIndex >= 0 && m_selectedSet.empty()) {
            m_selectedSet.insert(m_selectedIndex);
        }
    }

    bool BeginEdit(int row, int col) {
        if (!m_editable || row < 0 || row >= static_cast<int>(m_rows.size()) || m_columns.empty()) {
            return false;
        }
        col = std::clamp(col, 0, static_cast<int>(m_columns.size()) - 1);
        CommitEdit(true);
        auto& cells = m_rows[static_cast<size_t>(row)];
        if (cells.size() <= static_cast<size_t>(col)) {
            cells.resize(static_cast<size_t>(col) + 1);
        }
        m_editing = true;
        m_editRow = row;
        m_editCol = col;
        m_editBuffer = cells[static_cast<size_t>(col)];
        m_editCaret = static_cast<int>(m_editBuffer.size());
        m_selectedIndex = row;
        m_selectedSet.clear();
        m_selectedSet.insert(row);
        EnsureSelectedVisible();
        return true;
    }

    void CommitEdit(bool apply) {
        if (!m_editing) {
            return;
        }
        if (apply &&
            m_editRow >= 0 && m_editRow < static_cast<int>(m_rows.size()) &&
            m_editCol >= 0 && m_editCol < static_cast<int>(m_columns.size())) {
            auto& cells = m_rows[static_cast<size_t>(m_editRow)];
            if (cells.size() <= static_cast<size_t>(m_editCol)) {
                cells.resize(static_cast<size_t>(m_editCol) + 1);
            }
            const std::wstring previous = cells[static_cast<size_t>(m_editCol)];
            cells[static_cast<size_t>(m_editCol)] = m_editBuffer;
            if (m_sortColumn == m_editCol) {
                ApplySort();
            }
            if (m_onCellChanged && previous != m_editBuffer) {
                m_onCellChanged(m_editRow, m_editCol, m_editBuffer);
            }
        }
        m_editing = false;
        m_editRow = -1;
        m_editCol = -1;
        m_editBuffer.clear();
        m_editCaret = 0;
    }

    void EndColumnResize() {
        m_resizingColumn = -1;
        m_resizeStartX = 0.0f;
        m_resizeStartWidth = 0.0f;
    }

    void EnsureSelectedVisible() {
        if (m_selectedIndex < 0 || m_viewOrder.empty()) {
            return;
        }
        const int view = DataToViewRow(m_selectedIndex);
        if (view < 0) {
            return;
        }
        const int visible = VisibleRowCount();
        if (view < m_scrollRow) {
            m_scrollRow = view;
        } else if (view >= m_scrollRow + visible) {
            m_scrollRow = view - visible + 1;
        }
        ClampScroll();
    }

    void ClampScroll() {
        const int maxScroll = std::max(0, static_cast<int>(m_viewOrder.size()) - VisibleRowCount());
        m_scrollRow = std::clamp(m_scrollRow, 0, maxScroll);
    }

    std::vector<Column> m_columns;
    std::vector<std::vector<std::wstring>> m_rows;
    std::vector<int> m_viewOrder;
    std::set<int> m_selectedSet;
    bool m_selectable = true;
    bool m_sortable = true;
    bool m_multiSelect = false;
    bool m_editable = false;
    bool m_resizableColumns = false;
    int m_frozenColumnCount = 0;
    float m_hScroll = 0.0f;
    int m_selectedIndex = -1;
    int m_selectionAnchor = -1;
    int m_hoveredRow = -1;
    int m_hoveredHeader = -1;
    int m_hoveredResizeEdge = -1;
    int m_sortColumn = -1;
    bool m_sortAscending = true;
    int m_scrollRow = 0;
    std::function<void(const std::vector<int>&)> m_onSelectionChanged;
    std::function<void(int, int, const std::wstring&)> m_onCellChanged;

    bool m_editing = false;
    int m_editRow = -1;
    int m_editCol = -1;
    int m_editFocusCol = 0;
    std::wstring m_editBuffer;
    int m_editCaret = 0;

    int m_lastClickRow = -1;
    int m_lastClickCol = -1;
    DWORD m_lastClickTime = 0;

    int m_resizingColumn = -1;
    float m_resizeStartX = 0.0f;
    float m_resizeStartWidth = 0.0f;
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

