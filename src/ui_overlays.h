#pragma once
// Popup, Modal
// See ui.h for the full umbrella include.
#include "ui_lists.h"

class Popup : public UIElement {
public:
    enum class Placement : uint8_t { Below = 0, Above = 1 };

    Popup() { m_style = DefaultStyle(nullptr); }

    static ComponentStyle DefaultStyle(const Theme* theme = nullptr) {
        ComponentStyle s;
        BoxDecoration shell;
        shell.background = ComponentStyle::ThemeColor(theme, "surface-2", ColorFromHex(0x1B2C3D));
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
        drop.radius = CornerRadius::Uniform(ComponentStyle::ThemeNumber(theme, Theme::NumberCategory::Radius, "md", 8.0f));
        dropdown->base.decoration = drop;
        s.dropdownStyle = std::move(dropdown);
        return s;
    }

    void ApplyThemeDefaults() override {
        if (m_styleFromLayout) {
            return;
        }
        const Theme* theme = (m_context && m_context->theme) ? m_context->theme : nullptr;
        m_style = DefaultStyle(theme);
        SyncLegacyFromStyle();
        for (auto& child : m_children) {
            if (child) {
                child->ApplyThemeDefaults();
            }
        }
    }

    void SetTriggerText(std::wstring text) { m_triggerText = std::move(text); }
    void SetPopupWidth(float w) { m_popupWidth = std::max(40.0f, w); }
    void SetPopupHeight(float h) { m_popupHeight = std::max(40.0f, h); }
    void SetPlacement(Placement p) { m_placement = p; }
    void SetOpen(bool open) {
        m_open = open;
        if (m_open) {
            LayoutPopupChildren();
        }
    }
    bool IsOpen() const { return m_open; }
    void ToggleOpen() { SetOpen(!m_open); }

    bool IsFocusable() const override { return true; }

    bool HitTest(float x, float y) const override {
        if (UIElement::HitTest(x, y)) {
            return true;
        }
        return m_open && PopupRect().Contains(x, y);
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
        if (m_open) {
            m_open = false;
            changed = true;
        }
        return changed;
    }

    bool OnKeyDown(WPARAM keyCode, LPARAM /*lParam*/) override {
        if (keyCode == VK_ESCAPE && m_open) {
            m_open = false;
            return true;
        }
        if (keyCode == VK_RETURN || keyCode == VK_SPACE) {
            ToggleOpen();
            return true;
        }
        return false;
    }

