#pragma once
// Component module - part of the ai-win-ui library.
// See ui.h for the full umbrella include.
#include "ui_views.h"

class MenuStrip : public UIElement {
public:
    MenuStrip() { m_style = DefaultStyle(nullptr); }

    static ComponentStyle DefaultStyle(const Theme* theme = nullptr) {
        ComponentStyle s;
        BoxDecoration shell;
        shell.background = ComponentStyle::ThemeColor(theme, "surface-2", ColorFromHex(0x142131));
        shell.border.width = Thickness{1, 1, 1, 1};
        shell.border.color = ComponentStyle::ThemeColor(theme, "border-subtle", ColorFromHex(0x3A5064));
        shell.radius = CornerRadius::Uniform(ComponentStyle::ThemeNumber(theme, Theme::NumberCategory::Radius, "md", 8.0f));
        s.base.decoration = shell;
        s.base.foreground = ComponentStyle::ThemeColor(theme, "fg", ColorFromHex(0xDCE7F4));
        s.base.fontSize = ComponentStyle::ThemeNumber(theme, Theme::NumberCategory::FontSize, "sm", 13.0f);
        BoxDecoration focused = shell;
        focused.border.width = Thickness{2, 2, 2, 2};
        focused.border.color = ComponentStyle::ThemeColor(theme, "border-focus", ColorFromHex(0xFFFFFF));
        s.overrides[static_cast<std::size_t>(StyleState::Focused)].decoration = focused;
        auto item = std::make_unique<ComponentStyle>();
        item->base.foreground = ComponentStyle::ThemeColor(theme, "fg", ColorFromHex(0xDCE7F4));
        BoxDecoration hover;
        hover.background = ComponentStyle::ThemeColor(theme, "surface-3", ColorFromHex(0x1F3348));
        hover.radius = CornerRadius::Uniform(6.0f);
        item->overrides[static_cast<std::size_t>(StyleState::Hover)].decoration = hover;
        BoxDecoration selected;
        selected.background = ComponentStyle::ThemeColor(theme, "accent-soft", ColorFromHex(0x2C4C70));
        selected.radius = CornerRadius::Uniform(6.0f);
        item->overrides[static_cast<std::size_t>(StyleState::Selected)].decoration = selected;
        item->overrides[static_cast<std::size_t>(StyleState::Selected)].foreground =
            ComponentStyle::ThemeColor(theme, "fg-strong", ColorFromHex(0xFFFFFF));
        s.itemStyle = std::move(item);
        return s;
    }

    void ApplyThemeDefaults() override {
        if (m_styleFromLayout) {
            return;
        }
        const Theme* theme = (m_context && m_context->theme) ? m_context->theme : nullptr;
        m_style = DefaultStyle(theme);
        SyncLegacyColorsFromStyle();
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
        bool changed = false;
        if (m_hasFocus) {
            m_hasFocus = false;
            changed = true;
        }
        if (m_hoveredIndex != -1) {
            m_hoveredIndex = -1;
            changed = true;
        }
        return changed;
    }

    bool OnKeyDown(WPARAM keyCode, LPARAM /*lParam*/) override {
        if (m_items.empty()) {
            return false;
        }
        switch (keyCode) {
            case VK_LEFT:
            case VK_UP:
                return MoveSelectionBy(-1);
            case VK_RIGHT:
            case VK_DOWN:
                return MoveSelectionBy(1);
            case VK_HOME:
                return SetSelectedIndex(0);
            case VK_END:
                return SetSelectedIndex(static_cast<int>(m_items.size()) - 1);
            case VK_RETURN:
            case VK_SPACE:
                return m_selectedIndex >= 0;
            default:
                return false;
        }
    }

