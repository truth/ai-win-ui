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

    Rect m_bounds{};
    Size m_desiredSize{};
    std::vector<std::unique_ptr<UIElement>> m_children;
    std::string m_name;
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
    HitTestRole m_hitTestRole = HitTestRole::Default;
    WindowChromeRequest m_windowChromeRequest = WindowChromeRequest::Unspecified;
    ComponentStyle m_style{};
    bool m_styleFromLayout = false;
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
        Color drawBackground = background;
        Color drawBorderColor = borderColor;
        float drawCornerRadius = cornerRadius;
        Thickness drawBorder = Border();

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
            }
        }

        const Thickness scaledBorder = ScaleThickness(drawBorder);
        const bool hasBorder =
            scaledBorder.left > 0.0f || scaledBorder.top > 0.0f ||
            scaledBorder.right > 0.0f || scaledBorder.bottom > 0.0f;
        const float scaledRadius = ScaleValue(drawCornerRadius);
        if (drawBackground.a > 0.0f || hasBorder) {
            BoxDecoration deco;
            deco.background = drawBackground;
            deco.border.width = scaledBorder;
            deco.border.color = drawBorderColor;
            deco.radius = CornerRadius::Uniform(scaledRadius);
            DrawBoxDecoration(renderer, m_bounds, deco);
        }
        // Do not clip children to the panel radius. Overlay controls (e.g. ComboBox
        // dropdown) must be able to paint outside their layout bounds. Background is
        // already rounded via DrawBoxDecoration / FillRoundedRect above.
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

    void SetSvgData(std::vector<uint8_t> svgData) {
        m_svgData = std::move(svgData);
        m_svg.reset();
    }

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
            m_svg = renderer.CreateSvgFromBytes(m_svgData.data(), m_svgData.size());
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
        renderer.DrawSvg(m_svg, drawRect);
    }

private:
    std::wstring m_source;
    std::vector<uint8_t> m_svgData;
    SvgHandle m_svg = nullptr;
    Size m_intrinsicSize{24.0f, 24.0f};
};

class Button : public UIElement {
public:
    // Default: filled action button. Caption*: window chrome controls with
    // geometric glyphs (min / max / close) and Fluent-like hover washes.
    enum class Variant {
        Default,
        CaptionMinimize,
        CaptionMaximize,
        CaptionClose,
    };

    explicit Button(std::wstring text) : m_text(std::move(text)) {
        m_style = DefaultStyle(nullptr);
    }

    void SetOnClick(std::function<void()> onClick) { m_onClick = std::move(onClick); }
    void SetText(std::wstring text) { m_text = std::move(text); }
    void SetVariant(Variant variant) {
        m_variant = variant;
        const Theme* theme = (m_context && m_context->theme) ? m_context->theme : nullptr;
        switch (variant) {
            case Variant::CaptionMinimize:
            case Variant::CaptionMaximize:
                m_style = CaptionStyle();
                m_styleFromLayout = true; // caption styles are fixed, not layout/theme
                cornerRadius = 0.0f;
                background = Color{0, 0, 0, 0};
                foreground = ColorFromHex(0xD7E2EE);
                break;
            case Variant::CaptionClose:
                m_style = CaptionCloseStyle();
                m_styleFromLayout = true;
                cornerRadius = 0.0f;
                background = Color{0, 0, 0, 0};
                foreground = ColorFromHex(0xD7E2EE);
                break;
            case Variant::Default:
            default:
                m_styleFromLayout = false;
                m_style = DefaultStyle(theme);
                break;
        }
    }
    Variant GetVariant() const { return m_variant; }

    // When true on CaptionMaximize, draw the dual-square restore glyph.
    void SetShowRestoreGlyph(bool show) { m_showRestoreGlyph = show; }
    bool ShowRestoreGlyph() const { return m_showRestoreGlyph; }

    float cornerRadius = 4.0f;
    float fontSize = 14.0f;
    Color background = ColorFromHex(0x2D2D30);
    Color foreground = ColorFromHex(0xFFFFFF);

    static ComponentStyle DefaultStyle(const Theme* theme = nullptr) {
        ComponentStyle s;
        BoxDecoration normalDeco;
        normalDeco.background = ComponentStyle::ThemeColor(theme, "btn-bg", ColorFromHex(0x2D2D30));
        normalDeco.border.width = Thickness{1.0f, 1.0f, 1.0f, 1.0f};
        normalDeco.border.color = ComponentStyle::ThemeColor(theme, "border", ColorFromHex(0x6A6A6A));
        normalDeco.radius = CornerRadius::Uniform(ComponentStyle::ThemeNumber(theme, Theme::NumberCategory::Radius, "sm", 4.0f));
        s.base.decoration = normalDeco;
        s.base.foreground = ComponentStyle::ThemeColor(theme, "fg-strong", ColorFromHex(0xFFFFFF));

        BoxDecoration hoverDeco = normalDeco;
        hoverDeco.background = ComponentStyle::ThemeColor(theme, "btn-bg-hover", ColorFromHex(0x3E3E42));
        s.overrides[static_cast<std::size_t>(StyleState::Hover)].decoration = hoverDeco;

        BoxDecoration pressedDeco = normalDeco;
        pressedDeco.background = ComponentStyle::ThemeColor(theme, "btn-bg-press", ColorFromHex(0x0E639C));
        s.overrides[static_cast<std::size_t>(StyleState::Pressed)].decoration = pressedDeco;

        BoxDecoration focusedDeco = normalDeco;
        focusedDeco.border.width = Thickness{2.0f, 2.0f, 2.0f, 2.0f};
        focusedDeco.border.color = ComponentStyle::ThemeColor(theme, "border-focus", ColorFromHex(0xFFFFFF));
        s.overrides[static_cast<std::size_t>(StyleState::Focused)].decoration = focusedDeco;

        BoxDecoration disabledDeco = normalDeco;
        disabledDeco.background = ComponentStyle::ThemeColor(theme, "surface-4", ColorFromHex(0x1F1F1F));
        disabledDeco.border.color = ComponentStyle::ThemeColor(theme, "border-subtle", ColorFromHex(0x3A3A3A));
        s.overrides[static_cast<std::size_t>(StyleState::Disabled)].decoration = disabledDeco;
        s.overrides[static_cast<std::size_t>(StyleState::Disabled)].opacity = 0.6f;

        return s;
    }

    void ApplyThemeDefaults() override {
        if (m_styleFromLayout || m_variant != Variant::Default) {
            return;
        }
        const Theme* theme = (m_context && m_context->theme) ? m_context->theme : nullptr;
        m_style = DefaultStyle(theme);
        if (m_style.base.decoration.has_value()) {
            background = m_style.base.decoration->background;
            if (!m_style.base.decoration->radius.IsZero()) {
                cornerRadius = m_style.base.decoration->radius.MaxRadius();
            }
        }
        if (m_style.base.foreground.has_value()) {
            foreground = *m_style.base.foreground;
        }
    }

    // Soft luminous wash - no border, flat against the title bar.
    static ComponentStyle CaptionStyle() {
        ComponentStyle s;
        BoxDecoration normal;
        normal.background = Color{0.0f, 0.0f, 0.0f, 0.0f};
        normal.border.width = Thickness{};
        normal.radius = CornerRadius::Uniform(0.0f);
        s.base.decoration = normal;
        s.base.foreground = ColorFromHex(0xD7E2EE);

        BoxDecoration hover = normal;
        hover.background = Color{1.0f, 1.0f, 1.0f, 0.08f};
        s.overrides[static_cast<std::size_t>(StyleState::Hover)].decoration = hover;
        s.overrides[static_cast<std::size_t>(StyleState::Hover)].foreground = ColorFromHex(0xF4F8FC);

        BoxDecoration pressed = normal;
        pressed.background = Color{1.0f, 1.0f, 1.0f, 0.14f};
        s.overrides[static_cast<std::size_t>(StyleState::Pressed)].decoration = pressed;
        s.overrides[static_cast<std::size_t>(StyleState::Pressed)].foreground = ColorFromHex(0xFFFFFF);

        BoxDecoration focused = normal;
        focused.background = Color{1.0f, 1.0f, 1.0f, 0.05f};
        s.overrides[static_cast<std::size_t>(StyleState::Focused)].decoration = focused;
        return s;
    }

    // Close: warm crimson hover (desktop OS convention), white glyph.
    static ComponentStyle CaptionCloseStyle() {
        ComponentStyle s = CaptionStyle();
        BoxDecoration hover;
        hover.background = ColorFromHex(0xE81123);
        hover.border.width = Thickness{};
        hover.radius = CornerRadius::Uniform(0.0f);
        s.overrides[static_cast<std::size_t>(StyleState::Hover)].decoration = hover;
        s.overrides[static_cast<std::size_t>(StyleState::Hover)].foreground = ColorFromHex(0xFFFFFF);

        BoxDecoration pressed;
        pressed.background = ColorFromHex(0xF1707A);
        pressed.border.width = Thickness{};
        pressed.radius = CornerRadius::Uniform(0.0f);
        s.overrides[static_cast<std::size_t>(StyleState::Pressed)].decoration = pressed;
        s.overrides[static_cast<std::size_t>(StyleState::Pressed)].foreground = ColorFromHex(0xFFFFFF);
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
        if (IsCaptionVariant()) {
            // Caption controls stay sharp-edged and borderless.
            deco.radius = CornerRadius::Uniform(0.0f);
            deco.border.width = Thickness{};
        } else {
            deco.radius = CornerRadius::Uniform(ScaleValue(cornerRadius));
            deco.border.width = Thickness{
                ScaleValue(deco.border.width.left),
                ScaleValue(deco.border.width.top),
                ScaleValue(deco.border.width.right),
                ScaleValue(deco.border.width.bottom)
            };
        }
        if (resolved.opacity.has_value()) {
            deco.opacity = *resolved.opacity;
        }
        DrawBoxDecoration(renderer, m_bounds, deco);

        Color fg = resolved.foreground.value_or(foreground);
        if (IsCaptionVariant()) {
            if (m_context && !m_context->windowActive) {
                // Dim caption glyphs when the window is not active.
                fg.a *= 0.45f;
            }
            DrawCaptionGlyph(renderer, fg);
            return;
        }

        // Center label properly (no left padding bias).
        const TextRenderOptions textOptions{
            TextWrapMode::NoWrap,
            TextHorizontalAlign::Center,
            TextVerticalAlign::Center
        };
        const float fs = resolved.fontSize.value_or(fontSize);
        renderer.DrawTextW(
            m_text.c_str(),
            static_cast<UINT32>(m_text.size()),
            m_bounds,
            fg,
            ScaleValue(fs),
            textOptions
        );
    }

private:
    bool IsCaptionVariant() const {
        return m_variant == Variant::CaptionMinimize
            || m_variant == Variant::CaptionMaximize
            || m_variant == Variant::CaptionClose;
    }

    void DrawCaptionGlyph(IRenderer& renderer, const Color& color) const {
        const float cx = (m_bounds.left + m_bounds.right) * 0.5f;
        const float cy = (m_bounds.top + m_bounds.bottom) * 0.5f;
        // Icon optical size ~10dip; stroke slightly thicker than hairline for clarity.
        const float half = ScaleValue(5.0f);
        const float stroke = std::max(1.0f, ScaleValue(1.15f));

        switch (m_variant) {
            case Variant::CaptionMinimize: {
                renderer.DrawLine(
                    PointF{cx - half, cy},
                    PointF{cx + half, cy},
                    color,
                    stroke);
                break;
            }
            case Variant::CaptionMaximize: {
                if (m_showRestoreGlyph) {
                    // Overlapping squares (restore): back offset up-right, front lower-left.
                    const float offset = ScaleValue(2.2f);
                    const float s = half * 0.78f;
                    const Rect back = Rect::Make(
                        cx - s + offset, cy - s - offset,
                        cx + s + offset, cy + s - offset);
                    const Rect front = Rect::Make(
                        cx - s - offset, cy - s + offset,
                        cx + s - offset, cy + s + offset);
                    renderer.DrawRect(back, color, stroke);
                    // Clear the overlapping edge of the back square by filling front bg is
                    // unavailable; draw front on top for a clean dual-window mark.
                    renderer.DrawRect(front, color, stroke);
                } else {
                    const Rect box = Rect::Make(cx - half, cy - half, cx + half, cy + half);
                    renderer.DrawRect(box, color, stroke);
                }
                break;
            }
            case Variant::CaptionClose: {
                // Slightly larger X for balance against the square glyph.
                const float xHalf = ScaleValue(4.6f);
                renderer.DrawLine(
                    PointF{cx - xHalf, cy - xHalf},
                    PointF{cx + xHalf, cy + xHalf},
                    color,
                    stroke);
                renderer.DrawLine(
                    PointF{cx + xHalf, cy - xHalf},
                    PointF{cx - xHalf, cy + xHalf},
                    color,
                    stroke);
                break;
            }
            default:
                break;
        }
    }

    void SyncQuickSetToStyle() {
        // Caption variants own their state palette; don't clobber with quick-set colors.
        if (IsCaptionVariant()) {
            return;
        }
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
    Variant m_variant = Variant::Default;
    bool m_showRestoreGlyph = false;
};

class TextInput : public UIElement {
public:
    explicit TextInput(std::wstring text = L"") : m_text(std::move(text)), m_caretPosition(m_text.size()) {
        m_style = DefaultStyle(nullptr);
    }

    static ComponentStyle DefaultStyle(const Theme* theme = nullptr) {
        ComponentStyle s;
        BoxDecoration normalDeco;
        normalDeco.background = ComponentStyle::ThemeColor(theme, "input-bg", ColorFromHex(0x1F1F1F));
        normalDeco.border.width = Thickness{1.0f, 1.0f, 1.0f, 1.0f};
        normalDeco.border.color = ComponentStyle::ThemeColor(theme, "border", ColorFromHex(0x6A6A6A));
        normalDeco.radius = CornerRadius::Uniform(ComponentStyle::ThemeNumber(theme, Theme::NumberCategory::Radius, "sm", 4.0f));
        s.base.decoration = normalDeco;
        s.base.foreground = ComponentStyle::ThemeColor(theme, "fg", ColorFromHex(0xEDEDED));

        BoxDecoration focusedDeco = normalDeco;
        focusedDeco.border.width = Thickness{2.0f, 2.0f, 2.0f, 2.0f};
        focusedDeco.border.color = ComponentStyle::ThemeColor(theme, "border-focus", ColorFromHex(0xFFFFFF));
        s.overrides[static_cast<std::size_t>(StyleState::Focused)].decoration = focusedDeco;

        BoxDecoration disabledDeco = normalDeco;
        disabledDeco.background = ComponentStyle::ThemeColor(theme, "surface-0", ColorFromHex(0x171717));
        disabledDeco.border.color = ComponentStyle::ThemeColor(theme, "border-subtle", ColorFromHex(0x3A3A3A));
        s.overrides[static_cast<std::size_t>(StyleState::Disabled)].decoration = disabledDeco;
        s.overrides[static_cast<std::size_t>(StyleState::Disabled)].opacity = 0.6f;

        return s;
    }

