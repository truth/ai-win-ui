#pragma once
// Component module - part of the ai-win-ui library.
// See ui.h for the full umbrella include.
#include "ui_overlays.h"

class TabControl : public UIElement {
public:
    TabControl() { m_style = DefaultStyle(nullptr); }

    static ComponentStyle DefaultStyle(const Theme* theme) {
        ComponentStyle s;
        BoxDecoration shell;
        shell.background = ComponentStyle::ThemeColor(theme, "surface-2", ColorFromHex(0x121F2E));
        shell.border.width = Thickness{1, 1, 1, 1};
        shell.border.color = ComponentStyle::ThemeColor(theme, "border-subtle", ColorFromHex(0x3B536B));
        shell.radius = CornerRadius::Uniform(ComponentStyle::ThemeNumber(theme, Theme::NumberCategory::Radius, "lg", 10.0f));
        s.base.decoration = shell;
        s.base.foreground = ComponentStyle::ThemeColor(theme, "fg", ColorFromHex(0xD6E4F2));
        s.base.fontSize = ComponentStyle::ThemeNumber(theme, Theme::NumberCategory::FontSize, "sm", 13.0f);

        auto tab = std::make_unique<ComponentStyle>();
        BoxDecoration tabBase;
        tabBase.background = ComponentStyle::ThemeColor(theme, "surface-3", ColorFromHex(0x1F3247));
        tab->base.decoration = tabBase;
        tab->base.foreground = ComponentStyle::ThemeColor(theme, "fg-muted", ColorFromHex(0xD6E4F2));
        BoxDecoration hover;
        hover.background = ComponentStyle::ThemeColor(theme, "surface-4", ColorFromHex(0x2A415A));
        tab->overrides[static_cast<std::size_t>(StyleState::Hover)].decoration = hover;
        BoxDecoration selected;
        selected.background = ComponentStyle::ThemeColor(theme, "accent-soft", ColorFromHex(0x355A82));
        tab->overrides[static_cast<std::size_t>(StyleState::Selected)].decoration = selected;
        tab->overrides[static_cast<std::size_t>(StyleState::Selected)].foreground =
            ComponentStyle::ThemeColor(theme, "fg-strong", ColorFromHex(0xFFFFFF));
        s.tabStyle = std::move(tab);
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

    // Own children (tab pages) are managed internally - Yoga must treat this as a leaf
    // so height/width attributes and preferred size are honored.
    bool IsLayoutLeaf() const override { return true; }

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

    void Measure(float availableWidth, float availableHeight) override {
        const float width = GetPreferredWidth(availableWidth);
        m_desiredSize.width = width;

        const float header = ScaleValue(std::max(1.0f, headerHeight));
        const float contentWidth = std::max(1.0f, width - ScaleValue(2.0f));
        // Measure every page so height fits the tallest tab (stable when switching).
        float body = ScaleValue(80.0f);
        for (auto& child : m_children) {
            if (!child) {
                continue;
            }
            child->Measure(contentWidth, availableHeight);
            body = std::max(body, child->DesiredSize().height);
        }
        const float natural = header + body + ScaleValue(4.0f);

        if (HasFixedHeight()) {
            m_desiredSize.height = GetPreferredHeight(width);
        } else {
            // ClampHeight already applies minHeight / maxHeight.
            m_desiredSize.height = ClampHeight(natural);
        }
        // Cap by parent constraint when it is a real finite height (not unbounded measure).
        if (availableHeight > 0.0f && availableHeight < 50000.0f) {
            m_desiredSize.height = std::min(m_desiredSize.height, availableHeight);
        }
    }

    void Arrange(const Rect& finalRect) override {
        UIElement::Arrange(finalRect);
        LayoutTabPages();
    }

protected:
    float MeasurePreferredWidth(float availableWidth) const override {
        return std::min(availableWidth, ScaleValue(520.0f));
    }

    float MeasurePreferredHeight(float width) const override {
        // Fallback when Measure() is not used (should be rare for TabControl).
        (void)width;
        return ScaleValue(headerHeight + 160.0f);
    }

public:
    void Render(IRenderer& renderer) override {
        // Ensure page layout is current even if only paint ran after a tab switch.
        if (m_bounds.Width() > 1.0f && m_bounds.Height() > 1.0f) {
            LayoutTabPages();
        }

        const StyleSpec shell = m_style.Resolve(m_hasFocus ? StyleState::Focused : StyleState::Normal);
        Color shellBg = background;
        Color shellBorder = borderColor;
        float scaledCornerRadius = ScaleValue(cornerRadius);
        float shellFs = fontSize;
        if (shell.decoration.has_value()) {
            shellBg = shell.decoration->background;
            shellBorder = shell.decoration->border.color;
            if (!shell.decoration->radius.IsZero()) {
                scaledCornerRadius = ScaleValue(shell.decoration->radius.MaxRadius());
            }
        }
        if (shell.fontSize.has_value()) {
            shellFs = *shell.fontSize;
        }

        renderer.FillRoundedRect(m_bounds, shellBg, scaledCornerRadius);
        renderer.DrawRoundedRect(m_bounds, shellBorder, 1.0f, scaledCornerRadius);
        if (m_hasFocus) {
            renderer.DrawRoundedRect(m_bounds, focusBorderColor, 2.0f, scaledCornerRadius);
        }

        const Rect header = HeaderRect();
        if (header.Height() > 0.5f && header.Width() > 0.5f) {
            renderer.FillRect(header, headerBackground);
            renderer.DrawLine(
                PointF{header.left, header.bottom},
                PointF{header.right, header.bottom},
                shellBorder,
                1.0f);

            const std::vector<Rect> tabs = BuildTabRects();
            const ComponentStyle* tabStyle = m_style.tabStyle ? m_style.tabStyle.get() : nullptr;
            for (size_t i = 0; i < tabs.size(); ++i) {
                const bool selected = static_cast<int>(i) == m_selectedIndex;
                const bool hovered = static_cast<int>(i) == m_hoveredTab;

                StyleState tabState = StyleState::Normal;
                if (selected) {
                    tabState = StyleState::Selected;
                } else if (hovered) {
                    tabState = StyleState::Hover;
                }

                Color tabBg = tabBackground;
                Color tabFg = selected ? selectedTabTextColor : tabTextColor;
                if (selected) {
                    tabBg = selectedTabBackground;
                } else if (hovered) {
                    tabBg = hoverTabBackground;
                }
                if (tabStyle) {
                    const StyleSpec tabSpec = tabStyle->Resolve(tabState);
                    if (tabSpec.decoration.has_value()) {
                        tabBg = tabSpec.decoration->background;
                    }
                    if (tabSpec.foreground.has_value()) {
                        tabFg = *tabSpec.foreground;
                    }
                }

                renderer.FillRect(tabs[i], tabBg);
                renderer.DrawRect(tabs[i], shellBorder, 1.0f);

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
                    tabFg,
                    ScaleValue(shellFs),
                    tabTextOptions);
            }
        }

        const Rect content = ContentRect();
        if (content.Width() > 0.5f && content.Height() > 0.5f) {
            renderer.FillRect(content, contentBackground);
            renderer.DrawRect(content, shellBorder, 1.0f);

            if (UIElement* child = SelectedChild()) {
                child->Render(renderer);
            }
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
            LayoutTabPages();
            return true;
        }
        const int clamped = std::clamp(index, 0, count - 1);
        if (clamped == m_selectedIndex) {
            return false;
        }
        m_selectedIndex = clamped;
        LayoutTabPages();
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
    float headerHeight = 30.0f;

private:
    void SyncLegacyColorsFromStyle() {
        if (m_style.base.decoration.has_value()) {
            background = m_style.base.decoration->background;
            headerBackground = m_style.base.decoration->background;
            contentBackground = m_style.base.decoration->background;
            borderColor = m_style.base.decoration->border.color;
            if (!m_style.base.decoration->radius.IsZero()) {
                cornerRadius = m_style.base.decoration->radius.MaxRadius();
            }
        }
        if (m_style.base.foreground.has_value()) {
            tabTextColor = *m_style.base.foreground;
        }
        if (m_style.base.fontSize.has_value()) {
            fontSize = *m_style.base.fontSize;
        }
        if (m_style.tabStyle) {
            const StyleSpec normal = m_style.tabStyle->Resolve(StyleState::Normal);
            const StyleSpec hover = m_style.tabStyle->Resolve(StyleState::Hover);
            const StyleSpec selected = m_style.tabStyle->Resolve(StyleState::Selected);
            if (normal.decoration.has_value()) {
                tabBackground = normal.decoration->background;
            }
            if (hover.decoration.has_value()) {
                hoverTabBackground = hover.decoration->background;
            }
            if (selected.decoration.has_value()) {
                selectedTabBackground = selected.decoration->background;
            }
            if (selected.foreground.has_value()) {
                selectedTabTextColor = *selected.foreground;
            }
            if (normal.foreground.has_value()) {
                tabTextColor = *normal.foreground;
            }
        }
    }

    // Place selected page into content rect; park others off-screen.
    void LayoutTabPages() {
        if (m_bounds.Width() <= 0.0f || m_bounds.Height() <= 0.0f) {
            return;
        }

        const Rect content = ContentRect();
        const Rect hidden = Rect::Make(-10000.0f, -10000.0f, -9990.0f, -9990.0f);
        NormalizeSelectedIndex();
        for (size_t i = 0; i < m_children.size(); ++i) {
            if (static_cast<int>(i) == m_selectedIndex && content.Width() > 0.5f && content.Height() > 0.5f) {
                m_children[i]->Measure(content.Width(), content.Height());
                m_children[i]->Arrange(content);
            } else {
                m_children[i]->Arrange(hidden);
            }
        }
    }

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
        if (m_bounds.Height() <= 0.0f || m_bounds.Width() <= 0.0f) {
            return Rect{};
        }
        const float height = std::min(m_bounds.Height(), ScaleValue(std::max(1.0f, headerHeight)));
        return Rect::Make(m_bounds.left, m_bounds.top, m_bounds.right, m_bounds.top + height);
    }