    bool OnMouseDown(float x, float y) override {
        if (TriggerRect().Contains(x, y)) {
            ToggleOpen();
            return true;
        }
        if (m_open && PopupRect().Contains(x, y)) {
            for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
                if (*it && (*it)->HitTest(x, y) && (*it)->OnMouseDown(x, y)) {
                    return true;
                }
            }
            return true;
        }
        return false;
    }

    bool OnMouseMove(float x, float y) override {
        bool changed = false;
        const bool overTrigger = TriggerRect().Contains(x, y);
        if (overTrigger != m_hovered) {
            m_hovered = overTrigger;
            changed = true;
        }
        if (m_open && PopupRect().Contains(x, y)) {
            for (auto& child : m_children) {
                if (child && child->OnMouseMove(x, y)) {
                    changed = true;
                }
            }
        }
        return changed;
    }

    void Measure(float availableWidth, float /*availableHeight*/) override {
        float w = HasFixedWidth() ? ScaleValue(m_fixedWidth) : ScaleValue(140.0f);
        float h = HasFixedHeight() ? ScaleValue(m_fixedHeight) : ScaleValue(m_triggerHeight);
        w = ClampWidth(std::min(w, std::max(0.0f, availableWidth)));
        h = ClampHeight(h);
        m_desiredSize = Size{w, h};
    }

    void Arrange(const Rect& finalRect) override {
        m_bounds = finalRect;
        if (m_open) {
            LayoutPopupChildren();
        }
    }

    void Render(IRenderer& renderer) override {
        const Rect trigger = TriggerRect();
        Color bg = background;
        Color fg = textColor;
        Color border = borderColor;
        if (m_style.base.decoration.has_value()) {
            bg = m_style.base.decoration->background;
            border = m_style.base.decoration->border.color;
        }
        if (m_style.base.foreground.has_value()) {
            fg = *m_style.base.foreground;
        }
        if (m_hovered || m_open) {
            bg = hoverBackground;
        }
        renderer.FillRoundedRect(trigger, bg, ScaleValue(cornerRadius));
        renderer.DrawRoundedRect(trigger, border, 1.0f, ScaleValue(cornerRadius));

        const std::wstring label = m_triggerText.empty() ? L"Popup" : m_triggerText;
        const Rect textRect = Rect::Make(
            trigger.left + ScaleValue(10.0f),
            trigger.top,
            trigger.right - ScaleValue(10.0f),
            trigger.bottom);
        const TextRenderOptions opts{
            TextWrapMode::NoWrap,
            TextHorizontalAlign::Center,
            TextVerticalAlign::Center
        };
        renderer.DrawTextW(label.c_str(), static_cast<UINT32>(label.size()), textRect, fg, ScaleValue(fontSize), opts);
        // Children only in overlay when open.
    }

    void RenderOverlay(IRenderer& renderer) override {
        // Nested overlays first (children may own their own).
        for (auto& child : m_children) {
            if (child) {
                child->RenderOverlay(renderer);
            }
        }
        if (!m_open) {
            return;
        }

        const Rect popup = PopupRect();
        Color bg = popupBackground;
        Color border = borderColor;
        float radius = cornerRadius;
        if (m_style.dropdownStyle && m_style.dropdownStyle->base.decoration.has_value()) {
            bg = m_style.dropdownStyle->base.decoration->background;
            border = m_style.dropdownStyle->base.decoration->border.color;
            if (!m_style.dropdownStyle->base.decoration->radius.IsZero()) {
                radius = m_style.dropdownStyle->base.decoration->radius.MaxRadius();
            }
        }
        renderer.FillRoundedRect(popup, bg, ScaleValue(radius));
        renderer.DrawRoundedRect(popup, border, 1.0f, ScaleValue(radius));

        for (auto& child : m_children) {
            if (child) {
                child->Render(renderer);
            }
        }
    }

    UIElement* FindOverlayHitAt(float x, float y) override {
        if (m_open) {
            if (PopupRect().Contains(x, y)) {
                for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
                    if (!*it) {
                        continue;
                    }
                    if (UIElement* nested = (*it)->FindOverlayHitAt(x, y)) {
                        return nested;
                    }
                    if ((*it)->HitTest(x, y)) {
                        if (UIElement* focusable = (*it)->FindFocusableAt(x, y)) {
                            return focusable;
                        }
                        return it->get();
                    }
                }
                return this;
            }
            if (TriggerRect().Contains(x, y)) {
                return this;
            }
        }
        return UIElement::FindOverlayHitAt(x, y);
    }

    bool DismissOverlaysAt(float x, float y) override {
        bool changed = UIElement::DismissOverlaysAt(x, y);
        if (m_open && !TriggerRect().Contains(x, y) && !PopupRect().Contains(x, y)) {
            m_open = false;
            changed = true;
        }
        return changed;
    }

    bool DismissAllOverlays() override {
        bool changed = UIElement::DismissAllOverlays();
        if (m_open) {
            m_open = false;
            changed = true;
        }
        return changed;
    }

    Color background = ColorFromHex(0x1B2C3D);
    Color hoverBackground = ColorFromHex(0x24384C);
    Color popupBackground = ColorFromHex(0x112130);
    Color borderColor = ColorFromHex(0x36506A);
    Color textColor = ColorFromHex(0xE9F2FD);
    float cornerRadius = 8.0f;
    float fontSize = 13.0f;
    float m_triggerHeight = 36.0f;

private:
    void SyncLegacyFromStyle() {
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
        if (m_style.dropdownStyle && m_style.dropdownStyle->base.decoration.has_value()) {
            popupBackground = m_style.dropdownStyle->base.decoration->background;
        }
        const StyleSpec hover = m_style.Resolve(StyleState::Hover);
        if (hover.decoration.has_value()) {
            hoverBackground = hover.decoration->background;
        }
    }

    Rect TriggerRect() const { return m_bounds; }

    Rect PopupRect() const {
        const float w = ScaleValue(m_popupWidth);
        const float h = ScaleValue(m_popupHeight);
        const float margin = ScaleValue(4.0f);
        const float viewportW = (m_context && m_context->viewportWidth > 1.0f)
            ? m_context->viewportWidth
            : m_bounds.right + w;
        const float viewportH = (m_context && m_context->viewportHeight > 1.0f)
            ? m_context->viewportHeight
            : 0.0f;
        // C4: same vertical placement helper as ComboBox dropdown.
        Rect placed = OverlayFlyout::PlaceVertical(
            m_bounds,
            h,
            viewportH,
            margin,
            m_placement == Placement::Above);
        float left = placed.left;
        if (left + w > viewportW - margin) {
            left = std::max(margin, viewportW - w - margin);
        }
        return Rect::Make(left, placed.top, left + w, placed.top + placed.Height());
    }

    void LayoutPopupChildren() {
        const Rect popup = PopupRect();
        const float pad = ScaleValue(12.0f);
        const float spacing = ScaleValue(8.0f);
        float y = popup.top + pad;
        const float contentLeft = popup.left + pad;
        const float contentRight = popup.right - pad;
        const float contentW = std::max(1.0f, contentRight - contentLeft);
        for (auto& child : m_children) {
            if (!child) {
                continue;
            }
            child->SetContext(m_context);
            child->Measure(contentW, 4096.0f);
            const float ch = std::max(1.0f, child->DesiredSize().height);
            child->Arrange(Rect::Make(contentLeft, y, contentRight, y + ch));
            y += ch + spacing;
        }
    }

    std::wstring m_triggerText = L"Popup";
    float m_popupWidth = 240.0f;
    float m_popupHeight = 140.0f;
    Placement m_placement = Placement::Below;
    bool m_open = false;
};