    void ApplyThemeDefaults() override {
        if (m_styleFromLayout) {
            return;
        }
        const Theme* theme = (m_context && m_context->theme) ? m_context->theme : nullptr;
        m_style = DefaultStyle(theme);
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
        if (ch == L'\b') {
            return true;
        }
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

    // Place caret at the glyph boundary nearest to localX (same NoWrap metrics as draw).
    size_t ComputeCaretIndex(float localX, const Rect& textRect) const {
        if (m_text.empty()) {
            return 0;
        }
        const float maxW = textRect.Width();
        size_t best = 0;
        float bestDist = std::abs(localX - 0.0f);
        for (size_t i = 1; i <= m_text.size(); ++i) {
            const float prefixWidth = MeasureNoWrapTextWidth(m_text.substr(0, i), maxW);
            const float dist = std::abs(localX - prefixWidth);
            if (dist < bestDist) {
                bestDist = dist;
                best = i;
            }
        }
        return best;
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

    bool GetTextInputCaretRect(Rect& out) const override {
        if (!m_hasFocus) {
            return false;
        }
        out = CaretRect();
        return true;
    }

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

        const Rect textRect = ContentRect();
        const Rect lineRect = TextLineRect(textRect);
        if (HasSelection()) {
            const auto [selStart, selEnd] = GetSelectionRange();
            std::wstring prefix = m_text.substr(0, selStart);
            std::wstring selection = m_text.substr(selStart, selEnd - selStart);
            const float prefixWidth = MeasureNoWrapTextWidth(prefix, textRect.Width());
            const Size selectionMetrics = MeasureTextValue(
                selection.empty() ? L" " : selection,
                fontSize,
                std::max(1.0f, textRect.Width()),
                TextWrapMode::NoWrap);
            Rect selectionRect = Rect::Make(
                lineRect.left + prefixWidth,
                lineRect.top,
                lineRect.left + prefixWidth + selectionMetrics.width,
                lineRect.bottom
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
            lineRect,
            fg,
            ScaleValue(fontSize),
            textOptions);

        if (m_hasFocus) {
            const Rect caretRect = CaretRect();
            if (m_showCaret) {
                renderer.DrawRect(caretRect, fg, 1.0f);
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

    Rect ContentRect() const {
        const float horizontalInset = ScaleValue(8.0f);
        const float verticalInset = ScaleValue(6.0f);
        return Rect::Make(
            m_bounds.left + horizontalInset,
            m_bounds.top + verticalInset,
            std::max(m_bounds.left + horizontalInset, m_bounds.right - horizontalInset),
            std::max(m_bounds.top + verticalInset, m_bounds.bottom - verticalInset));
    }

    Rect TextLineRect(const Rect& contentRect) const {
        const Size metrics = MeasureTextValue(
            m_text.empty() ? L" " : m_text,
            fontSize,
            std::max(1.0f, contentRect.Width()),
            TextWrapMode::NoWrap);
        const float lineHeight = std::min(contentRect.Height(), std::max(1.0f, metrics.height));
        const float top = contentRect.top + std::max(0.0f, (contentRect.Height() - lineHeight) * 0.5f);
        return Rect::Make(contentRect.left, top, contentRect.right, top + lineHeight);
    }

    float MeasureNoWrapTextWidth(const std::wstring& text, float maxWidth) const {
        if (text.empty()) {
            return 0.0f;
        }
        return MeasureTextValue(
            text,
            fontSize,
            std::max(1.0f, maxWidth),
            TextWrapMode::NoWrap).width;
    }

    Rect CaretRect() const {
        const Rect textRect = ContentRect();
        const Rect lineRect = TextLineRect(textRect);
        const std::wstring prefix = m_text.substr(0, m_caretPosition);
        const float caretX = lineRect.left + MeasureNoWrapTextWidth(prefix, textRect.Width());
        return Rect::Make(caretX, lineRect.top, caretX + ScaleValue(1.0f), lineRect.bottom);
    }

    bool m_isDragging = false;
    size_t m_dragAnchor = 0;
};

class Checkbox : public UIElement {
public:
    explicit Checkbox(std::wstring text = L"") : m_text(std::move(text)) {
        m_style = DefaultStyle(nullptr);
    }

    static ComponentStyle DefaultStyle(const Theme* theme = nullptr) {
        ComponentStyle s;
        BoxDecoration normalDeco;
        normalDeco.background = ComponentStyle::ThemeColor(theme, "btn-bg", ColorFromHex(0x2D2D30));
        normalDeco.border.width = Thickness{1.0f, 1.0f, 1.0f, 1.0f};
        normalDeco.border.color = ComponentStyle::ThemeColor(theme, "border", ColorFromHex(0x6A6A6A));
        s.base.decoration = normalDeco;
        s.base.foreground = ComponentStyle::ThemeColor(theme, "fg", ColorFromHex(0xEDEDED));

        BoxDecoration focusedDeco = normalDeco;
        focusedDeco.border.width = Thickness{2.0f, 2.0f, 2.0f, 2.0f};
        focusedDeco.border.color = ComponentStyle::ThemeColor(theme, "border-focus", ColorFromHex(0xFFFFFF));
        s.overrides[static_cast<std::size_t>(StyleState::Focused)].decoration = focusedDeco;

        BoxDecoration disabledDeco = normalDeco;
        disabledDeco.background = ComponentStyle::ThemeColor(theme, "surface-4", ColorFromHex(0x1F1F1F));
        disabledDeco.border.color = ComponentStyle::ThemeColor(theme, "border-subtle", ColorFromHex(0x3A3A3A));
        s.overrides[static_cast<std::size_t>(StyleState::Disabled)].decoration = disabledDeco;
        s.overrides[static_cast<std::size_t>(StyleState::Disabled)].opacity = 0.6f;
        return s;
    }

    void ApplyThemeDefaults() override {
        if (m_styleFromLayout) {
            return;
        }
        const Theme* theme = (m_context && m_context->theme) ? m_context->theme : nullptr;
        m_style = DefaultStyle(theme);
        if (m_style.base.foreground.has_value()) {
            textColor = *m_style.base.foreground;
        }
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
        m_style = DefaultStyle(nullptr);
    }

    static ComponentStyle DefaultStyle(const Theme* theme = nullptr) {
        ComponentStyle s;
        BoxDecoration normalDeco;
        normalDeco.background = ColorFromHex(0x000000, 0.0f);  // transparent
        normalDeco.border.width = Thickness{1.0f, 1.0f, 1.0f, 1.0f};
        normalDeco.border.color = ComponentStyle::ThemeColor(theme, "border", ColorFromHex(0x6A6A6A));
        normalDeco.radius = CornerRadius::Uniform(9.0f);
        s.base.decoration = normalDeco;
        s.base.foreground = ComponentStyle::ThemeColor(theme, "fg", ColorFromHex(0xEDEDED));

        BoxDecoration focusedDeco = normalDeco;
        focusedDeco.border.width = Thickness{2.0f, 2.0f, 2.0f, 2.0f};
        focusedDeco.border.color = ComponentStyle::ThemeColor(theme, "border-focus", ColorFromHex(0xFFFFFF));
        s.overrides[static_cast<std::size_t>(StyleState::Focused)].decoration = focusedDeco;

        BoxDecoration disabledDeco = normalDeco;
        disabledDeco.border.color = ComponentStyle::ThemeColor(theme, "border-subtle", ColorFromHex(0x3A3A3A));
        s.overrides[static_cast<std::size_t>(StyleState::Disabled)].decoration = disabledDeco;
        s.overrides[static_cast<std::size_t>(StyleState::Disabled)].opacity = 0.6f;
        return s;
    }

    void ApplyThemeDefaults() override {
        if (m_styleFromLayout) {
            return;
        }
        const Theme* theme = (m_context && m_context->theme) ? m_context->theme : nullptr;
        m_style = DefaultStyle(theme);
        if (m_style.base.foreground.has_value()) {
            textColor = *m_style.base.foreground;
        }
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
    explicit Slider(std::wstring label = L"") : m_label(std::move(label)) {
        m_style = DefaultStyle(nullptr);
    }

    static ComponentStyle DefaultStyle(const Theme* theme = nullptr) {
        ComponentStyle s;
        BoxDecoration shell;
        shell.background = ComponentStyle::ThemeColor(theme, "input-bg", ColorFromHex(0x1F1F1F));
        shell.border.width = Thickness{1.0f, 1.0f, 1.0f, 1.0f};
        shell.border.color = ComponentStyle::ThemeColor(theme, "border", ColorFromHex(0x6A6A6A));
        shell.radius = CornerRadius::Uniform(ComponentStyle::ThemeNumber(theme, Theme::NumberCategory::Radius, "md", 8.0f));
        s.base.decoration = shell;
        s.base.foreground = ComponentStyle::ThemeColor(theme, "fg", ColorFromHex(0xEDEDED));
        s.base.fontSize = ComponentStyle::ThemeNumber(theme, Theme::NumberCategory::FontSize, "sm", 13.0f);

        BoxDecoration track;
        track.background = ComponentStyle::ThemeColor(theme, "surface-4", ColorFromHex(0x333333));
        track.radius = CornerRadius::Uniform(4.0f);
        s.base.track = track;

        BoxDecoration fill;
        fill.background = ComponentStyle::ThemeColor(theme, "accent", ColorFromHex(0x2D6CDF));
        fill.radius = CornerRadius::Uniform(4.0f);
        s.base.fill = fill;

        BoxDecoration thumb;
        thumb.background = ComponentStyle::ThemeColor(theme, "accent", ColorFromHex(0x2D6CDF));
        thumb.border.width = Thickness{1.0f, 1.0f, 1.0f, 1.0f};
        thumb.border.color = ComponentStyle::ThemeColor(theme, "border", ColorFromHex(0x6A6A6A));
        thumb.radius = CornerRadius::Uniform(8.0f);
        s.base.thumb = thumb;

        BoxDecoration focusedShell = shell;
        focusedShell.border.width = Thickness{2.0f, 2.0f, 2.0f, 2.0f};
        focusedShell.border.color = ComponentStyle::ThemeColor(theme, "border-focus", ColorFromHex(0xFFFFFF));
        s.overrides[static_cast<std::size_t>(StyleState::Focused)].decoration = focusedShell;

        BoxDecoration hoverThumb = thumb;
        hoverThumb.background = ComponentStyle::ThemeColor(theme, "accent", ColorFromHex(0x4A8AFF));
        s.overrides[static_cast<std::size_t>(StyleState::Hover)].thumb = hoverThumb;

        BoxDecoration pressedThumb = thumb;
        pressedThumb.background = ComponentStyle::ThemeColor(theme, "accent-soft", ColorFromHex(0x1A5AD0));
        s.overrides[static_cast<std::size_t>(StyleState::Pressed)].thumb = pressedThumb;

        return s;
    }

    void ApplyThemeDefaults() override {
        if (m_styleFromLayout) {
            return;
        }
        const Theme* theme = (m_context && m_context->theme) ? m_context->theme : nullptr;
        m_style = DefaultStyle(theme);
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
        if (!m_hasFocus && !m_hovered && !m_dragging) {
            return false;
        }
        m_hasFocus = false;
        m_hovered = false;
        m_pressed = false;
        m_dragging = false;
        return true;
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
        m_pressed = true;
        m_hovered = true;
        UpdateValueFromPoint(x);
        return true;
    }

    bool OnMouseMove(float x, float y) override {
        bool changed = false;
        const bool nextHover = HitTest(x, y);
        if (nextHover != m_hovered) {
            m_hovered = nextHover;
            changed = true;
        }
        if (m_dragging) {
            UpdateValueFromPoint(x);
            return true;
        }
        return changed;
    }

    bool OnMouseUp(float x, float y) override {
        if (!m_dragging && !m_pressed) {
            return false;
        }
        m_dragging = false;
        m_pressed = false;
        m_hovered = HitTest(x, y);
        return true;
    }

    bool OnMouseLeave() override {
        if (!m_dragging && !m_hovered && !m_pressed) {
            return false;
        }
        if (m_dragging) {
            m_dragging = false;
        }
        m_pressed = false;
        m_hovered = false;
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
        SyncQuickSetToStyle();
        StyleState state = GetCurrentState();
        if (m_dragging) {
            state = StyleState::Pressed;
        }
        const StyleSpec resolved = m_style.Resolve(state);

        BoxDecoration shell = resolved.decoration.value_or(BoxDecoration{});
        shell.radius = CornerRadius::Uniform(ScaleValue(
            shell.radius.IsZero() ? cornerRadius : shell.radius.MaxRadius()));
        shell.border.width = Thickness{
            ScaleValue(shell.border.width.left),
            ScaleValue(shell.border.width.top),
            ScaleValue(shell.border.width.right),
            ScaleValue(shell.border.width.bottom)
        };
        if (resolved.opacity.has_value()) {
            shell.opacity = *resolved.opacity;
        }
        DrawBoxDecoration(renderer, m_bounds, shell);

        const float trackLeft = m_bounds.left + ScaleValue(14.0f);
        const float trackRight = m_bounds.right - ScaleValue(14.0f);
        const float trackTop = m_bounds.top + (m_bounds.bottom - m_bounds.top) * 0.5f - ScaleValue(4.0f);
        const float trackBottom = trackTop + ScaleValue(8.0f);
        const Rect trackRect = Rect::Make(trackLeft, trackTop, trackRight, trackBottom);

        BoxDecoration trackDeco = resolved.track.value_or(BoxDecoration{});
        if (!resolved.track.has_value()) {
            trackDeco.background = ColorFromHex(0x333333);
            trackDeco.radius = CornerRadius::Uniform(4.0f);
        }
        trackDeco.radius = CornerRadius::Uniform(ScaleValue(
            trackDeco.radius.IsZero() ? 4.0f : trackDeco.radius.MaxRadius()));
        DrawBoxDecoration(renderer, trackRect, trackDeco);

        const float range = std::max(0.001f, m_max - m_min);
        const float ratio = std::clamp((m_value - m_min) / range, 0.0f, 1.0f);
        const float position = trackLeft + (trackRight - trackLeft) * ratio;

        // Filled portion of the track up to the thumb.
        if (ratio > 0.001f) {
            BoxDecoration fillDeco = resolved.fill.value_or(BoxDecoration{});
            if (!resolved.fill.has_value()) {
                fillDeco.background = highlightColor;
                fillDeco.radius = CornerRadius::Uniform(4.0f);
            }
            fillDeco.radius = CornerRadius::Uniform(ScaleValue(
                fillDeco.radius.IsZero() ? 4.0f : fillDeco.radius.MaxRadius()));
            const Rect fillRect = Rect::Make(trackLeft, trackTop, position, trackBottom);
            DrawBoxDecoration(renderer, fillRect, fillDeco);
        }

        const Rect thumbRect = Rect::Make(
            position - ScaleValue(8.0f),
            trackTop - ScaleValue(6.0f),
            position + ScaleValue(8.0f),
            trackBottom + ScaleValue(6.0f));
        BoxDecoration thumbDeco = resolved.thumb.value_or(BoxDecoration{});
        if (!resolved.thumb.has_value()) {
            thumbDeco.background = highlightColor;
            thumbDeco.border.width = Thickness{1.0f, 1.0f, 1.0f, 1.0f};
            thumbDeco.border.color = ColorFromHex(0x6A6A6A);
            thumbDeco.radius = CornerRadius::Uniform(8.0f);
        }
        thumbDeco.radius = CornerRadius::Uniform(ScaleValue(
            thumbDeco.radius.IsZero() ? 8.0f : thumbDeco.radius.MaxRadius()));
        thumbDeco.border.width = Thickness{
            ScaleValue(thumbDeco.border.width.left),
            ScaleValue(thumbDeco.border.width.top),
            ScaleValue(thumbDeco.border.width.right),
            ScaleValue(thumbDeco.border.width.bottom)
        };
        DrawBoxDecoration(renderer, thumbRect, thumbDeco);

        if (!m_label.empty()) {
            Rect labelRect = m_bounds;
            labelRect.right = trackLeft - ScaleValue(8.0f);
            const TextRenderOptions textOptions{
                TextWrapMode::NoWrap,
                TextHorizontalAlign::Start,
                TextVerticalAlign::Center
            };
            const Color fg = resolved.foreground.value_or(textColor);
            const float size = resolved.fontSize.value_or(fontSize);
            renderer.DrawTextW(
                m_label.c_str(),
                static_cast<UINT32>(m_label.size()),
                labelRect,
                fg,
                ScaleValue(size),
                textOptions);
        }
    }

    void SetText(std::wstring text) { m_label = std::move(text); }
    void SetRange(float minValue, float maxValue) { m_min = minValue; m_max = maxValue; SetValue(m_value); }
    void SetValue(float value) { m_value = std::clamp(value, m_min, m_max); }
    void SetStep(float step) { m_step = std::max(0.001f, step); }

    // Legacy quick-set fields (still honored; synced into style before paint).
    float fontSize = 13.0f;
    Color background = ColorFromHex(0x1F1F1F);
    Color borderColor = ColorFromHex(0x6A6A6A);
    Color focusBorderColor = ColorFromHex(0xFFFFFF);
    Color textColor = ColorFromHex(0xEDEDED);
    Color highlightColor = ColorFromHex(0x2D6CDF);
    float cornerRadius = 8.0f;

private:
    void SyncQuickSetToStyle() {
        // Shell + text only. track/thumb/fill come from DefaultStyle / style block;
        // highlightColor is applied once via ApplyHighlightToStyle() when attributes change.
        if (!m_style.base.decoration.has_value()) {
            m_style.base.decoration = BoxDecoration{};
        }
        m_style.base.decoration->background = background;
        m_style.base.decoration->border.color = borderColor;
        m_style.base.decoration->radius = CornerRadius::Uniform(cornerRadius);
        m_style.base.foreground = textColor;
        m_style.base.fontSize = fontSize;
    }

    void ApplyHighlightToStyle() {
        if (!m_style.base.fill.has_value()) {
            m_style.base.fill = BoxDecoration{};
            m_style.base.fill->radius = CornerRadius::Uniform(4.0f);
        }
        m_style.base.fill->background = highlightColor;
        if (!m_style.base.thumb.has_value()) {
            m_style.base.thumb = BoxDecoration{};
            m_style.base.thumb->radius = CornerRadius::Uniform(8.0f);
            m_style.base.thumb->border.width = Thickness{1.0f, 1.0f, 1.0f, 1.0f};
            m_style.base.thumb->border.color = ColorFromHex(0x6A6A6A);
        }
        m_style.base.thumb->background = highlightColor;
    }

public:
    void SetHighlightColor(Color color) {
        highlightColor = color;
        ApplyHighlightToStyle();
    }

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
};

class ProgressBar : public UIElement {
public:
    explicit ProgressBar(std::wstring label = L"") : m_label(std::move(label)) {
        m_style = DefaultStyle(nullptr);
    }

    static ComponentStyle DefaultStyle(const Theme* theme = nullptr) {
        ComponentStyle s;
        BoxDecoration shell;
        shell.background = ComponentStyle::ThemeColor(theme, "surface-2", ColorFromHex(0x1A2431));
        shell.border.width = Thickness{1.0f, 1.0f, 1.0f, 1.0f};
        shell.border.color = ComponentStyle::ThemeColor(theme, "border-subtle", ColorFromHex(0x3A4B5D));
        shell.radius = CornerRadius::Uniform(ComponentStyle::ThemeNumber(theme, Theme::NumberCategory::Radius, "md", 8.0f));
        s.base.decoration = shell;
        s.base.foreground = ComponentStyle::ThemeColor(theme, "fg", ColorFromHex(0xEAF3FF));
        s.base.fontSize = ComponentStyle::ThemeNumber(theme, Theme::NumberCategory::FontSize, "sm", 12.0f);

        BoxDecoration track;
        track.background = ComponentStyle::ThemeColor(theme, "surface-3", ColorFromHex(0x2A3747));
        track.radius = CornerRadius::Uniform(5.0f);
        s.base.track = track;

        BoxDecoration fill;
        fill.background = ComponentStyle::ThemeColor(theme, "accent", ColorFromHex(0x4E7BFF));
        fill.radius = CornerRadius::Uniform(5.0f);
        s.base.fill = fill;

        BoxDecoration focused = shell;
        focused.border.width = Thickness{2.0f, 2.0f, 2.0f, 2.0f};
        focused.border.color = ComponentStyle::ThemeColor(theme, "border-focus", ColorFromHex(0xFFFFFF));
        s.overrides[static_cast<std::size_t>(StyleState::Focused)].decoration = focused;

        BoxDecoration hoverFill = fill;
        hoverFill.background = ComponentStyle::ThemeColor(theme, "accent", ColorFromHex(0x6A93FF));
        s.overrides[static_cast<std::size_t>(StyleState::Hover)].fill = hoverFill;

        BoxDecoration pressedFill = fill;
        pressedFill.background = ComponentStyle::ThemeColor(theme, "accent-soft", ColorFromHex(0x3A68D8));
        s.overrides[static_cast<std::size_t>(StyleState::Pressed)].fill = pressedFill;

        return s;
    }

    void ApplyThemeDefaults() override {
        if (m_styleFromLayout) {
            return;
        }
        const Theme* theme = (m_context && m_context->theme) ? m_context->theme : nullptr;
        m_style = DefaultStyle(theme);
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
        if (!m_hasFocus && !m_dragging && !m_hovered) {
            return false;
        }
        m_hasFocus = false;
        m_dragging = false;
        m_hovered = false;
        m_pressed = false;
        return true;
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
        m_pressed = true;
        m_hovered = true;
        return UpdateValueFromPoint(x);
    }

    bool OnMouseMove(float x, float y) override {
        bool changed = false;
        const bool nextHover = HitTest(x, y);
        if (nextHover != m_hovered) {
            m_hovered = nextHover;
            changed = true;
        }
        if (m_dragging) {
            return UpdateValueFromPoint(x) || changed;
        }
        return changed;
    }

    bool OnMouseUp(float x, float y) override {
        if (!m_dragging && !m_pressed) {
            return false;
        }
        m_dragging = false;
        m_pressed = false;
        m_hovered = HitTest(x, y);
        return true;
    }

    bool OnMouseLeave() override {
        if (!m_dragging && !m_hovered && !m_pressed) {
            return false;
        }
        m_dragging = false;
        m_pressed = false;
        m_hovered = false;
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
        SyncQuickSetToStyle();
        StyleState state = GetCurrentState();
        if (m_dragging) {
            state = StyleState::Pressed;
        }
        const StyleSpec resolved = m_style.Resolve(state);

        BoxDecoration shell = resolved.decoration.value_or(BoxDecoration{});
        shell.radius = CornerRadius::Uniform(ScaleValue(
            shell.radius.IsZero() ? cornerRadius : shell.radius.MaxRadius()));
        shell.border.width = Thickness{
            ScaleValue(shell.border.width.left),
            ScaleValue(shell.border.width.top),
            ScaleValue(shell.border.width.right),
            ScaleValue(shell.border.width.bottom)
        };
        if (resolved.opacity.has_value()) {
            shell.opacity = *resolved.opacity;
        }
        DrawBoxDecoration(renderer, m_bounds, shell);

        const float insetX = ScaleValue(12.0f);
        const float insetY = ScaleValue(8.0f);
        const float barHeightPx = std::max(ScaleValue(4.0f), ScaleValue(barHeight));
        const float trackTop = m_bounds.top + insetY + std::max(0.0f, (m_bounds.Height() - insetY * 2.0f - barHeightPx) * 0.5f);
        const Rect trackRect = Rect::Make(
            m_bounds.left + insetX,
            trackTop,
            m_bounds.right - insetX,
            trackTop + barHeightPx);

        BoxDecoration trackDeco = resolved.track.value_or(BoxDecoration{});
        if (!resolved.track.has_value()) {
            trackDeco.background = trackColor;
            trackDeco.radius = CornerRadius::Uniform(barHeight * 0.5f);
        }
        const float trackRadius = ScaleValue(
            trackDeco.radius.IsZero() ? barHeight * 0.5f : trackDeco.radius.MaxRadius());
        trackDeco.radius = CornerRadius::Uniform(trackRadius);
        DrawBoxDecoration(renderer, trackRect, trackDeco);

        const float progress = GetNormalizedValue();
        const float filledWidth = trackRect.Width() * progress;
        if (filledWidth > 0.5f) {
            BoxDecoration fillDeco = resolved.fill.value_or(BoxDecoration{});
            if (!resolved.fill.has_value()) {
                fillDeco.background = fillColor;
                fillDeco.radius = CornerRadius::Uniform(barHeight * 0.5f);
            }
            fillDeco.radius = CornerRadius::Uniform(ScaleValue(
                fillDeco.radius.IsZero() ? barHeight * 0.5f : fillDeco.radius.MaxRadius()));
            const Rect fillRect = Rect::Make(trackRect.left, trackRect.top, trackRect.left + filledWidth, trackRect.bottom);
            DrawBoxDecoration(renderer, fillRect, fillDeco);
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
            const Color fg = resolved.foreground.value_or(textColor);
            const float size = resolved.fontSize.value_or(fontSize);
            renderer.DrawTextW(
                overlayText.c_str(),
                static_cast<UINT32>(overlayText.size()),
                m_bounds,
                fg,
                ScaleValue(size),
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
    void SetFillColor(Color color) {
        fillColor = color;
        if (!m_style.base.fill.has_value()) {
            m_style.base.fill = BoxDecoration{};
            m_style.base.fill->radius = CornerRadius::Uniform(5.0f);
        }
        m_style.base.fill->background = color;
    }
    void SetTrackColor(Color color) {
        trackColor = color;
        if (!m_style.base.track.has_value()) {
            m_style.base.track = BoxDecoration{};
            m_style.base.track->radius = CornerRadius::Uniform(5.0f);
        }
        m_style.base.track->background = color;
    }

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
    void SyncQuickSetToStyle() {
        if (!m_style.base.decoration.has_value()) {
            m_style.base.decoration = BoxDecoration{};
        }
        m_style.base.decoration->background = background;
        m_style.base.decoration->border.color = borderColor;
        m_style.base.decoration->radius = CornerRadius::Uniform(cornerRadius);
        m_style.base.foreground = textColor;
        m_style.base.fontSize = fontSize;
    }

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

// Spin-box numeric editor: value field + up/down buttons.
// Keyboard: Up/Down (+/-step), PageUp/PageDown (+/-10*step), Home/End.
class NumericUpDown : public UIElement {
public:
    explicit NumericUpDown(std::wstring label = L"") : m_label(std::move(label)) {
        m_style = DefaultStyle(nullptr);
    }

    static ComponentStyle DefaultStyle(const Theme* theme = nullptr) {
        ComponentStyle s;
        BoxDecoration shell;
        shell.background = ComponentStyle::ThemeColor(theme, "input-bg", ColorFromHex(0x1A2431));
        shell.border.width = Thickness{1, 1, 1, 1};
        shell.border.color = ComponentStyle::ThemeColor(theme, "border-subtle", ColorFromHex(0x3A4B5D));
        shell.radius = CornerRadius::Uniform(
            ComponentStyle::ThemeNumber(theme, Theme::NumberCategory::Radius, "md", 8.0f));
        s.base.decoration = shell;
        s.base.foreground = ComponentStyle::ThemeColor(theme, "fg", ColorFromHex(0xEAF3FF));
        s.base.fontSize = ComponentStyle::ThemeNumber(theme, Theme::NumberCategory::FontSize, "sm", 13.0f);

        BoxDecoration focused = shell;
        focused.border.width = Thickness{2, 2, 2, 2};
        focused.border.color = ComponentStyle::ThemeColor(theme, "border-focus", ColorFromHex(0xFFFFFF));
        s.overrides[static_cast<std::size_t>(StyleState::Focused)].decoration = focused;

        BoxDecoration hover;
        hover.background = ComponentStyle::ThemeColor(theme, "surface-3", ColorFromHex(0x2F4258));
        s.overrides[static_cast<std::size_t>(StyleState::Hover)].decoration = hover;

        BoxDecoration pressed;
        pressed.background = ComponentStyle::ThemeColor(theme, "surface-4", ColorFromHex(0x3A5470));
        s.overrides[static_cast<std::size_t>(StyleState::Pressed)].decoration = pressed;
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
        if (!m_hasFocus && m_hoveredButton == 0 && m_pressedButton == 0) {
            return false;
        }
        m_hasFocus = false;
        m_hoveredButton = 0;
        m_pressedButton = 0;
        return true;
    }

    bool OnKeyDown(WPARAM keyCode, LPARAM /*lParam*/) override {
        switch (keyCode) {
            case VK_UP:
                return SetValueInternal(m_value + m_step);
            case VK_DOWN:
                return SetValueInternal(m_value - m_step);
            case VK_PRIOR:
                return SetValueInternal(m_value + m_step * 10.0f);
            case VK_NEXT:
                return SetValueInternal(m_value - m_step * 10.0f);
            case VK_HOME:
                return SetValueInternal(m_min);
            case VK_END:
                return SetValueInternal(m_max);
            default:
                return false;
        }
    }

    bool OnMouseMove(float x, float y) override {
        if (!HitTest(x, y)) {
            if (m_hoveredButton == 0) {
                return false;
            }
            m_hoveredButton = 0;
            return true;
        }
        const int nextHover = ButtonFromPoint(x, y);
        if (nextHover == m_hoveredButton) {
            return false;
        }
        m_hoveredButton = nextHover;
        return true;
    }

    bool OnMouseLeave() override {
        if (m_hoveredButton == 0 && m_pressedButton == 0) {
            return false;
        }
        m_hoveredButton = 0;
        m_pressedButton = 0;
        return true;
    }

    bool OnMouseDown(float x, float y) override {
        if (!HitTest(x, y)) {
            return false;
        }
        m_pressedButton = ButtonFromPoint(x, y);
        if (m_pressedButton == 1) {
            SetValueInternal(m_value + m_step);
            return true;
        }
        if (m_pressedButton == 2) {
            SetValueInternal(m_value - m_step);
            return true;
        }
        return true; // capture focus on value field
    }

    bool OnMouseUp(float x, float y) override {
        if (m_pressedButton == 0) {
            return false;
        }
        m_pressedButton = 0;
        m_hoveredButton = HitTest(x, y) ? ButtonFromPoint(x, y) : 0;
        return true;
    }

protected:
    float MeasurePreferredWidth(float availableWidth) const override {
        return std::min(availableWidth, ScaleValue(220.0f));
    }

    float MeasurePreferredHeight(float width) const override {
        (void)width;
        return ScaleValue(36.0f);
    }

public:
    void Render(IRenderer& renderer) override {
        const StyleSpec shell = m_style.Resolve(m_hasFocus ? StyleState::Focused : StyleState::Normal);
        Color shellBg = background;
        Color shellBorder = borderColor;
        float scaledCorner = ScaleValue(cornerRadius);
        Color shellFg = textColor;
        float shellFs = fontSize;
        if (shell.decoration.has_value()) {
            shellBg = shell.decoration->background;
            shellBorder = shell.decoration->border.color;
            if (!shell.decoration->radius.IsZero()) {
                scaledCorner = ScaleValue(shell.decoration->radius.MaxRadius());
            }
        }
        if (shell.foreground.has_value()) {
            shellFg = *shell.foreground;
        }
        if (shell.fontSize.has_value()) {
            shellFs = *shell.fontSize;
        }

        renderer.FillRoundedRect(m_bounds, shellBg, scaledCorner);
        renderer.DrawRoundedRect(m_bounds, shellBorder, 1.0f, scaledCorner);
        if (m_hasFocus) {
            renderer.DrawRoundedRect(m_bounds, focusBorderColor, 2.0f, scaledCorner);
        }

        const Rect buttons = ButtonsRect();
        // Button strip background
        renderer.FillRect(buttons, buttonStripBackground);
        renderer.DrawLine(
            PointF{buttons.left, m_bounds.top + 1.0f},
            PointF{buttons.left, m_bounds.bottom - 1.0f},
            shellBorder,
            1.0f);
        const float midY = (buttons.top + buttons.bottom) * 0.5f;
        renderer.DrawLine(
            PointF{buttons.left + 1.0f, midY},
            PointF{buttons.right - 1.0f, midY},
            borderColor,
            1.0f);

        DrawSpinButton(renderer, UpButtonRect(), /*up=*/true, m_hoveredButton == 1, m_pressedButton == 1);
        DrawSpinButton(renderer, DownButtonRect(), /*up=*/false, m_hoveredButton == 2, m_pressedButton == 2);

        // Label + value text in the field area
        const float padX = ScaleValue(10.0f);
        Rect field = m_bounds;
        field.right = buttons.left - ScaleValue(4.0f);
        field.left += padX;

        std::wstring display = FormatValue();
        if (!m_label.empty()) {
            display = m_label + L": " + display;
        }
        const TextRenderOptions textOptions{
            TextWrapMode::NoWrap,
            TextHorizontalAlign::Start,
            TextVerticalAlign::Center
        };
        renderer.DrawTextW(
            display.c_str(),
            static_cast<UINT32>(display.size()),
            field,
            shellFg,
            ScaleValue(shellFs),
            textOptions);
    }

    void SetLabel(std::wstring label) { m_label = std::move(label); }
    void SetRange(float minValue, float maxValue) {
        m_min = minValue;
        m_max = std::max(minValue, maxValue);
        SetValue(m_value);
    }
    void SetValue(float value) { m_value = SnapToStep(std::clamp(value, m_min, m_max)); }
    void SetStep(float step) { m_step = std::max(0.0001f, step); }
    void SetDecimalPlaces(int places) { m_decimalPlaces = std::clamp(places, 0, 6); }
    float Value() const { return m_value; }

    Color background = ColorFromHex(0x1A2431);
    Color borderColor = ColorFromHex(0x3A4B5D);
    Color focusBorderColor = ColorFromHex(0xFFFFFF);
    Color buttonStripBackground = ColorFromHex(0x243142);
    Color buttonHoverBackground = ColorFromHex(0x2F4258);
    Color buttonPressedBackground = ColorFromHex(0x3A5470);
    Color glyphColor = ColorFromHex(0xEAF3FF);
    Color textColor = ColorFromHex(0xEAF3FF);
    float cornerRadius = 8.0f;
    float fontSize = 13.0f;

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
            glyphColor = *m_style.base.foreground;
        }
        if (m_style.base.fontSize.has_value()) {
            fontSize = *m_style.base.fontSize;
        }
        const StyleSpec hover = m_style.Resolve(StyleState::Hover);
        const StyleSpec pressed = m_style.Resolve(StyleState::Pressed);
        if (hover.decoration.has_value()) {
            buttonHoverBackground = hover.decoration->background;
        }
        if (pressed.decoration.has_value()) {
            buttonPressedBackground = pressed.decoration->background;
        }
    }

    // 0 = none, 1 = up, 2 = down
    int ButtonFromPoint(float x, float y) const {
        if (UpButtonRect().Contains(x, y)) {
            return 1;
        }
        if (DownButtonRect().Contains(x, y)) {
            return 2;
        }
        return 0;
    }

    Rect ButtonsRect() const {
        // Wide enough to click reliably at 100-150% DPI.
        const float width = ScaleValue(34.0f);
        return Rect::Make(m_bounds.right - width, m_bounds.top, m_bounds.right, m_bounds.bottom);
    }

    Rect UpButtonRect() const {
        const Rect buttons = ButtonsRect();
        const float midY = (buttons.top + buttons.bottom) * 0.5f;
        return Rect::Make(buttons.left, buttons.top, buttons.right, midY);
    }

    Rect DownButtonRect() const {
        const Rect buttons = ButtonsRect();
        const float midY = (buttons.top + buttons.bottom) * 0.5f;
        return Rect::Make(buttons.left, midY, buttons.right, buttons.bottom);
    }

    void DrawSpinButton(IRenderer& renderer, const Rect& rect, bool up, bool hovered, bool pressed) const {
        if (pressed) {
            renderer.FillRect(rect, buttonPressedBackground);
        } else if (hovered) {
            renderer.FillRect(rect, buttonHoverBackground);
        }

        const float cx = (rect.left + rect.right) * 0.5f;
        const float cy = (rect.top + rect.bottom) * 0.5f;
        const float halfW = ScaleValue(5.0f);
        const float halfH = ScaleValue(3.0f);
        std::vector<PointF> chevron;
        if (up) {
            chevron = {
                PointF{cx - halfW, cy + halfH * 0.5f},
                PointF{cx, cy - halfH},
                PointF{cx + halfW, cy + halfH * 0.5f}
            };
        } else {
            chevron = {
                PointF{cx - halfW, cy - halfH * 0.5f},
                PointF{cx, cy + halfH},
                PointF{cx + halfW, cy - halfH * 0.5f}
            };
        }
        renderer.DrawPolyline(chevron, glyphColor, ScaleValue(1.8f));
    }

    bool SetValueInternal(float value) {
        const float next = SnapToStep(std::clamp(value, m_min, m_max));
        if (std::abs(next - m_value) <= 0.00001f) {
            return false;
        }
        m_value = next;
        return true;
    }

    float SnapToStep(float value) const {
        if (m_step <= 0.0f) {
            return value;
        }
        const float relative = value - m_min;
        const float steps = std::round(relative / m_step);
        return std::clamp(m_min + steps * m_step, m_min, m_max);
    }

    std::wstring FormatValue() const {
        wchar_t buffer[64] = {};
        if (m_decimalPlaces <= 0) {
            swprintf_s(buffer, L"%.0f", static_cast<double>(m_value));
        } else {
            swprintf_s(buffer, L"%.*f", m_decimalPlaces, static_cast<double>(m_value));
        }
        return buffer;
    }

    std::wstring m_label;
    float m_min = 0.0f;
    float m_max = 100.0f;
    float m_value = 0.0f;
    float m_step = 1.0f;
    int m_decimalPlaces = 0;
    int m_hoveredButton = 0;
    int m_pressedButton = 0;
};

// Segment-based date/time editor (up-down style, no calendar popup in v1).
// Fields: Y/M/D and/or H/M[/S] depending on mode. Left/Right switch segments;
// Up/Down or chevrons adjust the active segment.
class DateTimePicker : public UIElement {
public:
    enum class Mode {
        Date,
        Time,
        DateTime
    };

    enum class Field {
        Year = 0,
        Month,
        Day,
        Hour,
        Minute,
        Second,
        Count
    };

    explicit DateTimePicker(std::wstring label = L"") : m_label(std::move(label)) {
        m_style = DefaultStyle(nullptr);
        SYSTEMTIME now = {};
        GetLocalTime(&now);
        m_year = now.wYear;
        m_month = now.wMonth;
        m_day = now.wDay;
        m_hour = now.wHour;
        m_minute = now.wMinute;
        m_second = now.wSecond;
        ClampDay();
        EnsureActiveFieldValid();
    }

    static ComponentStyle DefaultStyle(const Theme* theme = nullptr) {
        ComponentStyle s;
        BoxDecoration shell;
        shell.background = ComponentStyle::ThemeColor(theme, "input-bg", ColorFromHex(0x1A2431));
        shell.border.width = Thickness{1, 1, 1, 1};
        shell.border.color = ComponentStyle::ThemeColor(theme, "border-subtle", ColorFromHex(0x3A4B5D));
        shell.radius = CornerRadius::Uniform(
            ComponentStyle::ThemeNumber(theme, Theme::NumberCategory::Radius, "md", 8.0f));
        s.base.decoration = shell;
        s.base.foreground = ComponentStyle::ThemeColor(theme, "fg", ColorFromHex(0xEAF3FF));
        s.base.fontSize = ComponentStyle::ThemeNumber(theme, Theme::NumberCategory::FontSize, "sm", 13.0f);

        BoxDecoration focused = shell;
        focused.border.width = Thickness{2, 2, 2, 2};
        focused.border.color = ComponentStyle::ThemeColor(theme, "border-focus", ColorFromHex(0xFFFFFF));
        s.overrides[static_cast<std::size_t>(StyleState::Focused)].decoration = focused;

        BoxDecoration hover;
        hover.background = ComponentStyle::ThemeColor(theme, "surface-3", ColorFromHex(0x2F4258));
        s.overrides[static_cast<std::size_t>(StyleState::Hover)].decoration = hover;

        BoxDecoration pressed;
        pressed.background = ComponentStyle::ThemeColor(theme, "surface-4", ColorFromHex(0x3A5470));
        s.overrides[static_cast<std::size_t>(StyleState::Pressed)].decoration = pressed;
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
            EnsureActiveFieldValid();
            return true;
        }
        return false;
    }

    bool OnBlur() override {
        if (!m_hasFocus && m_hoveredButton == 0 && m_pressedButton == 0) {
            return false;
        }
        m_hasFocus = false;
        m_hoveredButton = 0;
        m_pressedButton = 0;
        return true;
    }

    bool OnKeyDown(WPARAM keyCode, LPARAM /*lParam*/) override {
        switch (keyCode) {
            case VK_LEFT:
                return MoveActiveField(-1);
            case VK_RIGHT:
                return MoveActiveField(1);
            case VK_UP:
                return AdjustActiveField(1);
            case VK_DOWN:
                return AdjustActiveField(-1);
            case VK_PRIOR:
                return AdjustActiveField(10);
            case VK_NEXT:
                return AdjustActiveField(-10);
            case VK_HOME:
                return SetActiveFieldToMin();
            case VK_END:
                return SetActiveFieldToMax();
            default:
                return false;
        }
    }

    bool OnMouseMove(float x, float y) override {
        if (!HitTest(x, y)) {
            if (m_hoveredButton == 0 && m_hoveredField < 0) {
                return false;
            }
            m_hoveredButton = 0;
            m_hoveredField = -1;
            return true;
        }

        bool changed = false;
        const int nextButton = ButtonFromPoint(x, y);
        if (nextButton != m_hoveredButton) {
            m_hoveredButton = nextButton;
            changed = true;
        }

        const int nextField = nextButton == 0 ? FieldFromPoint(x, y) : -1;
        if (nextField != m_hoveredField) {
            m_hoveredField = nextField;
            changed = true;
        }
        return changed;
    }

    bool OnMouseLeave() override {
        if (m_hoveredButton == 0 && m_pressedButton == 0 && m_hoveredField < 0) {
            return false;
        }
        m_hoveredButton = 0;
        m_pressedButton = 0;
        m_hoveredField = -1;
        return true;
    }

    bool OnMouseDown(float x, float y) override {
        if (!HitTest(x, y)) {
            return false;
        }

        m_pressedButton = ButtonFromPoint(x, y);
        if (m_pressedButton == 1) {
            AdjustActiveField(1);
            return true;
        }
        if (m_pressedButton == 2) {
            AdjustActiveField(-1);
            return true;
        }

        const int field = FieldFromPoint(x, y);
        if (field >= 0) {
            m_activeField = field;
            return true;
        }
        return true;
    }

    bool OnMouseUp(float x, float y) override {
        if (m_pressedButton == 0) {
            return false;
        }
        m_pressedButton = 0;
        m_hoveredButton = HitTest(x, y) ? ButtonFromPoint(x, y) : 0;
        return true;
    }

protected:
    float MeasurePreferredWidth(float availableWidth) const override {
        float ideal = 220.0f;
        if (m_mode == Mode::DateTime) {
            ideal = m_showSeconds ? 300.0f : 260.0f;
        } else if (m_mode == Mode::Time) {
            ideal = m_showSeconds ? 180.0f : 150.0f;
        }
        if (!m_label.empty()) {
            ideal += 80.0f;
        }
        return std::min(availableWidth, ScaleValue(ideal));
    }

    float MeasurePreferredHeight(float width) const override {
        (void)width;
        return ScaleValue(36.0f);
    }

public:
    void Render(IRenderer& renderer) override {
        const StyleSpec shell = m_style.Resolve(m_hasFocus ? StyleState::Focused : StyleState::Normal);
        Color shellBg = background;
        Color shellBorder = borderColor;
        float scaledCorner = ScaleValue(cornerRadius);
        if (shell.decoration.has_value()) {
            shellBg = shell.decoration->background;
            shellBorder = shell.decoration->border.color;
            if (!shell.decoration->radius.IsZero()) {
                scaledCorner = ScaleValue(shell.decoration->radius.MaxRadius());
            }
        }
        if (shell.foreground.has_value()) {
            textColor = *shell.foreground;
        }
        if (shell.fontSize.has_value()) {
            fontSize = *shell.fontSize;
        }

        renderer.FillRoundedRect(m_bounds, shellBg, scaledCorner);
        renderer.DrawRoundedRect(m_bounds, shellBorder, 1.0f, scaledCorner);
        if (m_hasFocus) {
            renderer.DrawRoundedRect(m_bounds, focusBorderColor, 2.0f, scaledCorner);
        }

        const Rect buttons = ButtonsRect();
        renderer.FillRect(buttons, buttonStripBackground);
        renderer.DrawLine(
            PointF{buttons.left, m_bounds.top + 1.0f},
            PointF{buttons.left, m_bounds.bottom - 1.0f},
            shellBorder,
            1.0f);
        const float midY = (buttons.top + buttons.bottom) * 0.5f;
        renderer.DrawLine(
            PointF{buttons.left + 1.0f, midY},
            PointF{buttons.right - 1.0f, midY},
            shellBorder,
            1.0f);

        DrawSpinButton(renderer, UpButtonRect(), true, m_hoveredButton == 1, m_pressedButton == 1);
        DrawSpinButton(renderer, DownButtonRect(), false, m_hoveredButton == 2, m_pressedButton == 2);

        const float padX = ScaleValue(10.0f);
        float cursorX = m_bounds.left + padX;
        const float fieldTop = m_bounds.top;
        const float fieldBottom = m_bounds.bottom;
        const float fieldRightLimit = buttons.left - ScaleValue(4.0f);

        if (!m_label.empty()) {
            const std::wstring labelText = m_label + L": ";
            const float labelWidth = EstimateTextWidth(labelText);
            const Rect labelRect = Rect::Make(cursorX, fieldTop, std::min(cursorX + labelWidth, fieldRightLimit), fieldBottom);
            DrawFieldText(renderer, labelText, labelRect, textColor, false);
            cursorX = labelRect.right;
        }

        const auto fields = ActiveFields();
        for (size_t i = 0; i < fields.size(); ++i) {
            if (i > 0) {
                const wchar_t* sep = SeparatorBefore(fields[i]);
                const float sepWidth = EstimateTextWidth(sep);
                const Rect sepRect = Rect::Make(cursorX, fieldTop, std::min(cursorX + sepWidth, fieldRightLimit), fieldBottom);
                DrawFieldText(renderer, sep, sepRect, mutedTextColor, false);
                cursorX = sepRect.right;
            }

            const int fieldIndex = static_cast<int>(fields[i]);
            const std::wstring token = FormatField(fields[i]);
            const float tokenWidth = EstimateTextWidth(token);
            const Rect tokenRect = Rect::Make(cursorX, fieldTop, std::min(cursorX + tokenWidth + ScaleValue(4.0f), fieldRightLimit), fieldBottom);
            const bool active = m_hasFocus && m_activeField == fieldIndex;
            const bool hovered = m_hoveredField == fieldIndex;
            if (active) {
                renderer.FillRoundedRect(tokenRect, activeFieldBackground, ScaleValue(3.0f));
            } else if (hovered) {
                renderer.FillRoundedRect(tokenRect, hoverFieldBackground, ScaleValue(3.0f));
            }
            DrawFieldText(renderer, token, tokenRect, active ? activeFieldTextColor : textColor, true);
            cursorX = tokenRect.right;
            if (cursorX >= fieldRightLimit) {
                break;
            }
        }
    }

    void SetLabel(std::wstring label) { m_label = std::move(label); }
    void SetMode(Mode mode) {
        m_mode = mode;
        EnsureActiveFieldValid();
    }
    void SetShowSeconds(bool showSeconds) {
        m_showSeconds = showSeconds;
        EnsureActiveFieldValid();
    }
    void SetDate(int year, int month, int day) {
        m_year = std::clamp(year, 1970, 9999);
        m_month = std::clamp(month, 1, 12);
        m_day = day;
        ClampDay();
    }
    void SetTime(int hour, int minute, int second = 0) {
        m_hour = std::clamp(hour, 0, 23);
        m_minute = std::clamp(minute, 0, 59);
        m_second = std::clamp(second, 0, 59);
    }
    bool SetFromString(const std::wstring& text) {
        // Accepts: yyyy-MM-dd | HH:mm[:ss] | yyyy-MM-dd HH:mm[:ss]
        int y = m_year, mo = m_month, d = m_day, h = m_hour, mi = m_minute, s = m_second;
        const wchar_t* p = text.c_str();
        while (*p == L' ' || *p == L'\t') {
            ++p;
        }
        bool hasDate = false;
        bool hasTime = false;
        if (std::iswdigit(p[0]) && std::iswdigit(p[1]) && std::iswdigit(p[2]) && std::iswdigit(p[3]) && p[4] == L'-') {
            int parsedY = 0, parsedM = 0, parsedD = 0;
            if (swscanf_s(p, L"%d-%d-%d", &parsedY, &parsedM, &parsedD) >= 3) {
                y = parsedY;
                mo = parsedM;
                d = parsedD;
                hasDate = true;
                while (*p && *p != L' ' && *p != L'T') {
                    ++p;
                }
                while (*p == L' ' || *p == L'T') {
                    ++p;
                }
            }
        }
        if (std::iswdigit(p[0])) {
            int parsedH = 0, parsedMi = 0, parsedS = 0;
            const int count = swscanf_s(p, L"%d:%d:%d", &parsedH, &parsedMi, &parsedS);
            if (count >= 2) {
                h = parsedH;
                mi = parsedMi;
                if (count >= 3) {
                    s = parsedS;
                    m_showSeconds = true;
                }
                hasTime = true;
            }
        }
        if (!hasDate && !hasTime) {
            return false;
        }
        if (hasDate) {
            SetDate(y, mo, d);
        }
        if (hasTime) {
            SetTime(h, mi, s);
        }
        // Mode is controlled by SetMode / layout props; value only fills fields.
        EnsureActiveFieldValid();
        return true;
    }

    std::wstring ValueText() const {
        wchar_t buffer[64] = {};
        switch (m_mode) {
            case Mode::Date:
                swprintf_s(buffer, L"%04d-%02d-%02d", m_year, m_month, m_day);
                break;
            case Mode::Time:
                if (m_showSeconds) {
                    swprintf_s(buffer, L"%02d:%02d:%02d", m_hour, m_minute, m_second);
                } else {
                    swprintf_s(buffer, L"%02d:%02d", m_hour, m_minute);
                }
                break;
            case Mode::DateTime:
                if (m_showSeconds) {
                    swprintf_s(buffer, L"%04d-%02d-%02d %02d:%02d:%02d", m_year, m_month, m_day, m_hour, m_minute, m_second);
                } else {
                    swprintf_s(buffer, L"%04d-%02d-%02d %02d:%02d", m_year, m_month, m_day, m_hour, m_minute);
                }
                break;
        }
        return buffer;
    }

    Color background = ColorFromHex(0x1A2431);
    Color borderColor = ColorFromHex(0x3A4B5D);
    Color focusBorderColor = ColorFromHex(0xFFFFFF);
    Color buttonStripBackground = ColorFromHex(0x243142);
    Color buttonHoverBackground = ColorFromHex(0x2F4258);
    Color buttonPressedBackground = ColorFromHex(0x3A5470);
    Color activeFieldBackground = ColorFromHex(0x2D6CDF);
    Color hoverFieldBackground = ColorFromHex(0x2A3A4D);
    Color activeFieldTextColor = ColorFromHex(0xFFFFFF);
    Color glyphColor = ColorFromHex(0xEAF3FF);
    Color textColor = ColorFromHex(0xEAF3FF);
    Color mutedTextColor = ColorFromHex(0x8FA3B8);
    float cornerRadius = 8.0f;
    float fontSize = 13.0f;

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
            glyphColor = *m_style.base.foreground;
        }
        if (m_style.base.fontSize.has_value()) {
            fontSize = *m_style.base.fontSize;
        }
        const StyleSpec hover = m_style.Resolve(StyleState::Hover);
        const StyleSpec pressed = m_style.Resolve(StyleState::Pressed);
        if (hover.decoration.has_value()) {
            buttonHoverBackground = hover.decoration->background;
            hoverFieldBackground = hover.decoration->background;
        }
        if (pressed.decoration.has_value()) {
            buttonPressedBackground = pressed.decoration->background;
        }
    }

    std::vector<Field> ActiveFields() const {
        std::vector<Field> fields;
        if (m_mode == Mode::Date || m_mode == Mode::DateTime) {
            fields.push_back(Field::Year);
            fields.push_back(Field::Month);
            fields.push_back(Field::Day);
        }
        if (m_mode == Mode::Time || m_mode == Mode::DateTime) {
            fields.push_back(Field::Hour);
            fields.push_back(Field::Minute);
            if (m_showSeconds) {
                fields.push_back(Field::Second);
            }
        }
        return fields;
    }

    void EnsureActiveFieldValid() {
        const auto fields = ActiveFields();
        if (fields.empty()) {
            m_activeField = static_cast<int>(Field::Year);
            return;
        }
        for (Field f : fields) {
            if (static_cast<int>(f) == m_activeField) {
                return;
            }
        }
        m_activeField = static_cast<int>(fields.front());
    }

    bool MoveActiveField(int delta) {
        const auto fields = ActiveFields();
        if (fields.empty()) {
            return false;
        }
        int index = 0;
        for (size_t i = 0; i < fields.size(); ++i) {
            if (static_cast<int>(fields[i]) == m_activeField) {
                index = static_cast<int>(i);
                break;
            }
        }
        const int next = (index + delta + static_cast<int>(fields.size())) % static_cast<int>(fields.size());
        if (static_cast<int>(fields[static_cast<size_t>(next)]) == m_activeField) {
            return false;
        }
        m_activeField = static_cast<int>(fields[static_cast<size_t>(next)]);
        return true;
    }

    bool AdjustActiveField(int delta) {
        switch (static_cast<Field>(m_activeField)) {
            case Field::Year:
                m_year = std::clamp(m_year + delta, 1970, 9999);
                ClampDay();
                return true;
            case Field::Month: {
                int month = m_month + delta;
                while (month < 1) {
                    month += 12;
                    m_year = std::clamp(m_year - 1, 1970, 9999);
                }
                while (month > 12) {
                    month -= 12;
                    m_year = std::clamp(m_year + 1, 1970, 9999);
                }
                m_month = month;
                ClampDay();
                return true;
            }
            case Field::Day: {
                const int dim = DaysInMonth(m_year, m_month);
                int day = m_day + delta;
                while (day < 1) {
                    int month = m_month - 1;
                    int year = m_year;
                    if (month < 1) {
                        month = 12;
                        year = std::clamp(year - 1, 1970, 9999);
                    }
                    m_year = year;
                    m_month = month;
                    day += DaysInMonth(m_year, m_month);
                }
                while (day > DaysInMonth(m_year, m_month)) {
                    day -= DaysInMonth(m_year, m_month);
                    int month = m_month + 1;
                    int year = m_year;
                    if (month > 12) {
                        month = 1;
                        year = std::clamp(year + 1, 1970, 9999);
                    }
                    m_year = year;
                    m_month = month;
                }
                m_day = day;
                return true;
            }
            case Field::Hour:
                m_hour = (m_hour + delta) % 24;
                if (m_hour < 0) {
                    m_hour += 24;
                }
                return true;
            case Field::Minute:
                m_minute = (m_minute + delta) % 60;
                if (m_minute < 0) {
                    m_minute += 60;
                }
                return true;
            case Field::Second:
                m_second = (m_second + delta) % 60;
                if (m_second < 0) {
                    m_second += 60;
                }
                return true;
            default:
                return false;
        }
    }

    bool SetActiveFieldToMin() {
        switch (static_cast<Field>(m_activeField)) {
            case Field::Year: m_year = 1970; ClampDay(); return true;
            case Field::Month: m_month = 1; ClampDay(); return true;
            case Field::Day: m_day = 1; return true;
            case Field::Hour: m_hour = 0; return true;
            case Field::Minute: m_minute = 0; return true;
            case Field::Second: m_second = 0; return true;
            default: return false;
        }
    }

    bool SetActiveFieldToMax() {
        switch (static_cast<Field>(m_activeField)) {
            case Field::Year: m_year = 9999; ClampDay(); return true;
            case Field::Month: m_month = 12; ClampDay(); return true;
            case Field::Day: m_day = DaysInMonth(m_year, m_month); return true;
            case Field::Hour: m_hour = 23; return true;
            case Field::Minute: m_minute = 59; return true;
            case Field::Second: m_second = 59; return true;
            default: return false;
        }
    }

    void ClampDay() {
        m_day = std::clamp(m_day, 1, DaysInMonth(m_year, m_month));
    }

    static bool IsLeapYear(int year) {
        return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
    }

    static int DaysInMonth(int year, int month) {
        static const int kDays[] = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
        if (month == 2 && IsLeapYear(year)) {
            return 29;
        }
        if (month < 1 || month > 12) {
            return 30;
        }
        return kDays[month];
    }

    std::wstring FormatField(Field field) const {
        wchar_t buffer[16] = {};
        switch (field) {
            case Field::Year: swprintf_s(buffer, L"%04d", m_year); break;
            case Field::Month: swprintf_s(buffer, L"%02d", m_month); break;
            case Field::Day: swprintf_s(buffer, L"%02d", m_day); break;
            case Field::Hour: swprintf_s(buffer, L"%02d", m_hour); break;
            case Field::Minute: swprintf_s(buffer, L"%02d", m_minute); break;
            case Field::Second: swprintf_s(buffer, L"%02d", m_second); break;
            default: break;
        }
        return buffer;
    }

    const wchar_t* SeparatorBefore(Field field) const {
        switch (field) {
            case Field::Month:
            case Field::Day:
                return L"-";
            case Field::Hour:
                return (m_mode == Mode::DateTime) ? L" " : L"";
            case Field::Minute:
            case Field::Second:
                return L":";
            default:
                return L"";
        }
    }

    float EstimateTextWidth(const std::wstring& text) const {
        // Approximate monospace-ish width for layout of segments.
        return ScaleValue(static_cast<float>(text.size()) * fontSize * 0.62f);
    }

    float EstimateTextWidth(const wchar_t* text) const {
        return EstimateTextWidth(std::wstring(text ? text : L""));
    }

    void DrawFieldText(IRenderer& renderer, const std::wstring& text, const Rect& rect, const Color& color, bool center) const {
        const TextRenderOptions options{
            TextWrapMode::NoWrap,
            center ? TextHorizontalAlign::Center : TextHorizontalAlign::Start,
            TextVerticalAlign::Center
        };
        renderer.DrawTextW(text.c_str(), static_cast<UINT32>(text.size()), rect, color, ScaleValue(fontSize), options);
    }

    void DrawFieldText(IRenderer& renderer, const wchar_t* text, const Rect& rect, const Color& color, bool center) const {
        DrawFieldText(renderer, std::wstring(text ? text : L""), rect, color, center);
    }

    Rect ButtonsRect() const {
        const float width = ScaleValue(34.0f);
        return Rect::Make(m_bounds.right - width, m_bounds.top, m_bounds.right, m_bounds.bottom);
    }

    Rect UpButtonRect() const {
        const Rect buttons = ButtonsRect();
        const float midY = (buttons.top + buttons.bottom) * 0.5f;
        return Rect::Make(buttons.left, buttons.top, buttons.right, midY);
    }

    Rect DownButtonRect() const {
        const Rect buttons = ButtonsRect();
        const float midY = (buttons.top + buttons.bottom) * 0.5f;
        return Rect::Make(buttons.left, midY, buttons.right, buttons.bottom);
    }

    int ButtonFromPoint(float x, float y) const {
        if (UpButtonRect().Contains(x, y)) {
            return 1;
        }
        if (DownButtonRect().Contains(x, y)) {
            return 2;
        }
        return 0;
    }

    int FieldFromPoint(float x, float y) const {
        if (!m_bounds.Contains(x, y) || ButtonFromPoint(x, y) != 0) {
            return -1;
        }

        const float padX = ScaleValue(10.0f);
        float cursorX = m_bounds.left + padX;
        const float fieldRightLimit = ButtonsRect().left - ScaleValue(4.0f);

        if (!m_label.empty()) {
            cursorX += EstimateTextWidth(m_label + L": ");
        }

        const auto fields = ActiveFields();
        for (size_t i = 0; i < fields.size(); ++i) {
            if (i > 0) {
                cursorX += EstimateTextWidth(SeparatorBefore(fields[i]));
            }
            const std::wstring token = FormatField(fields[i]);
            const float tokenWidth = EstimateTextWidth(token) + ScaleValue(4.0f);
            const Rect tokenRect = Rect::Make(cursorX, m_bounds.top, std::min(cursorX + tokenWidth, fieldRightLimit), m_bounds.bottom);
            if (tokenRect.Contains(x, y)) {
                return static_cast<int>(fields[i]);
            }
            cursorX = tokenRect.right;
            if (cursorX >= fieldRightLimit) {
                break;
            }
        }
        return -1;
    }

    void DrawSpinButton(IRenderer& renderer, const Rect& rect, bool up, bool hovered, bool pressed) const {
        if (pressed) {
            renderer.FillRect(rect, buttonPressedBackground);
        } else if (hovered) {
            renderer.FillRect(rect, buttonHoverBackground);
        }

        const float cx = (rect.left + rect.right) * 0.5f;
        const float cy = (rect.top + rect.bottom) * 0.5f;
        const float halfW = ScaleValue(4.0f);
        const float halfH = ScaleValue(2.5f);
        std::vector<PointF> chevron;
        if (up) {
            chevron = {
                PointF{cx - halfW, cy + halfH * 0.5f},
                PointF{cx, cy - halfH},
                PointF{cx + halfW, cy + halfH * 0.5f}
            };
        } else {
            chevron = {
                PointF{cx - halfW, cy - halfH * 0.5f},
                PointF{cx, cy + halfH},
                PointF{cx + halfW, cy - halfH * 0.5f}
            };
        }
        renderer.DrawPolyline(chevron, glyphColor, ScaleValue(1.6f));
    }

    std::wstring m_label;
    Mode m_mode = Mode::Date;
    bool m_showSeconds = false;
    int m_year = 2026;
    int m_month = 1;
    int m_day = 1;
    int m_hour = 0;
    int m_minute = 0;
    int m_second = 0;
    int m_activeField = static_cast<int>(Field::Year);
    int m_hoveredField = -1;
    int m_hoveredButton = 0;
    int m_pressedButton = 0;
};

// Multi-line text editor with a bold/italic formatting subset (v1).
// Hard line breaks via Enter. Ctrl+B / Ctrl+I toggle style on selection or typing.
// Optional load-time markup: **bold** and *italic*.
class RichTextBox : public UIElement {
public:
    explicit RichTextBox(std::wstring text = L"") {
        m_style = DefaultStyle(nullptr);
        SetText(std::move(text));
    }

    static ComponentStyle DefaultStyle(const Theme* theme = nullptr) {
        ComponentStyle s;
        BoxDecoration shell;
        shell.background = ComponentStyle::ThemeColor(theme, "input-bg", ColorFromHex(0x1A2431));
        shell.border.width = Thickness{1, 1, 1, 1};
        shell.border.color = ComponentStyle::ThemeColor(theme, "border-subtle", ColorFromHex(0x3A4B5D));
        shell.radius = CornerRadius::Uniform(
            ComponentStyle::ThemeNumber(theme, Theme::NumberCategory::Radius, "md", 8.0f));
        s.base.decoration = shell;
        s.base.foreground = ComponentStyle::ThemeColor(theme, "fg", ColorFromHex(0xEAF3FF));
        s.base.fontSize = ComponentStyle::ThemeNumber(theme, Theme::NumberCategory::FontSize, "sm", 13.0f);

        BoxDecoration focused = shell;
        focused.border.width = Thickness{2, 2, 2, 2};
        focused.border.color = ComponentStyle::ThemeColor(theme, "border-focus", ColorFromHex(0xFFFFFF));
        s.overrides[static_cast<std::size_t>(StyleState::Focused)].decoration = focused;
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
            ResetCaretBlink();
            return true;
        }
        return false;
    }

    bool OnBlur() override {
        if (!m_hasFocus) {
            return false;
        }
        m_hasFocus = false;
        m_isDragging = false;
        return true;
    }

    bool OnKeyDown(WPARAM keyCode, LPARAM /*lParam*/) override {
        const bool ctrlDown = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
        const bool shiftDown = (GetKeyState(VK_SHIFT) & 0x8000) != 0;

        if (ctrlDown && (keyCode == 'B' || keyCode == 'b')) {
            ToggleStyleOnSelectionOrTyping(/*bold=*/true, /*italic=*/false);
            return true;
        }
        if (ctrlDown && (keyCode == 'I' || keyCode == 'i')) {
            ToggleStyleOnSelectionOrTyping(/*bold=*/false, /*italic=*/true);
            return true;
        }
        if (ctrlDown && (keyCode == 'A' || keyCode == 'a')) {
            m_selectionStart = 0;
            m_selectionEnd = m_text.size();
            m_caretPosition = m_text.size();
            ResetCaretBlink();
            return true;
        }

        switch (keyCode) {
            case VK_RETURN:
                InsertText(L"\n");
                return true;
            case VK_BACK:
                if (HasSelection()) {
                    DeleteSelection();
                    ResetCaretBlink();
                    return true;
                }
                if (m_caretPosition > 0) {
                    EraseRange(m_caretPosition - 1, 1);
                    --m_caretPosition;
                    ClearSelection();
                    ResetCaretBlink();
                    return true;
                }
                return false;
            case VK_DELETE:
                if (HasSelection()) {
                    DeleteSelection();
                    ResetCaretBlink();
                    return true;
                }
                if (m_caretPosition < m_text.size()) {
                    EraseRange(m_caretPosition, 1);
                    ResetCaretBlink();
                    return true;
                }
                return false;
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
                return false;
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
                return false;
            case VK_UP:
                return MoveCaretByLine(-1, shiftDown);
            case VK_DOWN:
                return MoveCaretByLine(1, shiftDown);
            case VK_HOME:
                m_caretPosition = LineStart(m_caretPosition);
                if (shiftDown) {
                    if (!HasSelection()) {
                        m_selectionStart = m_selectionEnd;
                    }
                    m_selectionEnd = m_caretPosition;
                } else {
                    ClearSelection();
                }
                ResetCaretBlink();
                return true;
            case VK_END:
                m_caretPosition = LineEnd(m_caretPosition);
                if (shiftDown) {
                    if (!HasSelection()) {
                        m_selectionStart = m_selectionEnd;
                    }
                    m_selectionEnd = m_caretPosition;
                } else {
                    ClearSelection();
                }
                ResetCaretBlink();
                return true;
            default:
                return false;
        }
    }

    bool OnChar(wchar_t ch) override {
        if (ch == L'\b' || ch == L'\r' || ch == L'\n') {
            return true;
        }
        if (ch >= L' ' && ch != 0x7F) {
            InsertText(std::wstring(1, ch));
            return true;
        }
        return false;
    }

    bool OnMouseDown(float x, float y) override {
        if (!HitTest(x, y)) {
            return false;
        }
        const size_t index = IndexFromPoint(x, y);
        const bool shiftDown = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
        if (shiftDown) {
            if (!HasSelection()) {
                m_selectionStart = m_caretPosition;
            }
            m_caretPosition = index;
            m_selectionEnd = m_caretPosition;
        } else {
            m_caretPosition = index;
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
        const size_t index = IndexFromPoint(x, y);
        if (index == m_caretPosition) {
            return false;
        }
        m_caretPosition = index;
        m_selectionStart = m_dragAnchor;
        m_selectionEnd = m_caretPosition;
        ResetCaretBlink();
        return true;
    }

    bool OnMouseUp(float /*x*/, float /*y*/) override {
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

    bool GetTextInputCaretRect(Rect& out) const override {
        if (!m_hasFocus) {
            return false;
        }
        out = CaretRect();
        return true;
    }

protected:
    float MeasurePreferredWidth(float availableWidth) const override {
        return std::min(availableWidth, ScaleValue(420.0f));
    }

    float MeasurePreferredHeight(float width) const override {
        (void)width;
        return ScaleValue(std::max(80.0f, preferredHeight));
    }

public:
    void Render(IRenderer& renderer) override {
        const float scaledCorner = ScaleValue(cornerRadius);
        const StyleSpec shell = m_style.Resolve(m_hasFocus ? StyleState::Focused : StyleState::Normal);
        Color shellBg = background;
        Color shellBorder = borderColor;
        float scaledR = scaledCorner;
        if (shell.decoration.has_value()) {
            shellBg = shell.decoration->background;
            shellBorder = shell.decoration->border.color;
            if (!shell.decoration->radius.IsZero()) {
                scaledR = ScaleValue(shell.decoration->radius.MaxRadius());
            }
        }
        if (shell.foreground.has_value()) {
            textColor = *shell.foreground;
        }
        if (shell.fontSize.has_value()) {
            fontSize = *shell.fontSize;
        }

        renderer.FillRoundedRect(m_bounds, shellBg, scaledR);
        renderer.DrawRoundedRect(m_bounds, shellBorder, 1.0f, scaledR);
        if (m_hasFocus) {
            renderer.DrawRoundedRect(m_bounds, focusBorderColor, 2.0f, scaledR);
        }

        const Rect content = ContentRect();
        renderer.PushRoundedClip(content, ScaleValue(2.0f));

        if (HasSelection()) {
            DrawSelection(renderer, content);
        }

        const auto lines = BuildLines();
        float y = content.top - m_scrollY;
        for (const LineInfo& line : lines) {
            if (y + LineHeight() < content.top) {
                y += LineHeight();
                continue;
            }
            if (y > content.bottom) {
                break;
            }
            DrawStyledLine(renderer, line, content.left, y, content.right);
            y += LineHeight();
        }

        if (m_hasFocus && m_showCaret) {
            const Rect caret = CaretRect();
            if (caret.bottom >= content.top && caret.top <= content.bottom) {
                renderer.FillRect(caret, textColor);
            }
        }

        renderer.PopLayer();
    }

    void SetText(std::wstring text) {
        m_text.clear();
        m_bold.clear();
        m_italic.clear();
        ParseMarkupInto(UnescapeLayoutText(std::move(text)));
        m_caretPosition = m_text.size();
        ClearSelection();
        m_scrollY = 0.0f;
    }

    void SetPlainText(std::wstring text) {
        m_text = UnescapeLayoutText(std::move(text));
        m_bold.assign(m_text.size(), false);
        m_italic.assign(m_text.size(), false);
        m_caretPosition = std::min(m_caretPosition, m_text.size());
        ClearSelection();
    }

    const std::wstring& Text() const { return m_text; }

    Color background = ColorFromHex(0x1A2431);
    Color borderColor = ColorFromHex(0x3A4B5D);
    Color focusBorderColor = ColorFromHex(0xFFFFFF);
    Color textColor = ColorFromHex(0xEAF3FF);
    Color selectionColor = ColorFromHex(0x3A86FF);
    float cornerRadius = 8.0f;
    float fontSize = 13.0f;
    float preferredHeight = 120.0f;

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

    struct LineInfo {
        size_t start = 0;
        size_t end = 0; // exclusive
    };

    static std::wstring UnescapeLayoutText(std::wstring text) {
        // XML attributes often carry literal "\n"; JSON already expands escapes.
        std::wstring out;
        out.reserve(text.size());
        for (size_t i = 0; i < text.size(); ++i) {
            if (text[i] == L'\\' && i + 1 < text.size()) {
                const wchar_t next = text[i + 1];
                if (next == L'n') {
                    out.push_back(L'\n');
                    ++i;
                    continue;
                }
                if (next == L't') {
                    out.push_back(L'\t');
                    ++i;
                    continue;
                }
                if (next == L'\\') {
                    out.push_back(L'\\');
                    ++i;
                    continue;
                }
            }
            out.push_back(text[i]);
        }
        return out;
    }

    void ParseMarkupInto(const std::wstring& source) {
        bool bold = false;
        bool italic = false;
        for (size_t i = 0; i < source.size();) {
            if (i + 1 < source.size() && source[i] == L'*' && source[i + 1] == L'*') {
                bold = !bold;
                i += 2;
                continue;
            }
            if (source[i] == L'*') {
                italic = !italic;
                ++i;
                continue;
            }
            m_text.push_back(source[i]);
            m_bold.push_back(bold);
            m_italic.push_back(italic);
            ++i;
        }
    }

    float LineHeight() const {
        return ScaleValue(fontSize * 1.35f);
    }

    Rect ContentRect() const {
        const float inset = ScaleValue(10.0f);
        return Rect::Make(
            m_bounds.left + inset,
            m_bounds.top + inset,
            std::max(m_bounds.left + inset, m_bounds.right - inset),
            std::max(m_bounds.top + inset, m_bounds.bottom - inset));
    }

    std::vector<LineInfo> BuildLines() const {
        std::vector<LineInfo> lines;
        size_t start = 0;
        for (size_t i = 0; i <= m_text.size(); ++i) {
            if (i == m_text.size() || m_text[i] == L'\n') {
                lines.push_back(LineInfo{start, i});
                start = i + 1;
            }
        }
        if (lines.empty()) {
            lines.push_back(LineInfo{0, 0});
        }
        return lines;
    }

    size_t LineStart(size_t index) const {
        if (m_text.empty()) {
            return 0;
        }
        index = std::min(index, m_text.size());
        while (index > 0 && m_text[index - 1] != L'\n') {
            --index;
        }
        return index;
    }

    size_t LineEnd(size_t index) const {
        index = std::min(index, m_text.size());
        while (index < m_text.size() && m_text[index] != L'\n') {
            ++index;
        }
        return index;
    }

    int LineIndexOf(size_t caret) const {
        const auto lines = BuildLines();
        for (size_t i = 0; i < lines.size(); ++i) {
            if (caret >= lines[i].start && caret <= lines[i].end) {
                // caret == end is on this line unless it's a newline boundary of a later line
                if (caret == lines[i].end && i + 1 < lines.size() && caret < m_text.size() && m_text[caret] == L'\n') {
                    // caret at the newline belongs to this line's end
                }
                return static_cast<int>(i);
            }
        }
        return static_cast<int>(lines.size()) - 1;
    }

    bool MoveCaretByLine(int delta, bool shiftDown) {
        const auto lines = BuildLines();
        if (lines.empty()) {
            return false;
        }
        int lineIndex = LineIndexOf(m_caretPosition);
        const size_t col = m_caretPosition - lines[static_cast<size_t>(lineIndex)].start;
        const int nextLine = std::clamp(lineIndex + delta, 0, static_cast<int>(lines.size()) - 1);
        if (nextLine == lineIndex) {
            return false;
        }
        const LineInfo& target = lines[static_cast<size_t>(nextLine)];
        const size_t lineLen = target.end - target.start;
        const size_t newCaret = target.start + std::min(col, lineLen);

        if (shiftDown && !HasSelection()) {
            m_selectionStart = m_caretPosition;
        }
        m_caretPosition = newCaret;
        if (shiftDown) {
            m_selectionEnd = m_caretPosition;
        } else {
            ClearSelection();
        }
        EnsureCaretVisible();
        ResetCaretBlink();
        return true;
    }

    float MeasureStyledWidth(size_t start, size_t end) const {
        if (start >= end || start >= m_text.size()) {
            return 0.0f;
        }
        end = std::min(end, m_text.size());
        float width = 0.0f;
        size_t i = start;
        while (i < end) {
            const bool bold = StyleAt(i, true);
            const bool italic = StyleAt(i, false);
            size_t j = i + 1;
            while (j < end && StyleAt(j, true) == bold && StyleAt(j, false) == italic) {
                ++j;
            }
            const std::wstring chunk = m_text.substr(i, j - i);
            width += MeasureTextValue(chunk, fontSize, 10000.0f, TextWrapMode::NoWrap).width;
            i = j;
        }
        return width;
    }

    bool StyleAt(size_t index, bool wantBold) const {
        if (index >= m_text.size()) {
            return wantBold ? m_typingBold : m_typingItalic;
        }
        return wantBold ? (index < m_bold.size() && m_bold[index])
                        : (index < m_italic.size() && m_italic[index]);
    }

    void DrawStyledLine(IRenderer& renderer, const LineInfo& line, float left, float top, float right) const {
        float x = left;
        size_t i = line.start;
        while (i < line.end) {
            const bool bold = StyleAt(i, true);
            const bool italic = StyleAt(i, false);
            size_t j = i + 1;
            while (j < line.end && StyleAt(j, true) == bold && StyleAt(j, false) == italic) {
                ++j;
            }
            const std::wstring chunk = m_text.substr(i, j - i);
            const float chunkWidth = MeasureTextValue(chunk, fontSize, 10000.0f, TextWrapMode::NoWrap).width;
            const Rect rect = Rect::Make(x, top, std::min(x + chunkWidth + 1.0f, right), top + LineHeight());
            TextRenderOptions options{
                TextWrapMode::NoWrap,
                TextHorizontalAlign::Start,
                TextVerticalAlign::Center,
                bold,
                italic
            };
            renderer.DrawTextW(
                chunk.c_str(),
                static_cast<UINT32>(chunk.size()),
                rect,
                textColor,
                ScaleValue(fontSize),
                options);
            x += chunkWidth;
            i = j;
            if (x >= right) {
                break;
            }
        }
    }

    void DrawSelection(IRenderer& renderer, const Rect& content) const {
        const auto [selStart, selEnd] = GetSelectionRange();
        if (selStart == selEnd) {
            return;
        }
        const auto lines = BuildLines();
        float y = content.top - m_scrollY;
        for (const LineInfo& line : lines) {
            if (selEnd <= line.start || selStart > line.end) {
                y += LineHeight();
                continue;
            }
            const size_t a = std::max(selStart, line.start);
            const size_t b = std::min(selEnd, line.end);
            float x0 = content.left + MeasureStyledWidth(line.start, a);
            float x1 = content.left + MeasureStyledWidth(line.start, b);
            if (selEnd > line.end) {
                // selection continues past this line - paint a small newline gutter
                x1 = std::max(x1, x0 + ScaleValue(6.0f));
            }
            const Rect sel = Rect::Make(x0, y, std::max(x0 + 2.0f, x1), y + LineHeight());
            if (sel.bottom >= content.top && sel.top <= content.bottom) {
                renderer.FillRect(sel, selectionColor);
            }
            y += LineHeight();
        }
    }

    size_t IndexFromPoint(float x, float y) const {
        const Rect content = ContentRect();
        const auto lines = BuildLines();
        float relY = y - content.top + m_scrollY;
        int lineIndex = static_cast<int>(relY / LineHeight());
        lineIndex = std::clamp(lineIndex, 0, static_cast<int>(lines.size()) - 1);
        const LineInfo& line = lines[static_cast<size_t>(lineIndex)];
        const float localX = x - content.left;

        size_t best = line.start;
        for (size_t i = line.start; i <= line.end; ++i) {
            const float width = MeasureStyledWidth(line.start, i);
            best = i;
            if (localX < width + ScaleValue(3.0f)) {
                break;
            }
        }
        return best;
    }

    Rect CaretRect() const {
        const Rect content = ContentRect();
        const auto lines = BuildLines();
        const int lineIndex = std::max(0, LineIndexOf(m_caretPosition));
        const LineInfo& line = lines[static_cast<size_t>(lineIndex)];
        const float x = content.left + MeasureStyledWidth(line.start, std::min(m_caretPosition, line.end));
        const float y = content.top - m_scrollY + static_cast<float>(lineIndex) * LineHeight();
        return Rect::Make(x, y, x + ScaleValue(1.5f), y + LineHeight());
    }

    void EnsureCaretVisible() {
        const Rect content = ContentRect();
        const int lineIndex = std::max(0, LineIndexOf(m_caretPosition));
        const float caretTop = static_cast<float>(lineIndex) * LineHeight();
        const float caretBottom = caretTop + LineHeight();
        if (caretTop < m_scrollY) {
            m_scrollY = caretTop;
        } else if (caretBottom > m_scrollY + content.Height()) {
            m_scrollY = caretBottom - content.Height();
        }
        m_scrollY = std::max(0.0f, m_scrollY);
    }

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

    void EraseRange(size_t start, size_t count) {
        if (count == 0 || start >= m_text.size()) {
            return;
        }
        count = std::min(count, m_text.size() - start);
        m_text.erase(start, count);
        if (start < m_bold.size()) {
            m_bold.erase(m_bold.begin() + static_cast<std::ptrdiff_t>(start),
                         m_bold.begin() + static_cast<std::ptrdiff_t>(start + count));
        }
        if (start < m_italic.size()) {
            m_italic.erase(m_italic.begin() + static_cast<std::ptrdiff_t>(start),
                           m_italic.begin() + static_cast<std::ptrdiff_t>(start + count));
        }
        SyncStyleVectors();
    }

    void DeleteSelection() {
        if (!HasSelection()) {
            return;
        }
        const auto [start, end] = GetSelectionRange();
        EraseRange(start, end - start);
        m_caretPosition = start;
        ClearSelection();
    }

    void InsertText(const std::wstring& text) {
        if (text.empty()) {
            return;
        }
        if (HasSelection()) {
            DeleteSelection();
        }
        SyncStyleVectors();
        m_text.insert(m_caretPosition, text);
        m_bold.insert(m_bold.begin() + static_cast<std::ptrdiff_t>(m_caretPosition), text.size(), m_typingBold);
        m_italic.insert(m_italic.begin() + static_cast<std::ptrdiff_t>(m_caretPosition), text.size(), m_typingItalic);
        m_caretPosition += text.size();
        ClearSelection();
        EnsureCaretVisible();
        ResetCaretBlink();
    }

    void SyncStyleVectors() {
        if (m_bold.size() < m_text.size()) {
            m_bold.resize(m_text.size(), false);
        } else if (m_bold.size() > m_text.size()) {
            m_bold.resize(m_text.size());
        }
        if (m_italic.size() < m_text.size()) {
            m_italic.resize(m_text.size(), false);
        } else if (m_italic.size() > m_text.size()) {
            m_italic.resize(m_text.size());
        }
    }

    void ToggleStyleOnSelectionOrTyping(bool toggleBold, bool toggleItalic) {
        if (HasSelection()) {
            const auto [start, end] = GetSelectionRange();
            SyncStyleVectors();
            for (size_t i = start; i < end && i < m_text.size(); ++i) {
                if (toggleBold) {
                    m_bold[i] = !m_bold[i];
                }
                if (toggleItalic) {
                    m_italic[i] = !m_italic[i];
                }
            }
            // Update typing style from first selected char
            if (start < m_text.size()) {
                m_typingBold = m_bold[start];
                m_typingItalic = m_italic[start];
            }
        } else {
            if (toggleBold) {
                m_typingBold = !m_typingBold;
            }
            if (toggleItalic) {
                m_typingItalic = !m_typingItalic;
            }
        }
        ResetCaretBlink();
    }

    void ResetCaretBlink() {
        m_showCaret = true;
        m_lastBlink = GetTickCount();
    }

    std::wstring m_text;
    std::vector<bool> m_bold;
    std::vector<bool> m_italic;
    size_t m_caretPosition = 0;
    size_t m_selectionStart = 0;
    size_t m_selectionEnd = 0;
    size_t m_dragAnchor = 0;
    bool m_isDragging = false;
    bool m_showCaret = true;
    bool m_typingBold = false;
    bool m_typingItalic = false;
    DWORD m_lastBlink = 0;
    float m_scrollY = 0.0f;
};

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

    // Open upward when there is not enough room below the header inside the window.
    bool ShouldOpenDropdownUpward(float listHeight) const {
        const Rect header = HeaderRect();
        const float viewportH = (m_context && m_context->viewportHeight > 1.0f)
            ? m_context->viewportHeight
            : 0.0f;
        if (viewportH <= 1.0f) {
            return false;
        }
        const float margin = ScaleValue(4.0f);
        const float spaceBelow = std::max(0.0f, viewportH - header.bottom - margin);
        const float spaceAbove = std::max(0.0f, header.top - margin);
        if (spaceBelow >= listHeight) {
            return false;
        }
        // Prefer the side with more room when neither fully fits.
        return spaceAbove > spaceBelow;
    }

    Rect DropdownRect() const {
        const Rect header = HeaderRect();
        float listHeight = IdealDropdownHeight();
        const float viewportH = (m_context && m_context->viewportHeight > 1.0f)
            ? m_context->viewportHeight
            : 0.0f;
        const float margin = ScaleValue(4.0f);
        const bool openUp = ShouldOpenDropdownUpward(listHeight);

        if (viewportH > 1.0f) {
            const float spaceBelow = std::max(0.0f, viewportH - header.bottom - margin);
            const float spaceAbove = std::max(0.0f, header.top - margin);
            const float available = openUp ? spaceAbove : spaceBelow;
            if (available > 0.0f) {
                listHeight = std::min(listHeight, available);
            }
            // At least one row when possible.
            listHeight = std::max(listHeight, std::min(ScaleValue(itemHeight), available > 0.0f ? available : ScaleValue(itemHeight)));
        }

        if (openUp) {
            return Rect::Make(header.left, header.top - listHeight, header.right, header.top);
        }
        return Rect::Make(header.left, header.bottom, header.right, header.bottom + listHeight);
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
class ScrollViewer : public UIElement {
public:
    bool scrollY = true;
    bool scrollX = false;
    bool showScrollbars = true;
    float barThickness = 10.0f;
    Color trackColor = ColorFromHex(0x1A2430, 0.85f);
    Color thumbColor = ColorFromHex(0x5A6F86, 0.9f);
    Color background = ColorFromHex(0x000000, 0.0f);

    void SetScrollOffset(float x, float y) {
        m_scrollX = x;
        m_scrollY = y;
        ClampScroll();
    }
    float ScrollX() const { return m_scrollX; }
    float ScrollY() const { return m_scrollY; }
    float ContentWidth() const { return m_contentSize.width; }
    float ContentHeight() const { return m_contentSize.height; }

    // Prefer this for capture when panning / scrollbar (not focusable children).
    UIElement* FindHitElementAt(float x, float y) override {
        if (!HitTest(x, y)) {
            return nullptr;
        }
        if (HitTestScrollbar(x, y) != ScrollHit::None) {
            return this;
        }
        for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
            if (!(*it)->HitTest(x, y)) {
                continue;
            }
            if (UIElement* deep = (*it)->FindHitElementAt(x, y)) {
                if (deep->IsFocusable()) {
                    return deep;
                }
            }
            // Non-interactive content: capture ScrollViewer for drag-pan.
            return this;
        }
        return this;
    }

    void Measure(float availableWidth, float availableHeight) override {
        const float viewportW = GetPreferredWidth(availableWidth);
        float viewportH = HasFixedHeight()
            ? GetPreferredHeight(availableWidth)
            : std::max(0.0f, availableHeight);
        if (viewportH <= 0.0f && !HasFixedHeight()) {
            viewportH = ScaleValue(200.0f);
        }

        const float childAvailW = scrollX ? 100000.0f : viewportW;
        const float childAvailH = scrollY ? 100000.0f : viewportH;

        float contentW = 0.0f;
        float contentH = 0.0f;
        for (auto& child : m_children) {
            child->Measure(childAvailW, childAvailH);
            contentW = std::max(contentW, child->DesiredSize().width);
            contentH = std::max(contentH, child->DesiredSize().height);
        }
        m_contentSize = Size{contentW, contentH};

        m_desiredSize.width = viewportW;
        m_desiredSize.height = HasFixedHeight()
            ? GetPreferredHeight(availableWidth)
            : (availableHeight > 0.0f && availableHeight < 100000.0f
                ? viewportH
                : std::min(contentH, ScaleValue(400.0f)));
        ClampScroll();
    }

    void Arrange(const Rect& finalRect) override {
        UIElement::Arrange(finalRect);
        ClampScroll();
        const float viewW = m_bounds.Width();
        const float viewH = m_bounds.Height();
        for (auto& child : m_children) {
            const float cw = scrollX
                ? std::max(child->DesiredSize().width, viewW)
                : viewW;
            const float ch = scrollY
                ? std::max(child->DesiredSize().height, viewH)
                : viewH;
            child->Arrange(Rect::Make(
                m_bounds.left - m_scrollX,
                m_bounds.top - m_scrollY,
                m_bounds.left - m_scrollX + cw,
                m_bounds.top - m_scrollY + ch));
        }
    }

    void Render(IRenderer& renderer) override {
        if (background.a > 0.0f) {
            renderer.FillRect(m_bounds, background);
        }
        renderer.PushRoundedClip(m_bounds, 0.0f);
        for (auto& child : m_children) {
            child->Render(renderer);
        }
        renderer.PopLayer();

        if (showScrollbars) {
            PaintPillScrollbars(
                renderer,
                m_bounds,
                m_contentSize.width,
                m_contentSize.height,
                m_bounds.Width(),
                m_bounds.Height(),
                m_scrollX,
                m_scrollY,
                trackColor,
                thumbColor,
                ScaleValue(barThickness));
        }
    }

    bool OnMouseWheel(float delta, float x, float y, bool shiftHeld) override {
        if (!HitTest(x, y)) {
            return false;
        }
        for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
            if ((*it)->HitTest(x, y) && (*it)->OnMouseWheel(delta, x, y, shiftHeld)) {
                return true;
            }
        }
        return ApplyWheel(delta, shiftHeld);
    }

    bool OnMouseDown(float x, float y) override {
        if (!HitTest(x, y)) {
            return false;
        }

        const ScrollHit hit = HitTestScrollbar(x, y);
        if (hit == ScrollHit::ThumbV || hit == ScrollHit::TrackV) {
            m_drag = DragMode::ThumbV;
            m_lastPointerX = x;
            m_lastPointerY = y;
            if (hit == ScrollHit::TrackV) {
                // Jump thumb toward click.
                ScrollFromThumbCenterY(y);
            }
            return true;
        }
        if (hit == ScrollHit::ThumbH || hit == ScrollHit::TrackH) {
            m_drag = DragMode::ThumbH;
            m_lastPointerX = x;
            m_lastPointerY = y;
            if (hit == ScrollHit::TrackH) {
                ScrollFromThumbCenterX(x);
            }
            return true;
        }

        // Focusable children (buttons, inputs) keep their clicks — no pan steal.
        for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
            if (!(*it)->HitTest(x, y)) {
                continue;
            }
            if (UIElement* deep = (*it)->FindHitElementAt(x, y)) {
                if (deep->IsFocusable()) {
                    deep->OnMouseDown(x, y);
                    return true;
                }
            }
        }

        // Content drag-pan (mouse swipe on non-interactive area).
        if (scrollY || scrollX) {
            m_drag = DragMode::Content;
            m_lastPointerX = x;
            m_lastPointerY = y;
            return true;
        }
        return false;
    }

    bool OnMouseMove(float x, float y) override {
        if (m_drag == DragMode::None) {
            // Hover for children.
            bool changed = false;
            for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
                if ((*it)->HitTest(x, y)) {
                    changed = (*it)->OnMouseMove(x, y) || changed;
                } else {
                    changed = (*it)->OnMouseLeave() || changed;
                }
            }
            return changed;
        }

        const float dx = x - m_lastPointerX;
        const float dy = y - m_lastPointerY;
        m_lastPointerX = x;
        m_lastPointerY = y;
        bool changed = false;

        if (m_drag == DragMode::Content) {
            // Drag content with pointer (natural: pull content up → increase scrollY).
            if (scrollY) {
                const float prev = m_scrollY;
                m_scrollY = std::clamp(m_scrollY - dy, 0.0f, MaxScrollY());
                changed = changed || std::abs(m_scrollY - prev) > 0.1f;
            }
            if (scrollX) {
                const float prev = m_scrollX;
                m_scrollX = std::clamp(m_scrollX - dx, 0.0f, MaxScrollX());
                changed = changed || std::abs(m_scrollX - prev) > 0.1f;
            }
        } else if (m_drag == DragMode::ThumbV && scrollY) {
            const ScrollbarMetrics m = ComputeMetrics();
            if (m.trackVH > m.thumbVH + 0.5f && MaxScrollY() > 0.0f) {
                const float range = m.trackVH - m.thumbVH;
                const float prev = m_scrollY;
                m_scrollY = std::clamp(m_scrollY + dy / range * MaxScrollY(), 0.0f, MaxScrollY());
                changed = std::abs(m_scrollY - prev) > 0.1f;
            }
        } else if (m_drag == DragMode::ThumbH && scrollX) {
            const ScrollbarMetrics m = ComputeMetrics();
            if (m.trackHW > m.thumbHW + 0.5f && MaxScrollX() > 0.0f) {
                const float range = m.trackHW - m.thumbHW;
                const float prev = m_scrollX;
                m_scrollX = std::clamp(m_scrollX + dx / range * MaxScrollX(), 0.0f, MaxScrollX());
                changed = std::abs(m_scrollX - prev) > 0.1f;
            }
        }

        if (changed) {
            Arrange(m_bounds);
        }
        return changed || m_drag != DragMode::None;
    }

    bool OnMouseUp(float x, float y) override {
        (void)x;
        (void)y;
        if (m_drag != DragMode::None) {
            m_drag = DragMode::None;
            return true;
        }
        bool changed = false;
        for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
            changed = (*it)->OnMouseUp(x, y) || changed;
        }
        return changed;
    }

    bool OnMouseLeave() override {
        // Keep drag if capture is held by host; only clear hover.
        bool changed = false;
        for (auto& child : m_children) {
            changed = child->OnMouseLeave() || changed;
        }
        return changed;
    }

protected:
    float MeasurePreferredWidth(float availableWidth) const override {
        return availableWidth;
    }

    float MeasurePreferredHeight(float width) const override {
        (void)width;
        return ScaleValue(200.0f);
    }

private:
    enum class DragMode { None, Content, ThumbV, ThumbH };
    enum class ScrollHit { None, ThumbV, TrackV, ThumbH, TrackH };

    struct ScrollbarMetrics {
        float thick = 0.0f;
        float inset = 0.0f;
        bool needV = false;
        bool needH = false;
        float trackVLeft = 0.0f;
        float trackVTop = 0.0f;
        float trackVH = 0.0f;
        float thumbVTop = 0.0f;
        float thumbVH = 0.0f;
        float trackHTop = 0.0f;
        float trackHLeft = 0.0f;
        float trackHW = 0.0f;
        float thumbHLeft = 0.0f;
        float thumbHW = 0.0f;
    };

    ScrollbarMetrics ComputeMetrics() const {
        ScrollbarMetrics m;
        m.thick = std::max(4.0f, ScaleValue(barThickness));
        m.inset = std::max(1.0f, m.thick * 0.25f);
        const float viewW = m_bounds.Width();
        const float viewH = m_bounds.Height();
        m.needV = scrollY && m_contentSize.height > viewH + 0.5f;
        m.needH = scrollX && m_contentSize.width > viewW + 0.5f;
        const float corner = (m.needV && m.needH) ? (m.thick + m.inset) : 0.0f;

        if (m.needV) {
            m.trackVLeft = m_bounds.right - m.inset - m.thick;
            m.trackVTop = m_bounds.top + m.inset;
            m.trackVH = std::max(m.thick, m_bounds.bottom - m.inset - corner - m.trackVTop);
            m.thumbVH = std::clamp(viewH / std::max(1.0f, m_contentSize.height) * m.trackVH, m.thick * 1.6f, m.trackVH);
            const float maxS = MaxScrollY();
            const float t = maxS > 0.0f ? std::clamp(m_scrollY / maxS, 0.0f, 1.0f) : 0.0f;
            m.thumbVTop = m.trackVTop + t * (m.trackVH - m.thumbVH);
        }
        if (m.needH) {
            m.trackHTop = m_bounds.bottom - m.inset - m.thick;
            m.trackHLeft = m_bounds.left + m.inset;
            m.trackHW = std::max(m.thick, m_bounds.right - m.inset - corner - m.trackHLeft);
            m.thumbHW = std::clamp(viewW / std::max(1.0f, m_contentSize.width) * m.trackHW, m.thick * 1.6f, m.trackHW);
            const float maxS = MaxScrollX();
            const float t = maxS > 0.0f ? std::clamp(m_scrollX / maxS, 0.0f, 1.0f) : 0.0f;
            m.thumbHLeft = m.trackHLeft + t * (m.trackHW - m.thumbHW);
        }
        return m;
    }

    ScrollHit HitTestScrollbar(float x, float y) const {
        if (!showScrollbars) {
            return ScrollHit::None;
        }
        const ScrollbarMetrics m = ComputeMetrics();
        if (m.needV) {
            const Rect thumb = Rect::Make(m.trackVLeft, m.thumbVTop, m.trackVLeft + m.thick, m.thumbVTop + m.thumbVH);
            const Rect track = Rect::Make(m.trackVLeft, m.trackVTop, m.trackVLeft + m.thick, m.trackVTop + m.trackVH);
            if (thumb.Contains(x, y)) {
                return ScrollHit::ThumbV;
            }
            if (track.Contains(x, y)) {
                return ScrollHit::TrackV;
            }
        }
        if (m.needH) {
            const Rect thumb = Rect::Make(m.thumbHLeft, m.trackHTop, m.thumbHLeft + m.thumbHW, m.trackHTop + m.thick);
            const Rect track = Rect::Make(m.trackHLeft, m.trackHTop, m.trackHLeft + m.trackHW, m.trackHTop + m.thick);
            if (thumb.Contains(x, y)) {
                return ScrollHit::ThumbH;
            }
            if (track.Contains(x, y)) {
                return ScrollHit::TrackH;
            }
        }
        return ScrollHit::None;
    }

    void ScrollFromThumbCenterY(float y) {
        const ScrollbarMetrics m = ComputeMetrics();
        if (!m.needV || m.trackVH <= m.thumbVH + 0.5f) {
            return;
        }
        const float center = y - m.thumbVH * 0.5f;
        const float t = std::clamp((center - m.trackVTop) / (m.trackVH - m.thumbVH), 0.0f, 1.0f);
        m_scrollY = t * MaxScrollY();
        ClampScroll();
        Arrange(m_bounds);
    }

    void ScrollFromThumbCenterX(float x) {
        const ScrollbarMetrics m = ComputeMetrics();
        if (!m.needH || m.trackHW <= m.thumbHW + 0.5f) {
            return;
        }
        const float center = x - m.thumbHW * 0.5f;
        const float t = std::clamp((center - m.trackHLeft) / (m.trackHW - m.thumbHW), 0.0f, 1.0f);
        m_scrollX = t * MaxScrollX();
        ClampScroll();
        Arrange(m_bounds);
    }

    bool ApplyWheel(float delta, bool shiftHeld) {
        const float step = ScaleValue(48.0f);
        bool changed = false;
        if (shiftHeld || (!scrollY && scrollX)) {
            if (scrollX) {
                const float prev = m_scrollX;
                m_scrollX = std::clamp(m_scrollX - delta * step, 0.0f, MaxScrollX());
                changed = std::abs(m_scrollX - prev) > 0.5f;
            }
        } else if (scrollY) {
            const float prev = m_scrollY;
            m_scrollY = std::clamp(m_scrollY - delta * step, 0.0f, MaxScrollY());
            changed = std::abs(m_scrollY - prev) > 0.5f;
        }
        if (changed) {
            Arrange(m_bounds);
        }
        return changed;
    }

    float MaxScrollX() const {
        return std::max(0.0f, m_contentSize.width - m_bounds.Width());
    }
    float MaxScrollY() const {
        return std::max(0.0f, m_contentSize.height - m_bounds.Height());
    }
    void ClampScroll() {
        m_scrollX = std::clamp(m_scrollX, 0.0f, MaxScrollX());
        m_scrollY = std::clamp(m_scrollY, 0.0f, MaxScrollY());
    }

    Size m_contentSize{};
    float m_scrollX = 0.0f;
    float m_scrollY = 0.0f;
    DragMode m_drag = DragMode::None;
    float m_lastPointerX = 0.0f;
    float m_lastPointerY = 0.0f;
};

class ListView : public UIElement {
public:
    struct Column {
        std::wstring title;
        float width = -1.0f;
    };

    ListView() { m_style = DefaultStyle(nullptr); }

    static ComponentStyle DefaultStyle(const Theme* theme = nullptr) {
        ComponentStyle s;
        BoxDecoration shell;
        shell.background = ComponentStyle::ThemeColor(theme, "surface-1", ColorFromHex(0x101C29));
        shell.border.width = Thickness{1, 1, 1, 1};
        shell.border.color = ComponentStyle::ThemeColor(theme, "border-subtle", ColorFromHex(0x35506A));
        shell.radius = CornerRadius::Uniform(
            ComponentStyle::ThemeNumber(theme, Theme::NumberCategory::Radius, "lg", 10.0f));
        s.base.decoration = shell;
        s.base.foreground = ComponentStyle::ThemeColor(theme, "fg", ColorFromHex(0xD0DDEC));
        s.base.fontSize = ComponentStyle::ThemeNumber(theme, Theme::NumberCategory::FontSize, "sm", 13.0f);

        BoxDecoration focused = shell;
        focused.border.width = Thickness{2, 2, 2, 2};
        focused.border.color = ComponentStyle::ThemeColor(theme, "border-focus", ColorFromHex(0xFFFFFF));
        s.overrides[static_cast<std::size_t>(StyleState::Focused)].decoration = focused;

        auto item = std::make_unique<ComponentStyle>();
        item->base.decoration = BoxDecoration{};
        item->base.decoration->background = Color{0, 0, 0, 0};
        item->base.foreground = ComponentStyle::ThemeColor(theme, "fg", ColorFromHex(0xD0DDEC));
        BoxDecoration hover;
        hover.background = ComponentStyle::ThemeColor(theme, "surface-3", ColorFromHex(0x203B56));
        item->overrides[static_cast<std::size_t>(StyleState::Hover)].decoration = hover;
        BoxDecoration selected;
        selected.background = ComponentStyle::ThemeColor(theme, "accent-soft", ColorFromHex(0x2D567C));
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
            case VK_LEFT:
                return ScrollHorizontal(-ScaleValue(40.0f));
            case VK_RIGHT:
                return ScrollHorizontal(ScaleValue(40.0f));
            default:
                return false;
        }
    }

    bool OnMouseWheel(float delta, float x, float y, bool shiftHeld) override {
        if (!HitTest(x, y)) {
            return false;
        }
        if (shiftHeld) {
            return ScrollHorizontal(-delta * ScaleValue(48.0f));
        }
        if (m_rows.empty()) {
            return false;
        }
        const int before = m_scrollOffset;
        m_scrollOffset = std::clamp(
            m_scrollOffset - static_cast<int>(std::lround(delta * 3.0f)),
            0,
            std::max(0, static_cast<int>(m_rows.size()) - VisibleRowCount()));
        return m_scrollOffset != before;
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
        // Prefer a compact default so validation pages fit without huge empty body.
        const int rows = std::clamp(static_cast<int>(m_rows.size()), 3, 6);
        return header + row * static_cast<float>(rows) + ScaleValue(2.0f);
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

        renderer.FillRoundedRect(m_bounds, shellBg, scaledCornerRadius);
        renderer.DrawRoundedRect(m_bounds, shellBorder, 1.0f, scaledCornerRadius);
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
                shellFg,
                ScaleValue(shellFs),
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
        const ComponentStyle* itemStyle = m_style.itemStyle ? m_style.itemStyle.get() : nullptr;

        float contentW = 0.0f;
        for (float w : widths) {
            contentW += w;
        }
        const float contentH = scaledRowHeight * static_cast<float>(std::max<size_t>(1, m_rows.size()));
        const float viewW = std::max(1.0f, m_bounds.Width());
        const float viewH = std::max(1.0f, bodyRect.Height());
        ClampHorizontalScroll(contentW, viewW);

        renderer.PushRoundedClip(m_bounds, scaledCornerRadius);
        renderer.FillRect(headerRect, headerBackground);
        renderer.DrawLine(
            PointF{m_bounds.left, headerRect.bottom},
            PointF{m_bounds.right, headerRect.bottom},
            gridLineColor,
            1.0f);

        float x = m_bounds.left - m_hScroll;
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

            StyleState itemState = StyleState::Normal;
            if (row == m_selectedRow) {
                itemState = StyleState::Selected;
            } else if (row == m_hoveredRow) {
                itemState = StyleState::Hover;
            }

            Color rowBg = (row % 2 == 0) ? rowBackgroundA : rowBackgroundB;
            Color rowFg = shellFg;
            float rowRadius = 0.0f;
            if (itemStyle) {
                const StyleSpec itemSpec = itemStyle->Resolve(itemState);
                if (itemSpec.decoration.has_value()) {
                    if (itemState != StyleState::Normal || itemSpec.decoration->background.a > 0.001f) {
                        rowBg = itemSpec.decoration->background;
                    }
                    if (!itemSpec.decoration->radius.IsZero()) {
                        rowRadius = ScaleValue(itemSpec.decoration->radius.MaxRadius());
                    }
                }
                if (itemSpec.foreground.has_value()) {
                    rowFg = *itemSpec.foreground;
                }
            } else {
                if (row == m_selectedRow) {
                    rowBg = selectedRowBackground;
                } else if (row == m_hoveredRow) {
                    rowBg = hoverRowBackground;
                }
            }

            if (rowBg.a > 0.001f) {
                if (rowRadius > 0.5f) {
                    renderer.FillRoundedRect(rowRect, rowBg, rowRadius);
                } else {
                    renderer.FillRect(rowRect, rowBg);
                }
            }

            float rowX = m_bounds.left - m_hScroll;
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
                    rowFg,
                    ScaleValue(shellFs),
                    textOptions);
                rowX += widths[col];
            }