    bool OnMouseDown(float x, float y) override {
        if (!HitTest(x, y)) {
            return false;
        }
        const int index = IndexFromPoint(x, y);
        if (index >= 0) {
            const bool changed = SetSelectedIndex(index);
            return changed || true;
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

        const int nextHovered = IndexFromPoint(x, y);
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
        const float minWidth = ScaleValue(220.0f);
        float total = ScaleValue(12.0f);
        const float horizontalPadding = ScaleValue(itemHorizontalPadding);
        const float spacing = ScaleValue(itemSpacing);
        const float iconWidth = ScaleValue(iconSize);
        const float iconGapWidth = ScaleValue(iconGap);
        for (size_t i = 0; i < m_items.size(); ++i) {
            const Size measured = MeasureTextValue(
                m_items[i].text,
                fontSize,
                4096.0f,
                TextWrapMode::NoWrap);
            total += measured.width + horizontalPadding * 2.0f;
            if (m_items[i].HasIcon()) {
                total += iconWidth + iconGapWidth;
            }
            if (i + 1 < m_items.size()) {
                total += spacing;
            }
        }
        return std::min(availableWidth, std::max(minWidth, total));
    }

    float MeasurePreferredHeight(float width) const override {
        (void)width;
        return ScaleValue(stripHeight);
    }

public:
    void Render(IRenderer& renderer) override {
        const float scaledCornerRadius = ScaleValue(cornerRadius);
        renderer.FillRoundedRect(m_bounds, background, scaledCornerRadius);
        renderer.DrawRoundedRect(m_bounds, borderColor, 1.0f, scaledCornerRadius);
        if (m_hasFocus) {
            renderer.DrawRoundedRect(m_bounds, focusBorderColor, 2.0f, scaledCornerRadius);
        }

        const std::vector<Rect> rects = BuildItemRects();
        const TextRenderOptions textOptions{
            TextWrapMode::NoWrap,
            TextHorizontalAlign::Start,
            TextVerticalAlign::Center
        };
        for (size_t i = 0; i < rects.size(); ++i) {
            StyleState itemState = StyleState::Normal;
            if (static_cast<int>(i) == m_selectedIndex) {
                itemState = StyleState::Selected;
            } else if (static_cast<int>(i) == m_hoveredIndex) {
                itemState = StyleState::Hover;
            }
            Color fill = background;
            Color itemFg = (itemState == StyleState::Selected) ? selectedTextColor : textColor;
            float itemRadius = ScaleValue(std::max(2.0f, cornerRadius - 2.0f));
            if (m_style.itemStyle) {
                const StyleSpec itemSpec = m_style.itemStyle->Resolve(itemState);
                if (itemSpec.decoration.has_value()) {
                    fill = itemSpec.decoration->background;
                    if (!itemSpec.decoration->radius.IsZero()) {
                        itemRadius = ScaleValue(itemSpec.decoration->radius.MaxRadius());
                    }
                }
                if (itemSpec.foreground.has_value()) {
                    itemFg = *itemSpec.foreground;
                }
            } else {
                if (itemState == StyleState::Selected) {
                    fill = selectedBackground;
                } else if (itemState == StyleState::Hover) {
                    fill = hoverBackground;
                }
            }

            if (fill.a > 0.001f) {
                renderer.FillRoundedRect(rects[i], fill, itemRadius);
            }

            EnsureStripItemSvg(renderer, m_items[i]);
            Rect textRect = rects[i];
            if (m_items[i].HasIcon()) {
                const float iconPx = ScaleValue(iconSize);
                const float iconInset = ScaleValue(itemHorizontalPadding);
                const float iconTop = rects[i].top + std::max(0.0f, (rects[i].Height() - iconPx) * 0.5f);
                const Rect iconRect = Rect::Make(
                    rects[i].left + iconInset,
                    iconTop,
                    rects[i].left + iconInset + iconPx,
                    iconTop + iconPx);
                if (m_items[i].svg) {
                    renderer.DrawSvg(m_items[i].svg, iconRect);
                }
                textRect.left = iconRect.right + ScaleValue(iconGap);
            } else {
                textRect.left += ScaleValue(itemHorizontalPadding);
            }

            renderer.DrawTextW(
                m_items[i].text.c_str(),
                static_cast<UINT32>(m_items[i].text.size()),
                textRect,
                itemFg,
                ScaleValue(fontSize),
                textOptions);
        }
    }

    void SetItems(std::vector<std::wstring> items) {
        std::vector<StripItem> entries;
        entries.reserve(items.size());
        for (auto& item : items) {
            StripItem entry;
            entry.text = std::move(item);
            entries.push_back(std::move(entry));
        }
        SetEntries(std::move(entries));
    }

    void SetEntries(std::vector<StripItem> items) {
        m_items = std::move(items);
        if (m_items.empty()) {
            m_selectedIndex = -1;
            m_hoveredIndex = -1;
            return;
        }
        if (m_selectedIndex < 0) {
            m_selectedIndex = 0;
        } else {
            m_selectedIndex = std::clamp(m_selectedIndex, 0, static_cast<int>(m_items.size()) - 1);
        }
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

    int SelectedIndex() const { return m_selectedIndex; }

    Color background = ColorFromHex(0x142131);
    Color borderColor = ColorFromHex(0x3A5064);
    Color focusBorderColor = ColorFromHex(0xFFFFFF);
    Color hoverBackground = ColorFromHex(0x1F3348);
    Color selectedBackground = ColorFromHex(0x2C4C70);
    Color textColor = ColorFromHex(0xDCE7F4);
    Color selectedTextColor = ColorFromHex(0xFFFFFF);
    float cornerRadius = 8.0f;
    float fontSize = 13.0f;
    float stripHeight = 30.0f;
    float itemSpacing = 6.0f;
    float itemHorizontalPadding = 12.0f;
    float iconSize = 14.0f;
    float iconGap = 8.0f;

private:

    void SyncLegacyColorsFromStyle() {
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
        if (m_style.itemStyle) {
            const StyleSpec hover = m_style.itemStyle->Resolve(StyleState::Hover);
            const StyleSpec selected = m_style.itemStyle->Resolve(StyleState::Selected);
            if (hover.decoration.has_value()) {
                hoverBackground = hover.decoration->background;
            }
            if (selected.decoration.has_value()) {
                selectedBackground = selected.decoration->background;
            }
            if (selected.foreground.has_value()) {
                selectedTextColor = *selected.foreground;
            }
        }
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

    std::vector<Rect> BuildItemRects() const {
        std::vector<Rect> rects;
        if (m_items.empty()) {
            return rects;
        }

        const float inset = ScaleValue(4.0f);
        const float spacing = ScaleValue(itemSpacing);
        const float horizontalPadding = ScaleValue(itemHorizontalPadding);
        const float iconWidth = ScaleValue(iconSize);
        const float iconGapWidth = ScaleValue(iconGap);
        const float top = m_bounds.top + inset;
        const float bottom = m_bounds.bottom - inset;
        float x = m_bounds.left + inset;
        rects.reserve(m_items.size());
        for (const auto& item : m_items) {
            const Size measured = MeasureTextValue(item.text, fontSize, 4096.0f, TextWrapMode::NoWrap);
            float width = measured.width + horizontalPadding * 2.0f;
            if (item.HasIcon()) {
                width += iconWidth + iconGapWidth;
            }
            width = std::max(width, ScaleValue(36.0f));
            const float right = std::min(m_bounds.right - inset, x + width);
            rects.push_back(Rect::Make(x, top, right, bottom));
            x = right + spacing;
        }
        return rects;
    }

    int IndexFromPoint(float x, float y) const {
        const std::vector<Rect> rects = BuildItemRects();
        for (size_t i = 0; i < rects.size(); ++i) {
            if (rects[i].Contains(x, y)) {
                return static_cast<int>(i);
            }
        }
        return -1;
    }

    std::vector<StripItem> m_items;
    int m_selectedIndex = -1;
    int m_hoveredIndex = -1;
};

class ToolStrip : public UIElement {
public:
    ToolStrip() { m_style = DefaultStyle(nullptr); }

    static ComponentStyle DefaultStyle(const Theme* theme = nullptr) {
        ComponentStyle s;
        BoxDecoration shell;
        shell.background = ComponentStyle::ThemeColor(theme, "surface-2", ColorFromHex(0x152132));
        shell.border.width = Thickness{1, 1, 1, 1};
        shell.border.color = ComponentStyle::ThemeColor(theme, "border-subtle", ColorFromHex(0x395266));
        shell.radius = CornerRadius::Uniform(ComponentStyle::ThemeNumber(theme, Theme::NumberCategory::Radius, "md", 8.0f));
        s.base.decoration = shell;
        s.base.foreground = ComponentStyle::ThemeColor(theme, "fg", ColorFromHex(0xDCE6F3));
        s.base.fontSize = ComponentStyle::ThemeNumber(theme, Theme::NumberCategory::FontSize, "sm", 13.0f);
        BoxDecoration focused = shell;
        focused.border.width = Thickness{2, 2, 2, 2};
        focused.border.color = ComponentStyle::ThemeColor(theme, "border-focus", ColorFromHex(0xFFFFFF));
        s.overrides[static_cast<std::size_t>(StyleState::Focused)].decoration = focused;
        auto item = std::make_unique<ComponentStyle>();
        BoxDecoration baseBtn;
        baseBtn.background = ComponentStyle::ThemeColor(theme, "surface-3", ColorFromHex(0x1B2D40));
        baseBtn.border.width = Thickness{1, 1, 1, 1};
        baseBtn.border.color = ComponentStyle::ThemeColor(theme, "border-subtle", ColorFromHex(0x4A6278));
        baseBtn.radius = CornerRadius::Uniform(6.0f);
        item->base.decoration = baseBtn;
        item->base.foreground = ComponentStyle::ThemeColor(theme, "fg", ColorFromHex(0xDCE6F3));
        BoxDecoration hover = baseBtn;
        hover.background = ComponentStyle::ThemeColor(theme, "surface-4", ColorFromHex(0x23405A));
        item->overrides[static_cast<std::size_t>(StyleState::Hover)].decoration = hover;
        BoxDecoration selected = baseBtn;
        selected.background = ComponentStyle::ThemeColor(theme, "accent-soft", ColorFromHex(0x315C86));
        item->overrides[static_cast<std::size_t>(StyleState::Selected)].decoration = selected;
        item->overrides[static_cast<std::size_t>(StyleState::Selected)].foreground =
            ComponentStyle::ThemeColor(theme, "fg-strong", ColorFromHex(0xFFFFFF));
        s.itemStyle = std::move(item);
        return s;
    }

    void ApplyThemeDefaults() override {
        if (m_styleFromLayout) {
            return;
        }
        const Theme* theme = (m_context && m_context->theme) ? m_context->theme : nullptr;
        m_style = DefaultStyle(theme);
        SyncLegacyColorsFromStyle();
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
        bool changed = false;
        if (m_hasFocus) {
            m_hasFocus = false;
            changed = true;
        }
        if (m_hoveredIndex != -1) {
            m_hoveredIndex = -1;
            changed = true;
        }
        return changed;
    }

    bool OnKeyDown(WPARAM keyCode, LPARAM /*lParam*/) override {
        if (m_items.empty()) {
            return false;
        }
        switch (keyCode) {
            case VK_LEFT:
            case VK_UP:
                return MoveSelectionBy(-1);
            case VK_RIGHT:
            case VK_DOWN:
                return MoveSelectionBy(1);
            case VK_HOME:
                return SetSelectedIndex(0);
            case VK_END:
                return SetSelectedIndex(static_cast<int>(m_items.size()) - 1);
            case VK_RETURN:
            case VK_SPACE:
                return m_selectedIndex >= 0;
            default:
                return false;
        }
    }

    bool OnMouseDown(float x, float y) override {
        if (!HitTest(x, y)) {
            return false;
        }
        const int index = IndexFromPoint(x, y);
        if (index >= 0) {
            const bool changed = SetSelectedIndex(index);
            return changed || true;
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

        const int nextHovered = IndexFromPoint(x, y);
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
        const float minWidth = ScaleValue(240.0f);
        float total = ScaleValue(12.0f);
        const float spacing = ScaleValue(itemSpacing);
        const float horizontalPadding = ScaleValue(itemHorizontalPadding);
        const float iconWidth = ScaleValue(iconSize);
        const float iconGapWidth = ScaleValue(iconGap);
        for (size_t i = 0; i < m_items.size(); ++i) {
            const Size measured = MeasureTextValue(
                m_items[i].text,
                fontSize,
                4096.0f,
                TextWrapMode::NoWrap);
            total += measured.width + horizontalPadding * 2.0f;
            if (m_items[i].HasIcon()) {
                total += iconWidth + iconGapWidth;
            }
            if (i + 1 < m_items.size()) {
                total += spacing;
            }
        }
        return std::min(availableWidth, std::max(minWidth, total));
    }

    float MeasurePreferredHeight(float width) const override {
        (void)width;
        return ScaleValue(stripHeight);
    }

public:
    void Render(IRenderer& renderer) override {
        const float scaledCornerRadius = ScaleValue(cornerRadius);
        renderer.FillRoundedRect(m_bounds, background, scaledCornerRadius);
        renderer.DrawRoundedRect(m_bounds, borderColor, 1.0f, scaledCornerRadius);
        if (m_hasFocus) {
            renderer.DrawRoundedRect(m_bounds, focusBorderColor, 2.0f, scaledCornerRadius);
        }

        const std::vector<Rect> rects = BuildItemRects();
        const TextRenderOptions textOptions{
            TextWrapMode::NoWrap,
            TextHorizontalAlign::Start,
            TextVerticalAlign::Center
        };
        for (size_t i = 0; i < rects.size(); ++i) {
            StyleState itemState = StyleState::Normal;
            if (static_cast<int>(i) == m_selectedIndex) {
                itemState = StyleState::Selected;
            } else if (static_cast<int>(i) == m_hoveredIndex) {
                itemState = StyleState::Hover;
            }
            Color fill = buttonBackground;
            Color itemBorder = buttonBorderColor;
            Color itemFg = (itemState == StyleState::Selected) ? selectedTextColor : textColor;
            float itemRadius = ScaleValue(std::max(2.0f, buttonCornerRadius));
            if (m_style.itemStyle) {
                const StyleSpec itemSpec = m_style.itemStyle->Resolve(itemState);
                if (itemSpec.decoration.has_value()) {
                    fill = itemSpec.decoration->background;
                    itemBorder = itemSpec.decoration->border.color;
                    if (!itemSpec.decoration->radius.IsZero()) {
                        itemRadius = ScaleValue(itemSpec.decoration->radius.MaxRadius());
                    }
                }
                if (itemSpec.foreground.has_value()) {
                    itemFg = *itemSpec.foreground;
                }
            } else {
                if (itemState == StyleState::Selected) {
                    fill = selectedBackground;
                } else if (itemState == StyleState::Hover) {
                    fill = hoverBackground;
                }
            }

            renderer.FillRoundedRect(rects[i], fill, itemRadius);
            renderer.DrawRoundedRect(rects[i], itemBorder, 1.0f, itemRadius);

            EnsureStripItemSvg(renderer, m_items[i]);
            Rect textRect = rects[i];
            if (m_items[i].HasIcon()) {
                const float iconPx = ScaleValue(iconSize);
                const float iconInset = ScaleValue(itemHorizontalPadding);
                const float iconTop = rects[i].top + std::max(0.0f, (rects[i].Height() - iconPx) * 0.5f);
                const Rect iconRect = Rect::Make(
                    rects[i].left + iconInset,
                    iconTop,
                    rects[i].left + iconInset + iconPx,
                    iconTop + iconPx);
                if (m_items[i].svg) {
                    renderer.DrawSvg(m_items[i].svg, iconRect);
                }
                textRect.left = iconRect.right + ScaleValue(iconGap);
            } else {
                textRect.left += ScaleValue(itemHorizontalPadding);
            }
            renderer.DrawTextW(
                m_items[i].text.c_str(),
                static_cast<UINT32>(m_items[i].text.size()),
                textRect,
                itemFg,
                ScaleValue(fontSize),
                textOptions);
        }
    }

    void SetItems(std::vector<std::wstring> items) {
        std::vector<StripItem> entries;
        entries.reserve(items.size());
        for (auto& item : items) {
            StripItem entry;
            entry.text = std::move(item);
            entries.push_back(std::move(entry));
        }
        SetEntries(std::move(entries));
    }

    void SetEntries(std::vector<StripItem> items) {
        m_items = std::move(items);
        if (m_items.empty()) {
            m_selectedIndex = -1;
            m_hoveredIndex = -1;
            return;
        }
        if (m_selectedIndex < 0) {
            m_selectedIndex = 0;
        } else {
            m_selectedIndex = std::clamp(m_selectedIndex, 0, static_cast<int>(m_items.size()) - 1);
        }
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

    int SelectedIndex() const { return m_selectedIndex; }

    Color background = ColorFromHex(0x152132);
    Color borderColor = ColorFromHex(0x395266);
    Color focusBorderColor = ColorFromHex(0xFFFFFF);
    Color buttonBackground = ColorFromHex(0x1B2D40);
    Color buttonBorderColor = ColorFromHex(0x4A6278);
    Color hoverBackground = ColorFromHex(0x23405A);
    Color selectedBackground = ColorFromHex(0x315C86);
    Color textColor = ColorFromHex(0xDCE6F3);
    Color selectedTextColor = ColorFromHex(0xFFFFFF);
    float cornerRadius = 8.0f;
    float buttonCornerRadius = 6.0f;
    float fontSize = 13.0f;
    float stripHeight = 34.0f;
    float itemSpacing = 8.0f;
    float itemHorizontalPadding = 12.0f;
    float iconSize = 14.0f;
    float iconGap = 8.0f;

private:

    void SyncLegacyColorsFromStyle() {
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
        if (m_style.itemStyle) {
            const StyleSpec normal = m_style.itemStyle->Resolve(StyleState::Normal);
            const StyleSpec hover = m_style.itemStyle->Resolve(StyleState::Hover);
            const StyleSpec selected = m_style.itemStyle->Resolve(StyleState::Selected);
            if (normal.decoration.has_value()) {
                buttonBackground = normal.decoration->background;
                buttonBorderColor = normal.decoration->border.color;
                if (!normal.decoration->radius.IsZero()) {
                    buttonCornerRadius = normal.decoration->radius.MaxRadius();
                }
            }
            if (hover.decoration.has_value()) {
                hoverBackground = hover.decoration->background;
            }
            if (selected.decoration.has_value()) {
                selectedBackground = selected.decoration->background;
            }
            if (selected.foreground.has_value()) {
                selectedTextColor = *selected.foreground;
            }
        }
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

    std::vector<Rect> BuildItemRects() const {
        std::vector<Rect> rects;
        if (m_items.empty()) {
            return rects;
        }

        const float inset = ScaleValue(6.0f);
        const float spacing = ScaleValue(itemSpacing);
        const float horizontalPadding = ScaleValue(itemHorizontalPadding);
        const float iconWidth = ScaleValue(iconSize);
        const float iconGapWidth = ScaleValue(iconGap);
        const float top = m_bounds.top + inset;
        const float bottom = m_bounds.bottom - inset;
        float x = m_bounds.left + inset;
        rects.reserve(m_items.size());
        for (const auto& item : m_items) {
            const Size measured = MeasureTextValue(item.text, fontSize, 4096.0f, TextWrapMode::NoWrap);
            float width = measured.width + horizontalPadding * 2.0f;
            if (item.HasIcon()) {
                width += iconWidth + iconGapWidth;
            }
            width = std::max(width, ScaleValue(38.0f));
            const float right = std::min(m_bounds.right - inset, x + width);
            rects.push_back(Rect::Make(x, top, right, bottom));
            x = right + spacing;
        }
        return rects;
    }

    int IndexFromPoint(float x, float y) const {
        const std::vector<Rect> rects = BuildItemRects();
        for (size_t i = 0; i < rects.size(); ++i) {
            if (rects[i].Contains(x, y)) {
                return static_cast<int>(i);
            }
        }
        return -1;
    }

    std::vector<StripItem> m_items;
    int m_selectedIndex = -1;
    int m_hoveredIndex = -1;
};

class StatusStrip : public UIElement {
public:
    StatusStrip() { m_style = DefaultStyle(nullptr); }

    static ComponentStyle DefaultStyle(const Theme* theme = nullptr) {
        ComponentStyle s;
        BoxDecoration shell;
        shell.background = ComponentStyle::ThemeColor(theme, "surface-2", ColorFromHex(0x13202E));
        shell.border.width = Thickness{1, 1, 1, 1};
        shell.border.color = ComponentStyle::ThemeColor(theme, "border-subtle", ColorFromHex(0x395064));
        shell.radius = CornerRadius::Uniform(ComponentStyle::ThemeNumber(theme, Theme::NumberCategory::Radius, "md", 8.0f));
        s.base.decoration = shell;
        s.base.foreground = ComponentStyle::ThemeColor(theme, "fg", ColorFromHex(0xE1EAF6));
        s.base.fontSize = ComponentStyle::ThemeNumber(theme, Theme::NumberCategory::FontSize, "sm", 12.0f);
        return s;
    }

    void ApplyThemeDefaults() override {
        if (m_styleFromLayout) {
            return;
        }
        const Theme* theme = (m_context && m_context->theme) ? m_context->theme : nullptr;
        m_style = DefaultStyle(theme);
        SyncLegacyColorsFromStyle();
    }


    bool OnMouseDown(float x, float y) override {
        return HitTest(x, y);
    }

protected:
    float MeasurePreferredWidth(float availableWidth) const override {
        const float minWidth = ScaleValue(240.0f);
        float total = ScaleValue(18.0f);
        const float spacing = ScaleValue(itemSpacing);
        for (size_t i = 0; i < m_items.size(); ++i) {
            const Size measured = MeasureTextValue(
                m_items[i],
                fontSize,
                4096.0f,
                TextWrapMode::NoWrap);
            total += measured.width;
            if (i + 1 < m_items.size()) {
                total += spacing + ScaleValue(10.0f);
            }
        }
        return std::min(availableWidth, std::max(minWidth, total));
    }

    float MeasurePreferredHeight(float width) const override {
        (void)width;
        return ScaleValue(stripHeight);
    }

public:
    void Render(IRenderer& renderer) override {
        const float scaledCornerRadius = ScaleValue(cornerRadius);
        renderer.FillRoundedRect(m_bounds, background, scaledCornerRadius);
        renderer.DrawRoundedRect(m_bounds, borderColor, 1.0f, scaledCornerRadius);

        if (m_items.empty()) {
            return;
        }

        const float insetX = ScaleValue(10.0f);
        const float spacing = ScaleValue(itemSpacing);
        const TextRenderOptions textOptions{
            TextWrapMode::NoWrap,
            TextHorizontalAlign::Start,
            TextVerticalAlign::Center
        };

        std::vector<Rect> rects;
        rects.reserve(m_items.size());
        float x = m_bounds.left + insetX;
        for (size_t i = 0; i < m_items.size(); ++i) {
            const Size measured = MeasureTextValue(m_items[i], fontSize, 4096.0f, TextWrapMode::NoWrap);
            float width = std::max(measured.width, ScaleValue(24.0f));
            if (i + 1 == m_items.size()) {
                width = std::max(width, m_bounds.right - insetX - x);
            }
            const float right = std::min(m_bounds.right - insetX, x + width);
            rects.push_back(Rect::Make(x, m_bounds.top, right, m_bounds.bottom));
            x = right + spacing + ScaleValue(10.0f);
        }

        for (size_t i = 0; i < rects.size(); ++i) {
            const Color color = (i + 1 == m_items.size()) ? mutedTextColor : textColor;
            renderer.DrawTextW(
                m_items[i].c_str(),
                static_cast<UINT32>(m_items[i].size()),
                rects[i],
                color,
                ScaleValue(fontSize),
                textOptions);

            if (i + 1 < rects.size()) {
                const float separatorX = rects[i].right + spacing * 0.5f;
                renderer.DrawLine(
                    PointF{separatorX, m_bounds.top + ScaleValue(7.0f)},
                    PointF{separatorX, m_bounds.bottom - ScaleValue(7.0f)},
                    separatorColor,
                    1.0f);
            }
        }
    }

    void SetItems(std::vector<std::wstring> items) {
        m_items = std::move(items);
    }

    Color background = ColorFromHex(0x13202E);
    Color borderColor = ColorFromHex(0x395064);
    Color separatorColor = ColorFromHex(0x50677C);
    Color textColor = ColorFromHex(0xE1EAF6);
    Color mutedTextColor = ColorFromHex(0xA4B5C8);
    float cornerRadius = 8.0f;
    float fontSize = 12.0f;
    float stripHeight = 30.0f;
    float itemSpacing = 10.0f;

private:

    void SyncLegacyColorsFromStyle() {
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

    std::vector<std::wstring> m_items;
};

class ContextMenu : public UIElement {
public:
    ContextMenu() { m_style = DefaultStyle(nullptr); }

    static ComponentStyle DefaultStyle(const Theme* theme = nullptr) {
        ComponentStyle s;
        BoxDecoration shell;
        shell.background = ComponentStyle::ThemeColor(theme, "surface-1", ColorFromHex(0x112130));
        shell.border.width = Thickness{1, 1, 1, 1};
        shell.border.color = ComponentStyle::ThemeColor(theme, "border-subtle", ColorFromHex(0x3A536B));
        shell.radius = CornerRadius::Uniform(ComponentStyle::ThemeNumber(theme, Theme::NumberCategory::Radius, "md", 8.0f));
        s.base.decoration = shell;
        s.base.foreground = ComponentStyle::ThemeColor(theme, "fg", ColorFromHex(0xE4EEF8));
        s.base.fontSize = ComponentStyle::ThemeNumber(theme, Theme::NumberCategory::FontSize, "sm", 13.0f);
        BoxDecoration focused = shell;
        focused.border.width = Thickness{2, 2, 2, 2};
        focused.border.color = ComponentStyle::ThemeColor(theme, "border-focus", ColorFromHex(0xFFFFFF));
        s.overrides[static_cast<std::size_t>(StyleState::Focused)].decoration = focused;
        auto item = std::make_unique<ComponentStyle>();
        item->base.foreground = ComponentStyle::ThemeColor(theme, "fg", ColorFromHex(0xE4EEF8));
        BoxDecoration hover;
        hover.background = ComponentStyle::ThemeColor(theme, "surface-3", ColorFromHex(0x1F3A54));
        hover.radius = CornerRadius::Uniform(4.0f);
        item->overrides[static_cast<std::size_t>(StyleState::Hover)].decoration = hover;
        BoxDecoration selected;
        selected.background = ComponentStyle::ThemeColor(theme, "accent-soft", ColorFromHex(0x2F557A));
        selected.radius = CornerRadius::Uniform(4.0f);
        item->overrides[static_cast<std::size_t>(StyleState::Selected)].decoration = selected;
        item->overrides[static_cast<std::size_t>(StyleState::Selected)].foreground =
            ComponentStyle::ThemeColor(theme, "fg-strong", ColorFromHex(0xFFFFFF));
        s.itemStyle = std::move(item);
        return s;
    }

    void ApplyThemeDefaults() override {
        if (m_styleFromLayout) {
            return;
        }
        const Theme* theme = (m_context && m_context->theme) ? m_context->theme : nullptr;
        m_style = DefaultStyle(theme);
        SyncLegacyColorsFromStyle();
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
        bool changed = false;
        if (m_hasFocus) {
            m_hasFocus = false;
            changed = true;
        }
        if (m_hoveredIndex != -1) {
            m_hoveredIndex = -1;
            changed = true;
        }
        return changed;
    }

    bool OnKeyDown(WPARAM keyCode, LPARAM /*lParam*/) override {
        if (m_items.empty()) {
            return false;
        }

        switch (keyCode) {
            case VK_UP:
            case VK_LEFT:
                return MoveSelectionBy(-1);
            case VK_DOWN:
            case VK_RIGHT:
                return MoveSelectionBy(1);
            case VK_HOME:
                return SetSelectedIndex(0);
            case VK_END:
                return SetSelectedIndex(static_cast<int>(m_items.size()) - 1);
            case VK_PRIOR:
                return MoveSelectionBy(-VisibleRowCount());
            case VK_NEXT:
                return MoveSelectionBy(VisibleRowCount());
            case VK_RETURN:
            case VK_SPACE:
                return m_selectedIndex >= 0;
            default:
                return false;
        }
    }

    bool OnMouseDown(float x, float y) override {
        if (!HitTest(x, y)) {
            return false;
        }
        const int index = IndexFromPoint(y);
        if (index >= 0) {
            return SetSelectedIndex(index) || true;
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
        float preferred = ScaleValue(220.0f);
        const float horizontalPadding = ScaleValue(22.0f);
        const float iconWidth = ScaleValue(iconSize);
        const float iconGapWidth = ScaleValue(iconGap);
        for (const auto& item : m_items) {
            const Size measured = MeasureTextValue(item.text, fontSize, 4096.0f, TextWrapMode::NoWrap);
            preferred = std::max(preferred, measured.width + horizontalPadding * 2.0f);
            if (item.HasIcon()) {
                preferred = std::max(preferred, measured.width + horizontalPadding * 2.0f + iconWidth + iconGapWidth);
            }
        }
        return std::min(availableWidth, preferred);
    }

    float MeasurePreferredHeight(float width) const override {
        (void)width;
        const int rowCount = std::clamp(static_cast<int>(m_items.size()), 3, maxVisibleItems);
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
            const std::wstring message = L"(empty menu)";
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
        const float rowHeightPx = std::max(1.0f, ScaleValue(itemHeight));
        const Rect contentRect = Rect::Make(
            m_bounds.left + inset,
            m_bounds.top + inset,
            m_bounds.right - inset,
            m_bounds.bottom - inset);
        const int visibleRows = VisibleRowCount();
        const int maxStart = std::max(0, static_cast<int>(m_items.size()) - visibleRows);
        m_scrollOffset = std::clamp(m_scrollOffset, 0, maxStart);
        const int endIndex = std::min(static_cast<int>(m_items.size()), m_scrollOffset + visibleRows);
        const TextRenderOptions textOptions{
            TextWrapMode::NoWrap,
            TextHorizontalAlign::Start,
            TextVerticalAlign::Center
        };

        renderer.PushRoundedClip(m_bounds, scaledCornerRadius);
        for (int i = m_scrollOffset; i < endIndex; ++i) {
            const float rowTop = contentRect.top + static_cast<float>(i - m_scrollOffset) * rowHeightPx;
            const float rowBottom = std::min(contentRect.bottom, rowTop + rowHeightPx);
            const Rect rowRect = Rect::Make(contentRect.left, rowTop, contentRect.right, rowBottom);

            StyleState itemState = StyleState::Normal;
            if (i == m_selectedIndex) {
                itemState = StyleState::Selected;
            } else if (i == m_hoveredIndex) {
                itemState = StyleState::Hover;
            }
            Color rowBg = Color{0, 0, 0, 0};
            Color rowFg = textColor;
            float rowRadius = ScaleValue(4.0f);
            if (m_style.itemStyle) {
                const StyleSpec itemSpec = m_style.itemStyle->Resolve(itemState);
                if (itemSpec.decoration.has_value()) {
                    rowBg = itemSpec.decoration->background;
                    if (!itemSpec.decoration->radius.IsZero()) {
                        rowRadius = ScaleValue(itemSpec.decoration->radius.MaxRadius());
                    }
                }
                if (itemSpec.foreground.has_value()) {
                    rowFg = *itemSpec.foreground;
                }
            } else {
                if (itemState == StyleState::Selected) {
                    rowBg = selectedBackground;
                } else if (itemState == StyleState::Hover) {
                    rowBg = hoverBackground;
                }
            }
            if (rowBg.a > 0.001f) {
                renderer.FillRoundedRect(rowRect, rowBg, rowRadius);
            }

            const Rect textRect = Rect::Make(
                rowRect.left + ScaleValue(10.0f),
                rowRect.top,
                rowRect.right - ScaleValue(10.0f),
                rowRect.bottom);
            EnsureStripItemSvg(renderer, m_items[i]);
            Rect drawTextRect = textRect;
            if (m_items[i].HasIcon()) {
                const float iconPx = ScaleValue(iconSize);
                const float iconTop = rowRect.top + std::max(0.0f, (rowRect.Height() - iconPx) * 0.5f);
                const Rect iconRect = Rect::Make(
                    rowRect.left + ScaleValue(10.0f),
                    iconTop,
                    rowRect.left + ScaleValue(10.0f) + iconPx,
                    iconTop + iconPx);
                if (m_items[i].svg) {
                    renderer.DrawSvg(m_items[i].svg, iconRect);
                }
                drawTextRect.left = iconRect.right + ScaleValue(iconGap);
            }
            renderer.DrawTextW(
                m_items[i].text.c_str(),
                static_cast<UINT32>(m_items[i].text.size()),
                drawTextRect,
                rowFg,
                ScaleValue(fontSize),
                textOptions);
        }

        if (visibleRows < static_cast<int>(m_items.size())) {
            const float thumbWidth = ScaleValue(4.0f);
            const float thumbRight = m_bounds.right - ScaleValue(3.0f);
            const float thumbLeft = thumbRight - thumbWidth;
            const float ratio = static_cast<float>(visibleRows) / static_cast<float>(m_items.size());
            const float thumbHeight = std::max(ScaleValue(14.0f), contentRect.Height() * ratio);
            const float maxOffsetFloat = static_cast<float>(std::max(1, static_cast<int>(m_items.size()) - visibleRows));
            const float offsetRatio = static_cast<float>(m_scrollOffset) / maxOffsetFloat;
            const float thumbTop = contentRect.top + (contentRect.Height() - thumbHeight) * offsetRatio;
            const Rect thumbRect = Rect::Make(thumbLeft, thumbTop, thumbRight, thumbTop + thumbHeight);
            renderer.FillRoundedRect(thumbRect, scrollThumbColor, ScaleValue(2.0f));
        }
        renderer.PopLayer();
    }

    void SetItems(std::vector<std::wstring> items) {
        std::vector<StripItem> entries;
        entries.reserve(items.size());
        for (auto& item : items) {
            StripItem entry;
            entry.text = std::move(item);
            entries.push_back(std::move(entry));
        }
        SetEntries(std::move(entries));
    }

    void SetEntries(std::vector<StripItem> items) {
        m_items = std::move(items);
        if (m_items.empty()) {
            m_selectedIndex = -1;
            m_hoveredIndex = -1;
            m_scrollOffset = 0;
            return;
        }
        if (m_selectedIndex < 0) {
            m_selectedIndex = 0;
        } else {
            m_selectedIndex = std::clamp(m_selectedIndex, 0, static_cast<int>(m_items.size()) - 1);
        }
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

    Color background = ColorFromHex(0x132132);
    Color borderColor = ColorFromHex(0x486076);
    Color focusBorderColor = ColorFromHex(0xFFFFFF);
    Color hoverBackground = ColorFromHex(0x1F3449);
    Color selectedBackground = ColorFromHex(0x2F557A);
    Color scrollThumbColor = ColorFromHex(0x59728C);
    Color textColor = ColorFromHex(0xEDF4FF);
    Color mutedTextColor = ColorFromHex(0x98AABD);
    float cornerRadius = 10.0f;
    float fontSize = 13.0f;
    float itemHeight = 28.0f;
    int maxVisibleItems = 8;
    float iconSize = 14.0f;
    float iconGap = 8.0f;

private:
    void SyncLegacyColorsFromStyle() {
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
        if (m_style.itemStyle) {
            const StyleSpec hover = m_style.itemStyle->Resolve(StyleState::Hover);
            const StyleSpec selected = m_style.itemStyle->Resolve(StyleState::Selected);
            if (hover.decoration.has_value()) {
                hoverBackground = hover.decoration->background;
            }
            if (selected.decoration.has_value()) {
                selectedBackground = selected.decoration->background;
            }
        }
    }

    int VisibleRowCount() const {
        const float innerHeight = std::max(1.0f, m_bounds.Height() - ScaleValue(8.0f));
        const float rowHeightPx = std::max(1.0f, ScaleValue(itemHeight));
        return std::max(1, static_cast<int>(std::floor(innerHeight / rowHeightPx)));
    }

    int IndexFromPoint(float y) const {
        if (m_items.empty()) {
            return -1;
        }
        const float inset = ScaleValue(4.0f);
        const float rowHeightPx = std::max(1.0f, ScaleValue(itemHeight));
        const float localY = y - (m_bounds.top + inset);
        if (localY < 0.0f) {
            return -1;
        }
        const int row = static_cast<int>(std::floor(localY / rowHeightPx));
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

    std::vector<StripItem> m_items;
    int m_selectedIndex = -1;
    int m_hoveredIndex = -1;
    int m_scrollOffset = 0;
};
