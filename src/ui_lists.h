#pragma once
// Component module - part of the ai-win-ui library.
// See ui.h for the full umbrella include.
#include "ui_text.h"

class ListBox : public UIElement {
public:
    ListBox() { m_style = DefaultStyle(nullptr); }

    static ComponentStyle DefaultStyle(const Theme* theme) {
        ComponentStyle s;
        BoxDecoration shell;
        shell.background = ComponentStyle::ThemeColor(theme, "surface-2", ColorFromHex(0x14202D));
        shell.border.width = Thickness{1, 1, 1, 1};
        shell.border.color = ComponentStyle::ThemeColor(theme, "border-subtle", ColorFromHex(0x34485C));
        shell.radius = CornerRadius::Uniform(ComponentStyle::ThemeNumber(theme, Theme::NumberCategory::Radius, "md", 8.0f));
        s.base.decoration = shell;
        s.base.foreground = ComponentStyle::ThemeColor(theme, "fg", ColorFromHex(0xE7EFFA));
        s.base.fontSize = ComponentStyle::ThemeNumber(theme, Theme::NumberCategory::FontSize, "sm", 13.0f);

        BoxDecoration focused = shell;
        focused.border.width = Thickness{2, 2, 2, 2};
        focused.border.color = ComponentStyle::ThemeColor(theme, "border-focus", ColorFromHex(0xFFFFFF));
        s.overrides[static_cast<std::size_t>(StyleState::Focused)].decoration = focused;

        auto item = std::make_unique<ComponentStyle>();
        item->base.decoration = BoxDecoration{};
        item->base.decoration->background = Color{0, 0, 0, 0};
        item->base.foreground = ComponentStyle::ThemeColor(theme, "fg", ColorFromHex(0xE7EFFA));
        BoxDecoration hover;
        hover.background = ComponentStyle::ThemeColor(theme, "surface-3", ColorFromHex(0x1F3247));
        hover.radius = CornerRadius::Uniform(4.0f);
        item->overrides[static_cast<std::size_t>(StyleState::Hover)].decoration = hover;
        BoxDecoration selected;
        selected.background = ComponentStyle::ThemeColor(theme, "accent-soft", ColorFromHex(0x2F4F77));
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
            case VK_UP: return MoveSelectionBy(-1);
            case VK_DOWN: return MoveSelectionBy(1);
            case VK_HOME: return SetSelectedIndex(0);
            case VK_END: return SetSelectedIndex(static_cast<int>(m_items.size()) - 1);
            case VK_PRIOR: return MoveSelectionBy(-VisibleRowCount());
            case VK_NEXT: return MoveSelectionBy(VisibleRowCount());
            default: break;
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

    // Consume wheel when rows overflow; return false at edges so outer ScrollViewer can pan.
    bool OnMouseWheel(float delta, float x, float y, bool /*shiftHeld*/) override {
        if (!HitTest(x, y)) {
            return false;
        }
        if (m_items.empty()) {
            return false;
        }
        const int before = m_scrollOffset;
        const int visibleRows = VisibleRowCount();
        m_scrollOffset = std::clamp(
            m_scrollOffset - static_cast<int>(std::lround(delta * 3.0f)),
            0,
            std::max(0, static_cast<int>(m_items.size()) - visibleRows));
        return m_scrollOffset != before;
    }

protected:
    float MeasurePreferredWidth(float availableWidth) const override {
        float preferred = ScaleValue(240.0f);
        if (!m_items.empty()) {
            const float horizontalPadding = ScaleValue(16.0f);
            float widest = 0.0f;
            for (size_t i = 0; i < std::min<size_t>(m_items.size(), 24); ++i) {
                const Size measured = MeasureTextValue(m_items[i], fontSize, 4096.0f, TextWrapMode::NoWrap);
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
        const StyleSpec shell = m_style.Resolve(GetCurrentState());
        BoxDecoration deco = shell.decoration.value_or(BoxDecoration{});
        if (!shell.decoration.has_value()) {
            deco.background = background;
            deco.border.width = Thickness{1, 1, 1, 1};
            deco.border.color = borderColor;
            deco.radius = CornerRadius::Uniform(cornerRadius);
        }
        deco.radius = CornerRadius::Uniform(ScaleValue(deco.radius.IsZero() ? cornerRadius : deco.radius.MaxRadius()));
        deco.border.width = Thickness{
            ScaleValue(deco.border.width.left), ScaleValue(deco.border.width.top),
            ScaleValue(deco.border.width.right), ScaleValue(deco.border.width.bottom)};
        DrawBoxDecoration(renderer, m_bounds, deco);

        const Color shellFg = shell.foreground.value_or(textColor);
        const float shellFs = shell.fontSize.value_or(fontSize);

        if (m_items.empty()) {
            const std::wstring message = L"(empty)";
            const TextRenderOptions textOptions{TextWrapMode::NoWrap, TextHorizontalAlign::Center, TextVerticalAlign::Center};
            renderer.DrawTextW(message.c_str(), static_cast<UINT32>(message.size()), m_bounds, mutedTextColor, ScaleValue(shellFs), textOptions);
            return;
        }

        const float inset = ScaleValue(4.0f);
        const float rowHeight = ScaleValue(itemHeight);
        const Rect contentRect = Rect::Make(m_bounds.left + inset, m_bounds.top + inset, m_bounds.right - inset, m_bounds.bottom - inset);
        const int visibleRows = std::max(1, static_cast<int>(std::floor(contentRect.Height() / std::max(1.0f, rowHeight))));
        const int maxStart = std::max(0, static_cast<int>(m_items.size()) - visibleRows);
        m_scrollOffset = std::clamp(m_scrollOffset, 0, maxStart);
        const int endIndex = std::min(static_cast<int>(m_items.size()), m_scrollOffset + visibleRows);

        const ComponentStyle* itemStyle = m_style.itemStyle ? m_style.itemStyle.get() : nullptr;

        for (int i = m_scrollOffset; i < endIndex; ++i) {
            const float rowTop = contentRect.top + static_cast<float>(i - m_scrollOffset) * rowHeight;
            const float rowBottom = std::min(contentRect.bottom, rowTop + rowHeight);
            const Rect rowRect = Rect::Make(contentRect.left, rowTop, contentRect.right, rowBottom);

            StyleState itemState = StyleState::Normal;
            if (i == m_selectedIndex) {
                itemState = StyleState::Selected;
            } else if (i == m_hoveredIndex) {
                itemState = StyleState::Hover;
            }

            Color rowBg = Color{0, 0, 0, 0};
            Color rowFg = shellFg;
            float rowRadius = ScaleValue(4.0f);
            if (itemStyle) {
                const StyleSpec itemSpec = itemStyle->Resolve(itemState);
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
                if (i == m_selectedIndex) {
                    rowBg = selectedBackground;
                } else if (i == m_hoveredIndex) {
                    rowBg = hoverBackground;
                }
            }

            if (rowBg.a > 0.001f) {
                renderer.FillRoundedRect(rowRect, rowBg, rowRadius);
            }

            const Rect textRect = Rect::Make(rowRect.left + ScaleValue(8.0f), rowRect.top, rowRect.right - ScaleValue(8.0f), rowRect.bottom);
            const TextRenderOptions textOptions{TextWrapMode::NoWrap, TextHorizontalAlign::Start, TextVerticalAlign::Center};
            renderer.DrawTextW(m_items[i].c_str(), static_cast<UINT32>(m_items[i].size()), textRect, rowFg, ScaleValue(shellFs), textOptions);
        }

        if (visibleRows < static_cast<int>(m_items.size())) {
            const float barWidth = ScaleValue(4.0f);
            const float barRight = m_bounds.right - ScaleValue(3.0f);
            const float barLeft = barRight - barWidth;
            const float ratio = static_cast<float>(visibleRows) / static_cast<float>(m_items.size());
            const float thumbHeight = std::max(ScaleValue(14.0f), contentRect.Height() * ratio);
            const float maxOffset = static_cast<float>(std::max(1, static_cast<int>(m_items.size()) - visibleRows));
            const float offsetRatio = static_cast<float>(m_scrollOffset) / maxOffset;
            const float thumbTop = contentRect.top + (contentRect.Height() - thumbHeight) * offsetRatio;
            renderer.FillRoundedRect(Rect::Make(barLeft, thumbTop, barRight, thumbTop + thumbHeight), scrollThumbColor, ScaleValue(2.0f));
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
    float itemHeight = 24.0f;

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

// Wave2 C5: virtualized list 鈥?O(visible) paint for large itemCount (1k+).
// Text via binder or "prefix + index"; no full string vector allocation required.
class VirtualListBox : public UIElement {
public:
    using ItemTextBinder = std::function<std::wstring(int index)>;

    VirtualListBox() { m_style = DefaultStyle(nullptr); }

    static ComponentStyle DefaultStyle(const Theme* theme) {
        ComponentStyle s;
        BoxDecoration shell;
        shell.background = ComponentStyle::ThemeColor(theme, "surface-2", ColorFromHex(0x14202D));
        shell.border.width = Thickness{1, 1, 1, 1};
        shell.border.color = ComponentStyle::ThemeColor(theme, "border-subtle", ColorFromHex(0x34485C));
        shell.radius = CornerRadius::Uniform(ComponentStyle::ThemeNumber(theme, Theme::NumberCategory::Radius, "md", 8.0f));
        s.base.decoration = shell;
        s.base.foreground = ComponentStyle::ThemeColor(theme, "fg", ColorFromHex(0xE7EFFA));
        s.base.fontSize = ComponentStyle::ThemeNumber(theme, Theme::NumberCategory::FontSize, "sm", 13.0f);
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

    void SetItemCount(int count) {
        m_itemCount = std::max(0, count);
        m_selectedIndex = std::min(m_selectedIndex, m_itemCount - 1);
        if (m_itemCount == 0) {
            m_selectedIndex = -1;
        }
        ClampScroll();
    }
    int ItemCount() const { return m_itemCount; }

    void SetItemPrefix(std::wstring prefix) { m_itemPrefix = std::move(prefix); }
    void SetItemBinder(ItemTextBinder binder) { m_binder = std::move(binder); }

    bool IsFocusable() const override { return true; }
    bool IsLayoutLeaf() const override { return true; }

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
        if (m_itemCount <= 0) {
            return false;
        }
        switch (keyCode) {
            case VK_UP: return MoveSelectionBy(-1);
            case VK_DOWN: return MoveSelectionBy(1);
            case VK_HOME: return SetSelectedIndex(0);
            case VK_END: return SetSelectedIndex(m_itemCount - 1);
            case VK_PRIOR: return MoveSelectionBy(-VisibleRowCount());
            case VK_NEXT: return MoveSelectionBy(VisibleRowCount());
            default: break;
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
        const int next = IndexFromPoint(y);
        if (next == m_hoveredIndex) {
            return false;
        }
        m_hoveredIndex = next;
        return true;
    }

    bool OnMouseLeave() override {
        if (m_hoveredIndex == -1) {
            return false;
        }
        m_hoveredIndex = -1;
        return true;
    }

    bool OnMouseWheel(float delta, float x, float y, bool /*shiftHeld*/) override {
        if (!HitTest(x, y) || m_itemCount <= 0) {
            return false;
        }
        const int before = m_scrollOffset;
        m_scrollOffset -= static_cast<int>(std::lround(delta * 3.0f));
        ClampScroll();
        return m_scrollOffset != before;
    }

protected:
    float MeasurePreferredWidth(float availableWidth) const override {
        return std::min(availableWidth, ScaleValue(320.0f));
    }
    float MeasurePreferredHeight(float width) const override {
        (void)width;
        if (HasFixedHeight()) {
            return ScaleValue(m_fixedHeight);
        }
        return ScaleValue(8.0f) + ScaleValue(itemHeight) * 12.0f + ScaleValue(8.0f);
    }

public:
    void Render(IRenderer& renderer) override {
        const StyleSpec shell = m_style.Resolve(GetCurrentState());
        BoxDecoration deco = shell.decoration.value_or(BoxDecoration{});
        if (!shell.decoration.has_value()) {
            deco.background = background;
            deco.border.width = Thickness{1, 1, 1, 1};
            deco.border.color = borderColor;
            deco.radius = CornerRadius::Uniform(cornerRadius);
        }
        deco.radius = CornerRadius::Uniform(ScaleValue(deco.radius.IsZero() ? cornerRadius : deco.radius.MaxRadius()));
        deco.border.width = Thickness{
            ScaleValue(deco.border.width.left), ScaleValue(deco.border.width.top),
            ScaleValue(deco.border.width.right), ScaleValue(deco.border.width.bottom)};
        DrawBoxDecoration(renderer, m_bounds, deco);

        const Color shellFg = shell.foreground.value_or(textColor);
        const float shellFs = shell.fontSize.value_or(fontSize);
        if (m_itemCount <= 0) {
            const std::wstring message = L"(empty virtual list)";
            const TextRenderOptions textOptions{TextWrapMode::NoWrap, TextHorizontalAlign::Center, TextVerticalAlign::Center};
            renderer.DrawTextW(message.c_str(), static_cast<UINT32>(message.size()), m_bounds, mutedTextColor, ScaleValue(shellFs), textOptions);
            return;
        }

        const float inset = ScaleValue(4.0f);
        const float rowHeight = std::max(1.0f, ScaleValue(itemHeight));
        const Rect contentRect = Rect::Make(
            m_bounds.left + inset, m_bounds.top + inset,
            m_bounds.right - inset, m_bounds.bottom - inset);
        const int visibleRows = std::max(1, static_cast<int>(std::floor(contentRect.Height() / rowHeight)));
        ClampScroll();
        const int endIndex = std::min(m_itemCount, m_scrollOffset + visibleRows + 1);

        renderer.PushRoundedClip(contentRect, 0.0f);
        for (int i = m_scrollOffset; i < endIndex; ++i) {
            const float rowTop = contentRect.top + static_cast<float>(i - m_scrollOffset) * rowHeight;
            const float rowBottom = std::min(contentRect.bottom, rowTop + rowHeight);
            const Rect rowRect = Rect::Make(contentRect.left, rowTop, contentRect.right, rowBottom);

            Color rowBg = Color{0, 0, 0, 0};
            Color rowFg = shellFg;
            if (i == m_selectedIndex) {
                rowBg = selectedBackground;
                rowFg = ColorFromHex(0xFFFFFF);
            } else if (i == m_hoveredIndex) {
                rowBg = hoverBackground;
            }
            if (rowBg.a > 0.001f) {
                renderer.FillRoundedRect(rowRect, rowBg, ScaleValue(4.0f));
            }

            const std::wstring text = ItemTextAt(i);
            const Rect textRect = Rect::Make(
                rowRect.left + ScaleValue(8.0f), rowRect.top,
                rowRect.right - ScaleValue(8.0f), rowRect.bottom);
            const TextRenderOptions textOptions{TextWrapMode::NoWrap, TextHorizontalAlign::Start, TextVerticalAlign::Center};
            renderer.DrawTextW(text.c_str(), static_cast<UINT32>(text.size()), textRect, rowFg, ScaleValue(shellFs), textOptions);
        }
        renderer.PopLayer();

        // Scroll thumb (approximate).
        if (m_itemCount > visibleRows) {
            const float barWidth = ScaleValue(4.0f);
            const float barRight = m_bounds.right - ScaleValue(3.0f);
            const float barLeft = barRight - barWidth;
            const float ratio = static_cast<float>(visibleRows) / static_cast<float>(m_itemCount);
            const float thumbHeight = std::max(ScaleValue(14.0f), contentRect.Height() * ratio);
            const float maxOffset = static_cast<float>(std::max(1, m_itemCount - visibleRows));
            const float offsetRatio = static_cast<float>(m_scrollOffset) / maxOffset;
            const float thumbTop = contentRect.top + (contentRect.Height() - thumbHeight) * offsetRatio;
            renderer.FillRoundedRect(
                Rect::Make(barLeft, thumbTop, barRight, thumbTop + thumbHeight),
                scrollThumbColor,
                ScaleValue(2.0f));
        }
    }

    bool SetSelectedIndex(int index) {
        if (m_itemCount <= 0) {
            if (m_selectedIndex == -1) {
                return false;
            }
            m_selectedIndex = -1;
            return true;
        }
        const int clamped = std::clamp(index, 0, m_itemCount - 1);
        if (clamped == m_selectedIndex) {
            EnsureSelectionVisible();
            return false;
        }
        m_selectedIndex = clamped;
        EnsureSelectionVisible();
        return true;
    }
    int SelectedIndex() const { return m_selectedIndex; }

    Color background = ColorFromHex(0x14202D);
    Color borderColor = ColorFromHex(0x34485C);
    Color selectedBackground = ColorFromHex(0x2F4F77);
    Color hoverBackground = ColorFromHex(0x1F3247);
    Color scrollThumbColor = ColorFromHex(0x536A82);
    Color textColor = ColorFromHex(0xE7EFFA);
    Color mutedTextColor = ColorFromHex(0x95A7BA);
    float cornerRadius = 8.0f;
    float fontSize = 13.0f;
    float itemHeight = 24.0f;

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

    std::wstring ItemTextAt(int index) const {
        if (m_binder) {
            return m_binder(index);
        }
        return m_itemPrefix + std::to_wstring(index);
    }

    int VisibleRowCount() const {
        const float innerHeight = std::max(1.0f, m_bounds.Height() - ScaleValue(8.0f));
        const float rowHeight = std::max(1.0f, ScaleValue(itemHeight));
        return std::max(1, static_cast<int>(std::floor(innerHeight / rowHeight)));
    }

    void ClampScroll() {
        const int maxStart = std::max(0, m_itemCount - VisibleRowCount());
        m_scrollOffset = std::clamp(m_scrollOffset, 0, maxStart);
    }

    int IndexFromPoint(float y) const {
        if (m_itemCount <= 0) {
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
        return index < m_itemCount ? index : -1;
    }

    bool MoveSelectionBy(int delta) {
        if (m_itemCount <= 0) {
            return false;
        }
        if (m_selectedIndex < 0) {
            return SetSelectedIndex(0);
        }
        return SetSelectedIndex(m_selectedIndex + delta);
    }

    void EnsureSelectionVisible() {
        if (m_selectedIndex < 0 || m_itemCount <= 0) {
            m_scrollOffset = 0;
            return;
        }
        const int visibleRows = VisibleRowCount();
        if (m_selectedIndex < m_scrollOffset) {
            m_scrollOffset = m_selectedIndex;
        } else if (m_selectedIndex >= m_scrollOffset + visibleRows) {
            m_scrollOffset = m_selectedIndex - visibleRows + 1;
        }
        ClampScroll();
    }

    int m_itemCount = 0;
    int m_selectedIndex = -1;
    int m_hoveredIndex = -1;
    int m_scrollOffset = 0;
    std::wstring m_itemPrefix = L"Item ";
    ItemTextBinder m_binder;
};

// Shared overlay flyout geometry (Wave2 C4): ComboBox + Popup use the same placement rules.
namespace OverlayFlyout {
inline bool PreferOpenAbove(float headerTop, float headerBottom, float listHeight, float viewportH, float margin) {
    if (viewportH <= 1.0f) {
        return false;
    }
    const float spaceBelow = std::max(0.0f, viewportH - headerBottom - margin);
    const float spaceAbove = std::max(0.0f, headerTop - margin);
    if (spaceBelow >= listHeight) {
        return false;
    }
    return spaceAbove > spaceBelow;
}

inline Rect PlaceVertical(
    const Rect& header,
    float listHeight,
    float viewportH,
    float margin,
    bool forceAbove) {
    float height = listHeight;
    const bool openUp = forceAbove || PreferOpenAbove(header.top, header.bottom, listHeight, viewportH, margin);
    if (viewportH > 1.0f) {
        const float spaceBelow = std::max(0.0f, viewportH - header.bottom - margin);
        const float spaceAbove = std::max(0.0f, header.top - margin);
        const float available = openUp ? spaceAbove : spaceBelow;
        if (available > 0.0f) {
            height = std::min(height, available);
        }
        height = std::max(height, std::min(1.0f, available > 0.0f ? available : height));
    }
    if (openUp) {
        return Rect::Make(header.left, header.top - height, header.right, header.top);
    }
    return Rect::Make(header.left, header.bottom, header.right, header.bottom + height);
}
} // namespace OverlayFlyout

class ComboBox : public UIElement {
public:
    ComboBox() { m_style = DefaultStyle(nullptr); }

    static ComponentStyle DefaultStyle(const Theme* theme) {
        ComponentStyle s;
        BoxDecoration shell;
        shell.background = ComponentStyle::ThemeColor(theme, "surface-2", ColorFromHex(0x13202D));
        shell.border.width = Thickness{1, 1, 1, 1};
        shell.border.color = ComponentStyle::ThemeColor(theme, "border-subtle", ColorFromHex(0x36506A));
        shell.radius = CornerRadius::Uniform(ComponentStyle::ThemeNumber(theme, Theme::NumberCategory::Radius, "md", 8.0f));
        s.base.decoration = shell;
        s.base.foreground = ComponentStyle::ThemeColor(theme, "fg", ColorFromHex(0xE9F2FD));
        s.base.fontSize = ComponentStyle::ThemeNumber(theme, Theme::NumberCategory::FontSize, "sm", 13.0f);

        auto dropdown = std::make_unique<ComponentStyle>();
        BoxDecoration drop;
        drop.background = ComponentStyle::ThemeColor(theme, "surface-1", ColorFromHex(0x112130));
        drop.border.width = Thickness{1, 1, 1, 1};
        drop.border.color = ComponentStyle::ThemeColor(theme, "border-subtle", ColorFromHex(0x36506A));
        dropdown->base.decoration = drop;
        s.dropdownStyle = std::move(dropdown);

        auto item = std::make_unique<ComponentStyle>();
        item->base.foreground = ComponentStyle::ThemeColor(theme, "fg", ColorFromHex(0xE9F2FD));
        BoxDecoration hover;
        hover.background = ComponentStyle::ThemeColor(theme, "surface-3", ColorFromHex(0x20374F));
        item->overrides[static_cast<std::size_t>(StyleState::Hover)].decoration = hover;
        BoxDecoration selected;
        selected.background = ComponentStyle::ThemeColor(theme, "accent-soft", ColorFromHex(0x2F4F77));
        item->overrides[static_cast<std::size_t>(StyleState::Selected)].decoration = selected;
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

    // Layout size is always the closed header. Expanded list is an overlay popup.
    bool HitTest(float x, float y) const override {
        if (UIElement::HitTest(x, y)) {
            return true;
        }
        if (m_expanded) {
            return DropdownRect().Contains(x, y);
        }
        return false;
    }

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
        // Closed control only - dropdown is painted as an overlay below the header.
        return ScaleValue(headerHeight);
    }

public:
    void Render(IRenderer& renderer) override {
        const StyleSpec shell = m_style.Resolve(m_hasFocus ? StyleState::Focused : StyleState::Normal);
        Color shellBg = background;
        Color shellBorder = borderColor;
        float scaledCornerRadius = ScaleValue(cornerRadius);
        Color shellFg = textColor;
        float shellFs = fontSize;
        if (shell.decoration.has_value()) {
            shellBg = shell.decoration->background;
            shellBorder = shell.decoration->border.color;
            if (!shell.decoration->radius.IsZero()) {
                scaledCornerRadius = ScaleValue(shell.decoration->radius.MaxRadius());
            }
        }
        if (shell.foreground.has_value()) {
            shellFg = *shell.foreground;
        }
        if (shell.fontSize.has_value()) {
            shellFs = *shell.fontSize;
        }

        // Header only in the normal pass; list is drawn in RenderOverlay so it sits above siblings.
        const Rect header = HeaderRect();
        renderer.FillRoundedRect(header, shellBg, scaledCornerRadius);
        renderer.DrawRoundedRect(header, shellBorder, 1.0f, scaledCornerRadius);
        if (m_hasFocus) {
            renderer.DrawRoundedRect(header, focusBorderColor, 2.0f, scaledCornerRadius);
        }

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
            shellFg,
            ScaleValue(shellFs),
            textOptions);

        const float arrowCenterX = header.right - ScaleValue(14.0f);
        const float arrowCenterY = header.top + header.Height() * 0.5f;
        const float arrowSize = ScaleValue(4.0f);
        // When list opens below, show "up" chevron; when open above or closed, show "down".
        const bool listAbove = m_expanded && ShouldOpenDropdownUpward(IdealDropdownHeight());
        if (m_expanded && !listAbove) {
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
    }

    void RenderOverlay(IRenderer& renderer) override {
        if (!m_expanded) {
            return;
        }

        const Rect dropdownRect = DropdownRect();
        if (dropdownRect.Height() <= 1.0f) {
            return;
        }

        Color dropBg = dropdownBackground;
        Color dropBorder = borderColor;
        if (m_style.dropdownStyle && m_style.dropdownStyle->base.decoration.has_value()) {
            dropBg = m_style.dropdownStyle->base.decoration->background;
            dropBorder = m_style.dropdownStyle->base.decoration->border.color;
        }
        renderer.FillRect(dropdownRect, dropBg);
        renderer.DrawRect(dropdownRect, dropBorder, 1.0f);

        const TextRenderOptions textOptions{
            TextWrapMode::NoWrap,
            TextHorizontalAlign::Start,
            TextVerticalAlign::Center
        };
        const float rowHeight = std::max(1.0f, ScaleValue(itemHeight));
        const int visibleRows = std::max(1, static_cast<int>(std::floor(dropdownRect.Height() / rowHeight)));
        const int count = std::min(static_cast<int>(m_items.size()), visibleRows);
        for (int i = 0; i < count; ++i) {
            const float top = dropdownRect.top + static_cast<float>(i) * rowHeight;
            const float bottom = std::min(dropdownRect.bottom, top + rowHeight);
            const Rect rowRect = Rect::Make(dropdownRect.left, top, dropdownRect.right, bottom);

            StyleState itemState = StyleState::Normal;
            if (i == m_selectedIndex) {
                itemState = StyleState::Selected;
            } else if (i == m_hoveredIndex) {
                itemState = StyleState::Hover;
            }
            Color rowBg = Color{0, 0, 0, 0};
            Color rowFg = textColor;
            if (m_style.itemStyle) {
                const StyleSpec itemSpec = m_style.itemStyle->Resolve(itemState);
                if (itemSpec.decoration.has_value()) {
                    rowBg = itemSpec.decoration->background;
                }
                if (itemSpec.foreground.has_value()) {
                    rowFg = *itemSpec.foreground;
                }
            } else {
                if (i == m_selectedIndex) {
                    rowBg = selectedBackground;
                } else if (i == m_hoveredIndex) {
                    rowBg = hoverBackground;
                }
            }
            if (rowBg.a > 0.001f) {
                renderer.FillRect(rowRect, rowBg);
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
                rowFg,
                ScaleValue(fontSize),
                textOptions);
        }
    }

    UIElement* FindOverlayHitAt(float x, float y) override {
        if (m_expanded && DropdownRect().Contains(x, y)) {
            return this;
        }
        return nullptr;
    }

    bool DismissOverlaysAt(float x, float y) override {
        bool changed = UIElement::DismissOverlaysAt(x, y);
        if (m_expanded &&
            !HeaderRect().Contains(x, y) &&
            !DropdownRect().Contains(x, y)) {
            m_expanded = false;
            m_hoveredIndex = -1;
            changed = true;
        }
        return changed;
    }

    bool DismissAllOverlays() override {
        bool changed = UIElement::DismissAllOverlays();
        if (m_expanded) {
            m_expanded = false;
            m_hoveredIndex = -1;
            changed = true;
        }
        return changed;
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
    float headerHeight = 30.0f;
    float itemHeight = 26.0f;
    int maxVisibleItems = 5;

private:
    void SyncLegacyColorsFromStyle() {
        if (m_style.base.decoration.has_value()) {
            background = m_style.base.decoration->background;
            headerBackground = m_style.base.decoration->background;
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
        if (m_style.dropdownStyle && m_style.dropdownStyle->base.decoration.has_value()) {
            dropdownBackground = m_style.dropdownStyle->base.decoration->background;
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

    Rect HeaderRect() const {
        // Always use design header height - layout bounds are header-sized.
        const float height = ScaleValue(headerHeight);
        return Rect::Make(m_bounds.left, m_bounds.top, m_bounds.right, m_bounds.top + height);
    }

    // Ideal list height before viewport clamping.
    float IdealDropdownHeight() const {
        const int rows = std::max(1, std::min(static_cast<int>(m_items.size()), std::max(1, maxVisibleItems)));
        return ScaleValue(itemHeight) * static_cast<float>(rows);
    }

    // Open upward when there is not enough room below the header (shared OverlayFlyout rules).
    bool ShouldOpenDropdownUpward(float listHeight) const {
        const Rect header = HeaderRect();
        const float viewportH = (m_context && m_context->viewportHeight > 1.0f)
            ? m_context->viewportHeight
            : 0.0f;
        return OverlayFlyout::PreferOpenAbove(
            header.top, header.bottom, listHeight, viewportH, ScaleValue(4.0f));
    }

    Rect DropdownRect() const {
        const Rect header = HeaderRect();
        const float listHeight = IdealDropdownHeight();
        const float viewportH = (m_context && m_context->viewportHeight > 1.0f)
            ? m_context->viewportHeight
            : 0.0f;
        return OverlayFlyout::PlaceVertical(
            header, listHeight, viewportH, ScaleValue(4.0f), /*forceAbove=*/false);
    }

    int DropdownIndexFromPoint(float y) const {
        if (!m_expanded || m_items.empty()) {
            return -1;
        }
        const Rect dropdown = DropdownRect();
        if (y < dropdown.top || y > dropdown.bottom) {
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

// Generic light-dismiss flyout (Wave1 C4). Layout size = trigger chip only;
// body children paint/hit in RenderOverlay above siblings (same contract as ComboBox).