            renderer.DrawLine(
                PointF{m_bounds.left, rowBottom},
                PointF{m_bounds.right, rowBottom},
                gridLineColor,
                1.0f);
        }

        PaintPillScrollbars(
            renderer,
            m_bounds,
            contentW,
            contentH,
            viewW,
            viewH,
            m_hScroll,
            static_cast<float>(m_scrollOffset) * scaledRowHeight,
            scrollTrackColor,
            scrollThumbColor,
            ScaleValue(8.0f));

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
    Color scrollThumbColor = ColorFromHex(0x7A93AD);
    Color scrollTrackColor = ColorFromHex(0x162433);
    float cornerRadius = 10.0f;
    float headerHeight = 30.0f;
    float rowHeight = 26.0f;
    float fontSize = 13.0f;
    float headerFontSize = 12.0f;
    Thickness cellPadding{8, 5, 8, 5};

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
                hoverRowBackground = hover.decoration->background;
            }
            if (selected.decoration.has_value()) {
                selectedRowBackground = selected.decoration->background;
            }
        }
    }

    bool ScrollHorizontal(float delta) {
        const float before = m_hScroll;
        m_hScroll += delta;
        const auto widths = ResolveColumnWidths();
        float contentW = 0.0f;
        for (float w : widths) {
            contentW += w;
        }
        ClampHorizontalScroll(contentW, std::max(1.0f, m_bounds.Width()));
        return m_hScroll != before;
    }

    void ClampHorizontalScroll(float contentW, float viewW) {
        const float maxScroll = std::max(0.0f, contentW - viewW);
        m_hScroll = std::clamp(m_hScroll, 0.0f, maxScroll);
    }

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

        // Keep natural column widths; horizontal pill scrollbar handles overflow.
        (void)totalWidth;
        return widths;
    }

    std::vector<Column> m_columns;
    std::vector<std::vector<std::wstring>> m_rows;
    int m_selectedRow = -1;
    int m_hoveredRow = -1;
    int m_scrollOffset = 0;
    float m_hScroll = 0.0f;
};