// Modal Dialog (Wave2 C7): Blocks input to underlying tree, dim overlay.
class Modal : public UIElement {
public:
    Modal() { m_style = DefaultStyle(nullptr); }

    static ComponentStyle DefaultStyle(const Theme* theme = nullptr) {
        ComponentStyle s;
        BoxDecoration shell;
        shell.background = ComponentStyle::ThemeColor(theme, "surface-1", ColorFromHex(0x1B2C3D));
        shell.border.width = Thickness{1, 1, 1, 1};
        shell.border.color = ComponentStyle::ThemeColor(theme, "border-subtle", ColorFromHex(0x36506A));
        shell.radius = CornerRadius::Uniform(ComponentStyle::ThemeNumber(theme, Theme::NumberCategory::Radius, "md", 8.0f));
        s.base.decoration = shell;
        s.base.foreground = ComponentStyle::ThemeColor(theme, "fg", ColorFromHex(0xE9F2FD));
        s.base.fontSize = ComponentStyle::ThemeNumber(theme, Theme::NumberCategory::FontSize, "sm", 13.0f);
        return s;
    }

    void ApplyThemeDefaults() override {
        if (m_styleFromLayout) return;
        const Theme* theme = (m_context && m_context->theme) ? m_context->theme : nullptr;
        m_style = DefaultStyle(theme);
        for (auto& child : m_children) {
            if (child) child->ApplyThemeDefaults();
        }
    }

    void SetModalWidth(float w) { m_modalWidth = std::max(100.0f, w); }
    void SetModalHeight(float h) { m_modalHeight = std::max(40.0f, h); }
    
    void SetOpen(bool open) {
        m_open = open;
        if (!m_open) {
            m_dragging = false;
            m_offsetX = 0.0f;
            m_offsetY = 0.0f;
        }
        if (m_open) {
            LayoutModalChildren();
        }
    }
    bool IsOpen() const { return m_open; }
    void Open() { SetOpen(true); }
    void Close() { SetOpen(false); }

    bool IsFocusable() const override { return true; }