    Rect ContentRect() const {
        if (m_bounds.Height() <= 0.0f || m_bounds.Width() <= 0.0f) {
            return Rect{};
        }
        const Rect header = HeaderRect();
        const float inset = ScaleValue(1.0f);
        const float top = std::min(m_bounds.bottom, header.bottom + inset);
        if (top >= m_bounds.bottom - inset) {
            // Degenerate: not enough height for a body - still reserve a thin content strip.
            return Rect::Make(
                m_bounds.left + inset,
                std::max(m_bounds.top, m_bounds.bottom - ScaleValue(8.0f)),
                m_bounds.right - inset,
                m_bounds.bottom - inset);
        }
        return Rect::Make(
            m_bounds.left + inset,
            top,
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

// Shared pill-end scrollbar painter (track + thumb, radius = half thickness).
inline void PaintPillScrollbars(
    IRenderer& renderer,
    const Rect& client,
    float contentW,
    float contentH,
    float viewW,
    float viewH,
    float scrollX,
    float scrollY,
    Color trackColor,
    Color thumbColor,
    float thicknessPx) {
    const float thick = std::max(4.0f, thicknessPx);
    const float inset = std::max(1.0f, thick * 0.25f);
    const float radius = thick * 0.5f;
    const bool needV = contentH > viewH + 0.5f;
    const bool needH = contentW > viewW + 0.5f;
    if (!needV && !needH) {
        return;
    }

    const float cornerReserve = (needV && needH) ? (thick + inset) : 0.0f;

    if (needV) {
        const float trackLeft = client.right - inset - thick;
        const float trackTop = client.top + inset;
        const float trackBottom = client.bottom - inset - cornerReserve;
        const float trackH = std::max(thick, trackBottom - trackTop);
        const Rect track = Rect::Make(trackLeft, trackTop, trackLeft + thick, trackTop + trackH);
        renderer.FillRoundedRect(track, trackColor, radius);

        const float thumbH = std::clamp(viewH / std::max(1.0f, contentH) * trackH, thick * 1.6f, trackH);
        const float maxScroll = std::max(0.0f, contentH - viewH);
        const float t = maxScroll > 0.0f ? std::clamp(scrollY / maxScroll, 0.0f, 1.0f) : 0.0f;
        const float thumbTop = trackTop + t * (trackH - thumbH);
        renderer.FillRoundedRect(
            Rect::Make(trackLeft, thumbTop, trackLeft + thick, thumbTop + thumbH),
            thumbColor,
            radius);
    }

    if (needH) {
        const float trackTop = client.bottom - inset - thick;
        const float trackLeft = client.left + inset;
        const float trackRight = client.right - inset - cornerReserve;
        const float trackW = std::max(thick, trackRight - trackLeft);
        const Rect track = Rect::Make(trackLeft, trackTop, trackLeft + trackW, trackTop + thick);
        renderer.FillRoundedRect(track, trackColor, radius);

        const float thumbW = std::clamp(viewW / std::max(1.0f, contentW) * trackW, thick * 1.6f, trackW);
        const float maxScroll = std::max(0.0f, contentW - viewW);
        const float t = maxScroll > 0.0f ? std::clamp(scrollX / maxScroll, 0.0f, 1.0f) : 0.0f;
        const float thumbLeft = trackLeft + t * (trackW - thumbW);
        renderer.FillRoundedRect(
            Rect::Make(thumbLeft, trackTop, thumbLeft + thumbW, trackTop + thick),
            thumbColor,
            radius);
    }
}

// Clip viewport + offset content. Wheel + drag-pan + scrollbar thumb drag.