class TreeView : public UIElement {
public:
    TreeView() { m_style = DefaultStyle(nullptr); }

    static ComponentStyle DefaultStyle(const Theme* theme = nullptr) {
        ComponentStyle s;
        BoxDecoration shell;
        shell.background = ComponentStyle::ThemeColor(theme, "surface-1", ColorFromHex(0x101B28));
        shell.border.width = Thickness{1, 1, 1, 1};
        shell.border.color = ComponentStyle::ThemeColor(theme, "border-subtle", ColorFromHex(0x35506A));
        shell.radius = CornerRadius::Uniform(
            ComponentStyle::ThemeNumber(theme, Theme::NumberCategory::Radius, "lg", 10.0f));
        s.base.decoration = shell;
        s.base.foreground = ComponentStyle::ThemeColor(theme, "fg", ColorFromHex(0xDCE8F5));
        s.base.fontSize = ComponentStyle::ThemeNumber(theme, Theme::NumberCategory::FontSize, "sm", 13.0f);

        BoxDecoration focused = shell;
        focused.border.width = Thickness{2, 2, 2, 2};
        focused.border.color = ComponentStyle::ThemeColor(theme, "border-focus", ColorFromHex(0xFFFFFF));
        s.overrides[static_cast<std::size_t>(StyleState::Focused)].decoration = focused;

        auto item = std::make_unique<ComponentStyle>();
        item->base.decoration = BoxDecoration{};
        item->base.decoration->background = Color{0, 0, 0, 0};
        item->base.decoration->radius = CornerRadius::Uniform(4.0f);
        item->base.foreground = ComponentStyle::ThemeColor(theme, "fg", ColorFromHex(0xDCE8F5));
        BoxDecoration hover;
        hover.background = ComponentStyle::ThemeColor(theme, "surface-3", ColorFromHex(0x1F3B55));
        hover.radius = CornerRadius::Uniform(4.0f);
        item->overrides[static_cast<std::size_t>(StyleState::Hover)].decoration = hover;
        BoxDecoration selected;
        selected.background = ComponentStyle::ThemeColor(theme, "accent-soft", ColorFromHex(0x2E577D));
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

    bool OnMouseWheel(float delta, float x, float y, bool /*shiftHeld*/) override {
        if (!HitTest(x, y)) {
            return false;
        }
        auto visible = BuildVisibleNodes();
        if (visible.empty()) {
            return false;
        }
        const int before = m_scrollOffset;
        const int visibleRows = VisibleRowCount(static_cast<int>(visible.size()));
        m_scrollOffset = std::clamp(
            m_scrollOffset - static_cast<int>(std::lround(delta * 3.0f)),
            0,
            std::max(0, static_cast<int>(visible.size()) - visibleRows));
        return m_scrollOffset != before;
    }

protected:
    float MeasurePreferredWidth(float availableWidth) const override {
        float preferred = ScaleValue(320.0f);
        const float maxWidth = std::max(availableWidth, preferred);
        return std::min(availableWidth, maxWidth);
    }

    float MeasurePreferredHeight(float width) const override {
        (void)width;
        return ScaleValue(200.0f);
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

        renderer.FillRoundedRect(m_bounds, shellBg, scaledCornerRadius);
        renderer.DrawRoundedRect(m_bounds, shellBorder, 1.0f, scaledCornerRadius);
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
                ScaleValue(shellFs),
                textOptions);
            return;
        }

