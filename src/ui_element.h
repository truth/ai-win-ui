#pragma once

#include "box_decoration.h"
#include "graphics_types.h"
#include "layout_engine.h"
#include "renderer.h"
#include "style.h"
#include "style_catalog.h"
#include "theme.h"
#include "ui_context.h"
#include "ui_text_metrics.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cwctype>
#include <functional>
#include <limits>
#include <memory>
#include <map>
#include <set>
#include <string>
#include <utility>
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

    // Window hit-test role for custom chrome (WM_NCHITTEST bridging).
    enum class HitTestRole : uint8_t {
        Default = 0,
        Caption = 1, // draggable title-bar region
        Client = 2,  // force client hit (buttons, inputs)
    };

    // Optional root-level preference; Unspecified keeps App / env defaults.
    enum class WindowChromeRequest : uint8_t {
        Unspecified = 0,
        System = 1,
        Custom = 2,
        Layered = 3,
    };

    virtual ~UIElement() = default;

    // Optional stable id for measure dumps / automation (layout name= / id=).
    void SetName(std::string name) { m_name = std::move(name); }
    const std::string& Name() const { return m_name; }

    // Depth-first name lookup (Wave1 host events: togglePopup / applyTheme).
    UIElement* FindElementByName(const std::string& name) {
        if (!name.empty() && m_name == name) {
            return this;
        }
        for (auto& child : m_children) {
            if (!child) {
                continue;
            }
            if (UIElement* hit = child->FindElementByName(name)) {
                return hit;
            }
        }
        return nullptr;
    }

    void SetBounds(const Rect& rect) { m_bounds = rect; }
    Rect Bounds() const { return m_bounds; }
    void SetHitTestRole(HitTestRole role) { m_hitTestRole = role; }
    HitTestRole GetHitTestRole() const { return m_hitTestRole; }
    void SetWindowChromeRequest(WindowChromeRequest request) { m_windowChromeRequest = request; }
    WindowChromeRequest GetWindowChromeRequest() const { return m_windowChromeRequest; }
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
        ApplyThemeDefaults();
        for (auto& child : m_children) {
            child->SetContext(context);
        }
    }
    UIContext* Context() const { return m_context; }

    // Lazy theme: refresh DefaultStyle() from theme when style was not set from layout.
    virtual void ApplyThemeDefaults() {}
    void SetStyle(ComponentStyle style) {
        m_style = std::move(style);
        m_styleFromLayout = true;
        m_styleCatalogName.clear();
    }
    // Catalog-named style so S4 can re-resolve after theme reload.
    void SetStyleFromCatalog(std::string catalogName, ComponentStyle style) {
        m_styleCatalogName = std::move(catalogName);
        m_style = std::move(style);
        m_styleFromLayout = true;
    }
    const std::string& StyleCatalogName() const { return m_styleCatalogName; }
    void RebindCatalogStyle() {
        if (m_styleCatalogName.empty() || !m_context || !m_context->styleCatalog) {
            return;
        }
        if (auto s = m_context->styleCatalog->Resolve(m_styleCatalogName)) {
            m_style = *s;
            m_styleFromLayout = true;
        }
    }
    const ComponentStyle& Style() const { return m_style; }
    ComponentStyle& MutableStyle() { return m_style; }

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

    // Second paint pass for popups (ComboBox dropdown, menus) so they draw above siblings.
    virtual void RenderOverlay(IRenderer& renderer) {
        for (auto& child : m_children) {
            child->RenderOverlay(renderer);
        }
    }

    // Prefer overlay hit targets (expanded dropdowns) over normal tree order.
    virtual UIElement* FindOverlayHitAt(float x, float y) {
        for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
            if (UIElement* hit = (*it)->FindOverlayHitAt(x, y)) {
                return hit;
            }
        }
        return nullptr;
    }

    // Close popups that do not contain (x,y). Used for click-outside dismiss (C2).
    virtual bool DismissOverlaysAt(float x, float y) {
        bool changed = false;
        for (auto& child : m_children) {
            changed = child->DismissOverlaysAt(x, y) || changed;
        }
        return changed;
    }

    // Close every open overlay regardless of pointer (Esc / host-level cancel).
    // Default: walk children. Overlay owners (ComboBox, future Popup) override.
    virtual bool DismissAllOverlays() {
        bool changed = false;
        for (auto& child : m_children) {
            changed = child->DismissAllOverlays() || changed;
        }
        return changed;
    }

    virtual bool IsFocusable() const { return false; }
    bool HasFocus() const { return m_hasFocus; }
    bool IsHovered() const { return m_hovered; }
    bool IsPressed() const { return m_pressed; }
    bool IsDisabled() const { return m_disabled; }
    void SetEnabled(bool enabled) { m_disabled = !enabled; }

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

    virtual bool GetTextInputCaretRect(Rect& /*out*/) const {
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
        if (!HitTest(x, y)) {
            return nullptr;
        }
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

    virtual bool HasCaptionHitAt(float x, float y) {
        if (!HitTest(x, y)) return false;
        if (m_hitTestRole == HitTestRole::Caption) return true;
        for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
            if (*it && (*it)->HasCaptionHitAt(x, y)) {
                return true;
            }
        }
        return false;
    }

    virtual void CollectFocusable(std::vector<UIElement*>& out) {
        if (IsFocusable()) {
            out.push_back(this);
        }
        for (auto& child : m_children) {
            child->CollectFocusable(out);
        }
    }

    // Collect caption / client-only rectangles for custom window chrome hit testing.
    // clientOnly is recorded for explicit Client role or focusable controls.
    void CollectChromeHitRegions(
        std::vector<std::pair<Rect, HitTestRole>>& captionRegions,
        std::vector<Rect>& clientOnlyRegions) const {
        const HitTestRole role = m_hitTestRole;
        if (role == HitTestRole::Caption) {
            captionRegions.emplace_back(m_bounds, role);
        }
        if (role == HitTestRole::Client || IsFocusable()) {
            clientOnlyRegions.push_back(m_bounds);
        }
        for (const auto& child : m_children) {
            child->CollectChromeHitRegions(captionRegions, clientOnlyRegions);
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

    // delta: wheel notches (positive = scroll content up / view down). shiftHeld: prefer horizontal.
    virtual bool OnMouseWheel(float delta, float x, float y, bool shiftHeld) {
        for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
            if ((*it)->HitTest(x, y) && (*it)->OnMouseWheel(delta, x, y, shiftHeld)) {
                return true;
            }
        }
        return false;
    }

protected:
    virtual float MeasurePreferredWidth(float availableWidth) const {
        return availableWidth;
    }

    virtual float MeasurePreferredHeight(float width) const {
        return 36.0f;
    }

public:
    virtual bool HitTest(float x, float y) const {
        return x >= m_bounds.left && x <= m_bounds.right && y >= m_bounds.top && y <= m_bounds.bottom;
    }

    const std::vector<std::unique_ptr<UIElement>>& Children() const { return m_children; }

    void AddChild(std::unique_ptr<UIElement> child) {
        if (child) {
            child->SetContext(m_context);
        }
        m_children.emplace_back(std::move(child));
    }
    std::vector<std::unique_ptr<UIElement>>& Children() { return m_children; }
    // Composite controls that own children for content (TabControl, etc.) should
    // override this to return true so Yoga measures them as opaque leaves.
    virtual bool IsLayoutLeaf() const { return m_children.empty(); }

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

    // Tree & Identity
    std::string                             m_name;
    std::vector<std::unique_ptr<UIElement>> m_children;
    UIContext*                              m_context = nullptr;

    // Layout Core
    Rect                                    m_bounds{};
    Size                                    m_desiredSize{};
    Thickness                               m_margin{};
    Thickness                               m_border{};

    // Sizing Constraints
    float                                   m_fixedWidth  = -1.0f;
    float                                   m_fixedHeight = -1.0f;
    float                                   m_minWidth    = -1.0f;
    float                                   m_maxWidth    = -1.0f;
    float                                   m_minHeight   = -1.0f;
    float                                   m_maxHeight   = -1.0f;

    // Flexbox Config
    float                                   m_flexGrow   = 0.0f;
    float                                   m_flexShrink = 0.0f;
    float                                   m_flexBasis  = -1.0f;
    SelfAlign                               m_alignSelf  = SelfAlign::Auto;

    // State & Interaction
    bool                                    m_hasFocus = false;
    bool                                    m_hovered  = false;
    bool                                    m_pressed  = false;
    bool                                    m_disabled = false;

    // Window & Chrome Integration
    HitTestRole                             m_hitTestRole = HitTestRole::Default;
    WindowChromeRequest                     m_windowChromeRequest = WindowChromeRequest::Unspecified;

    // Styling
    ComponentStyle                          m_style{};
    bool                                    m_styleFromLayout = false;
    std::string                             m_styleCatalogName;
};