    UIElement* FindOverlayHitAt(float x, float y) override {
        if (!m_open) {
            return UIElement::FindOverlayHitAt(x, y);
        }
        if (ModalRect().Contains(x, y)) {
            // If the pointer is on a caption area, return 'this' so the host
            // sets m_mouseCaptureTarget = Modal, and drag events flow here.
            for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
                if (*it && (*it)->HasCaptionHitAt(x, y)) {
                    return this;
                }
            }
            // Otherwise return the deepest interactive child.
            for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
                if (*it && (*it)->HitTest(x, y)) {
                    if (UIElement* hit = (*it)->FindOverlayHitAt(x, y)) {
                        return hit;
                    }
                    if (UIElement* hit = (*it)->FindHitElementAt(x, y)) {
                        return hit;
                    }
                }
            }
            // Background of modal itself
            return this;
        }
        // Block hits to anything else by returning this modal for the dim region
        return this;
    }

    bool DismissAllOverlays() override {
        bool changed = UIElement::DismissAllOverlays();
        if (m_open) {
            m_open = false;
            changed = true;
        }
        return changed;
    }

    bool OnMouseDown(float x, float y) override {
        if (ModalRect().Contains(x, y)) {
            bool handled = false;
            for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
                if (*it && (*it)->HitTest(x, y)) {
                    if ((*it)->OnMouseDown(x, y)) {
                        handled = true;
                        break;
                    }
                }
            }
            if (handled) return true;

            // Check if we hit a caption area
            for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
                if (*it && (*it)->HasCaptionHitAt(x, y)) {
                    m_dragging = true;
                    m_dragStartX = x;
                    m_dragStartY = y;
                    m_dragStartOffsetX = m_offsetX;
                    m_dragStartOffsetY = m_offsetY;
                    return true;
                }
            }
            return true;
        }
        // Outside the modal. Block input.
        return true; 
    }

    bool OnMouseMove(float x, float y) override {
        if (m_dragging) {
            m_offsetX = m_dragStartOffsetX + (x - m_dragStartX);
            m_offsetY = m_dragStartOffsetY + (y - m_dragStartY);
            LayoutModalChildren();
            return true;
        }
        bool changed = false;
        if (m_open && ModalRect().Contains(x, y)) {
            for (auto& child : m_children) {
                if (child && child->OnMouseMove(x, y)) {
                    changed = true;
                }
            }
        }
        return changed;
    }

    bool OnMouseUp(float x, float y) override {
        if (m_dragging) {
            m_dragging = false;
            return true;
        }
        bool changed = false;
        if (m_open && ModalRect().Contains(x, y)) {
            for (auto& child : m_children) {
                if (child && child->OnMouseUp(x, y)) {
                    changed = true;
                }
            }
        }
        return changed;
    }

    void Measure(float /*availableWidth*/, float /*availableHeight*/) override {
        // Modal is an overlay placeholder. It doesn't take space in flow.
        m_desiredSize = Size{0.0f, 0.0f};
    }

    void Arrange(const Rect& finalRect) override {
        m_bounds = finalRect;
        if (m_open) {
            LayoutModalChildren();
        }
    }

    void Render(IRenderer& /*renderer*/) override {
        // Normal render pass does nothing. Modal draws in RenderOverlay.
    }

    void RenderOverlay(IRenderer& renderer) override {
        if (!m_open || !m_context) {
            return;
        }

        // Draw dim layer covering the entire viewport
        Rect dimRect = Rect::Make(0.0f, 0.0f, m_context->viewportWidth, m_context->viewportHeight);
        renderer.FillRect(dimRect, Color{0.0f, 0.0f, 0.0f, 0.5f});

        const Rect modal = ModalRect();
        Color bg = ColorFromHex(0x1B2C3D);
        Color border = ColorFromHex(0x36506A);
        if (m_style.base.decoration.has_value()) {
            bg = m_style.base.decoration->background;
            border = m_style.base.decoration->border.color;
        }
        float radius = 8.0f;
        if (m_style.base.decoration && m_style.base.decoration->radius.topLeft > 0) {
            radius = m_style.base.decoration->radius.topLeft;
        }

        // Drop shadow for modal
        renderer.FillSoftShadow(modal, radius, 0.0f, 6.0f, 12.0f, Color{0.0f, 0.0f, 0.0f, 0.3f});

        renderer.FillRoundedRect(modal, bg, ScaleValue(radius));
        renderer.DrawRoundedRect(modal, border, 1.0f, ScaleValue(radius));

        renderer.PushRoundedClip(modal, ScaleValue(radius));
        // Draw children
        for (auto& child : m_children) {
            if (child) {
                child->Render(renderer);
            }
        }
        renderer.PopLayer();

        UIElement::RenderOverlay(renderer);
    }

private:
    Rect ModalRect() const {
        if (!m_context) return Rect::Make(0,0,0,0);
        float vw = m_context->viewportWidth;
        float vh = m_context->viewportHeight;
        float w = ScaleValue(m_modalWidth);
        float h = ScaleValue(m_modalHeight);
        float left = (vw - w) * 0.5f + m_offsetX;
        float top = (vh - h) * 0.5f + m_offsetY;
        return Rect::Make(left, top, left + w, top + h);
    }

    void LayoutModalChildren() {
        if (!m_context) return;
        const Rect modal = ModalRect();
        const float w = std::max(1.0f, modal.right - modal.left);
        const float h = std::max(1.0f, modal.bottom - modal.top);

        for (auto& child : m_children) {
            if (!child) continue;
            child->SetContext(m_context);
            child->Measure(w, h);
            child->Arrange(modal);
        }
    }

    float m_modalWidth = 320.0f;
    float m_modalHeight = 200.0f;
    bool m_open = false;

    float m_offsetX = 0.0f;
    float m_offsetY = 0.0f;
    bool m_dragging = false;
    float m_dragStartX = 0.0f;
    float m_dragStartY = 0.0f;
    float m_dragStartOffsetX = 0.0f;
    float m_dragStartOffsetY = 0.0f;
};