        const Rect content = ContentRect();
        const float row = std::max(1.0f, ScaleValue(itemHeight));
        const int visibleRows = std::max(1, static_cast<int>(std::floor(content.Height() / row)));
        const int maxStart = std::max(0, static_cast<int>(visible.size()) - visibleRows);
        m_scrollOffset = std::clamp(m_scrollOffset, 0, maxStart);
        const int count = std::min(visibleRows, static_cast<int>(visible.size()) - m_scrollOffset);
        const ComponentStyle* itemStyle = m_style.itemStyle ? m_style.itemStyle.get() : nullptr;

        renderer.PushRoundedClip(m_bounds, scaledCornerRadius);
        for (int i = 0; i < count; ++i) {
            const int globalIndex = m_scrollOffset + i;
            const auto& entry = visible[static_cast<size_t>(globalIndex)];
            const float top = content.top + static_cast<float>(i) * row;
            const float bottom = std::min(content.bottom, top + row);
            const Rect rowRect = Rect::Make(content.left, top, content.right, bottom);

            const bool selected = entry.path == m_selectedPath;
            const bool hovered = m_hasHoveredPath && entry.path == m_hoveredPath;
            StyleState itemState = StyleState::Normal;
            if (selected) {
                itemState = StyleState::Selected;
            } else if (hovered) {
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
                if (selected) {
                    rowBg = selectedBackground;
                } else if (hovered) {
                    rowBg = hoverBackground;
                }
            }
            if (rowBg.a > 0.001f) {
                renderer.FillRoundedRect(rowRect, rowBg, rowRadius);
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
                rowFg,
                ScaleValue(shellFs),
                textOptions);
        }

        const float contentH = row * static_cast<float>(std::max<size_t>(1, visible.size()));
        PaintPillScrollbars(
            renderer,
            m_bounds,
            content.Width(),
            contentH,
            content.Width(),
            content.Height(),
            0.0f,
            static_cast<float>(m_scrollOffset) * row,
            scrollTrackColor,
            scrollThumbColor,
            ScaleValue(8.0f));

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
    Color scrollThumbColor = ColorFromHex(0x7A93AD);
    Color scrollTrackColor = ColorFromHex(0x162433);
    float cornerRadius = 10.0f;
    float fontSize = 13.0f;
    float itemHeight = 24.0f;
    float indentWidth = 18.0f;

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

struct StripItem {
    std::wstring text;
    std::vector<uint8_t> svgData;
    SvgHandle svg = nullptr;
    Size intrinsicSize{16.0f, 16.0f};

    bool HasIcon() const { return !svgData.empty(); }
};

inline void EnsureStripItemSvg(IRenderer& renderer, StripItem& item) {
    if (!item.svg && !item.svgData.empty()) {
        item.svg = renderer.CreateSvgFromBytes(item.svgData.data(), item.svgData.size());
        if (item.svg) {
            item.intrinsicSize = renderer.GetSvgSize(item.svg);
        }
    }
}

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
