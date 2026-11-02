#pragma once
// Modern composite containers (Modern Components v2):
// Expander (collapsible accordion card) & SplitView (interactive resizable two-pane container)

#include "ui_widgets.h"

#include <algorithm>
#include <cmath>
#include <string>

class Expander : public UIElement {
public:
    Expander() = default;

    std::wstring title = L"Section";
    std::wstring subtitle;
    std::string icon;
    bool expanded = true;
    float headerHeight = 42.0f;
    float cornerRadius = 10.0f;
    float spacing = 8.0f;
    Thickness contentPadding{14.0f, 12.0f, 14.0f, 14.0f};
    Color background = ColorFromHex(0xFFFFFF);
    Color headerBackground = ColorFromHex(0xF8FAFC);
    Color borderColor = ColorFromHex(0xE6ECF5);
    Color titleColor = ColorFromHex(0x181D27);
    Color subtitleColor = ColorFromHex(0x697586);

    bool IsLayoutLeaf() const override { return true; }

protected:
    float MeasurePreferredWidth(float availableWidth) const override {
        return availableWidth > 0.0f ? availableWidth : ScaleValue(320.0f);
    }

    float MeasurePreferredHeight(float width) const override {
        const float hdrH = ScaleValue(headerHeight);
        if (!expanded || m_children.empty()) {
            return hdrH;
        }
        const Thickness pad = ScaleThickness(contentPadding);
        const float innerW = std::max(1.0f, width - pad.left - pad.right);
        const float gap = ScaleValue(spacing);
        float totalH = hdrH + pad.top + pad.bottom;
        bool first = true;
        for (const auto& child : m_children) {
            if (!child) continue;
            child->Measure(innerW, 4096.0f);
            if (!first) totalH += gap;
            totalH += child->DesiredSize().height;
            first = false;
        }
        return totalH;
    }

public:
    void Measure(float availableWidth, float /*availableHeight*/) override {
        const float width = GetPreferredWidth(availableWidth);
        m_desiredSize.width = width;
        if (HasFixedHeight()) {
            m_desiredSize.height = GetPreferredHeight(width);
        } else {
            m_desiredSize.height = ClampHeight(MeasurePreferredHeight(width));
        }
    }

    void Arrange(const Rect& finalRect) override {
        UIElement::Arrange(finalRect);
        LayoutChildren();
    }

    void LayoutChildren() {
        const float hdrH = ScaleValue(headerHeight);
        if (!expanded) {
            for (auto& child : m_children) {
                if (child) {
                    child->Arrange(Rect{m_bounds.left, m_bounds.top, m_bounds.left, m_bounds.top});
                }
            }
            return;
        }
        const Thickness pad = ScaleThickness(contentPadding);
        const float innerLeft = m_bounds.left + pad.left;
        const float innerRight = std::max(innerLeft, m_bounds.right - pad.right);
        const float innerW = std::max(1.0f, innerRight - innerLeft);
        const float gap = ScaleValue(spacing);
        float curY = m_bounds.top + hdrH + pad.top;
        for (auto& child : m_children) {
            if (!child) continue;
            child->Measure(innerW, 4096.0f);
            const float ch = child->DesiredSize().height;
            child->Arrange(Rect{innerLeft, curY, innerRight, curY + ch});
            curY += ch + gap;
        }
    }

    UIElement* FindHitElementAt(float x, float y) override {
        if (!HitTest(x, y)) return nullptr;
        const float hdrBottom = m_bounds.top + ScaleValue(headerHeight);
        if (y <= hdrBottom) {
            return this;
        }
        if (expanded) {
            for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
                if (*it && (*it)->HitTest(x, y)) {
                    if (UIElement* deep = (*it)->FindHitElementAt(x, y)) {
                        return deep;
                    }
                    return it->get();
                }
            }
        }
        return this;
    }

    bool OnMouseMove(float x, float y) override {
        bool changed = false;
        if (!m_tooltip.empty() && !m_hovered) {
            m_hovered = true;
            changed = true;
        }
        const float hdrBottom = m_bounds.top + ScaleValue(headerHeight);
        const bool inHdr = (y >= m_bounds.top && y <= hdrBottom);
        if (m_headerHovered != inHdr) {
            m_headerHovered = inHdr;
            m_cursor = inHdr ? CursorType::Hand : CursorType::Auto;
            changed = true;
        }
        if (expanded && !inHdr) {
            for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
                if (*it && (*it)->HitTest(x, y)) {
                    changed = (*it)->OnMouseMove(x, y) || changed;
                } else if (*it) {
                    changed = (*it)->OnMouseLeave() || changed;
                }
            }
        }
        return changed;
    }

    bool OnMouseLeave() override {
        bool changed = UIElement::OnMouseLeave();
        if (m_headerHovered) {
            m_headerHovered = false;
            changed = true;
        }
        return changed;
    }

    bool OnMouseDown(float x, float y) override {
        if (!HitTest(x, y)) return false;
        const float hdrBottom = m_bounds.top + ScaleValue(headerHeight);
        if (y <= hdrBottom) {
            expanded = !expanded;
            LayoutChildren();
            if (m_context && m_context->requestLayout) {
                m_context->requestLayout();
            }
            return true;
        }
        if (expanded) {
            for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
                if (*it && (*it)->HitTest(x, y) && (*it)->OnMouseDown(x, y)) {
                    return true;
                }
            }
        }
        return false;
    }

    void Render(IRenderer& renderer) override {
        if (!renderer.IntersectsDirtyRect(m_bounds)) {
            return;
        }
        const float r = ScaleValue(cornerRadius);
        const float hdrH = ScaleValue(headerHeight);
        renderer.FillRoundedRect(m_bounds, background, r);

        const Rect hdrRect{m_bounds.left, m_bounds.top, m_bounds.right, std::min(m_bounds.bottom, m_bounds.top + hdrH)};
        if (expanded) {
            CornerRadius topRadii{r, r, 0.0f, 0.0f};
            renderer.FillRRect(hdrRect, topRadii, m_headerHovered ? ColorFromHex(0xF1F5F9) : headerBackground);
            renderer.DrawLine(
                PointF{m_bounds.left, hdrRect.bottom},
                PointF{m_bounds.right, hdrRect.bottom},
                borderColor,
                ScaleValue(1.0f));
        } else {
            renderer.FillRoundedRect(hdrRect, m_headerHovered ? ColorFromHex(0xF1F5F9) : headerBackground, r);
        }
        renderer.DrawRoundedRect(m_bounds, borderColor, ScaleValue(1.0f), r);

        const float cy = (hdrRect.top + hdrRect.bottom) * 0.5f;
        float textLeft = hdrRect.left + ScaleValue(14.0f);
        if (!icon.empty()) {
            bool filled = false;
            if (const char* pathData = VectorIcon::LookupIconPath(icon, filled)) {
                const float isz = ScaleValue(16.0f);
                renderer.DrawSvgPath(
                    pathData,
                    24.0f,
                    24.0f,
                    Rect{textLeft, cy - isz * 0.5f, textLeft + isz, cy + isz * 0.5f},
                    Color{0.0f, 0.0f, 0.0f, 0.0f},
                    ColorFromHex(0x2764FF),
                    ScaleValue(2.0f));
                textLeft += isz + ScaleValue(10.0f);
            }
        }

        const float chevSz = ScaleValue(15.0f);
        const float chevLeft = hdrRect.right - ScaleValue(14.0f) - chevSz;
        bool chevFill = false;
        if (const char* chevPath = VectorIcon::LookupIconPath(expanded ? "chevron-down" : "chevron-right", chevFill)) {
            renderer.DrawSvgPath(
                chevPath,
                24.0f,
                24.0f,
                Rect{chevLeft, cy - chevSz * 0.5f, chevLeft + chevSz, cy + chevSz * 0.5f},
                Color{0.0f, 0.0f, 0.0f, 0.0f},
                subtitleColor,
                ScaleValue(2.0f));
        }

        DrawStyledText(
            renderer,
            title,
            Rect{textLeft, hdrRect.top, chevLeft - ScaleValue(8.0f), hdrRect.bottom},
            ScaleValue(13.0f),
            titleColor,
            TextHorizontalAlign::Start,
            TextVerticalAlign::Center,
            600);

        if (!subtitle.empty()) {
            DrawStyledText(
                renderer,
                subtitle,
                Rect{textLeft + ScaleValue(120.0f), hdrRect.top, chevLeft - ScaleValue(10.0f), hdrRect.bottom},
                ScaleValue(11.5f),
                subtitleColor,
                TextHorizontalAlign::End,
                TextVerticalAlign::Center,
                500);
        }

        if (expanded) {
            LayoutChildren();
            for (auto& child : m_children) {
                if (child) child->Render(renderer);
            }
        }
    }

private:
    bool m_headerHovered = false;
};

class SplitView : public UIElement {
public:
    SplitView() = default;

    bool vertical = false; // false = left/right panes, true = top/bottom panes
    float splitRatio = 0.36f;
    float splitterSize = 8.0f;
    float minPaneSize = 70.0f;
    Color splitterColor = ColorFromHex(0xF1F5F9);
    Color splitterHoverColor = ColorFromHex(0xDCE7FF);
    Color handleColor = ColorFromHex(0x94A3B8);

    bool IsLayoutLeaf() const override { return true; }

protected:
    float MeasurePreferredWidth(float availableWidth) const override {
        return availableWidth > 0.0f ? availableWidth : ScaleValue(420.0f);
    }

    float MeasurePreferredHeight(float /*width*/) const override {
        return ScaleValue(160.0f);
    }

public:
    void Measure(float availableWidth, float availableHeight) override {
        const float width = GetPreferredWidth(availableWidth);
        const float height = HasFixedHeight()
            ? GetPreferredHeight(width)
            : (availableHeight > 0.0f && availableHeight < 10000.0f ? availableHeight : ScaleValue(160.0f));
        m_desiredSize = Size{width, ClampHeight(height)};
    }

    void Arrange(const Rect& finalRect) override {
        UIElement::Arrange(finalRect);
        LayoutPanes();
    }

    Rect SplitterRect() const {
        const float totalW = std::max(0.0f, m_bounds.right - m_bounds.left);
        const float totalH = std::max(0.0f, m_bounds.bottom - m_bounds.top);
        const float bar = ScaleValue(splitterSize);
        const float ratio = std::clamp(splitRatio, 0.12f, 0.88f);
        if (!vertical) {
            const float avail = std::max(0.0f, totalW - bar);
            const float firstW = avail * ratio;
            return Rect{m_bounds.left + firstW, m_bounds.top, m_bounds.left + firstW + bar, m_bounds.bottom};
        } else {
            const float avail = std::max(0.0f, totalH - bar);
            const float firstH = avail * ratio;
            return Rect{m_bounds.left, m_bounds.top + firstH, m_bounds.right, m_bounds.top + firstH + bar};
        }
    }

    void LayoutPanes() {
        const Rect sp = SplitterRect();
        if (!vertical) {
            const Rect firstRect{m_bounds.left, m_bounds.top, sp.left, m_bounds.bottom};
            const Rect secondRect{sp.right, m_bounds.top, m_bounds.right, m_bounds.bottom};
            if (m_children.size() >= 1 && m_children[0]) {
                m_children[0]->Measure(firstRect.Width(), firstRect.Height());
                m_children[0]->Arrange(firstRect);
            }
            if (m_children.size() >= 2 && m_children[1]) {
                m_children[1]->Measure(secondRect.Width(), secondRect.Height());
                m_children[1]->Arrange(secondRect);
            }
        } else {
            const Rect firstRect{m_bounds.left, m_bounds.top, m_bounds.right, sp.top};
            const Rect secondRect{m_bounds.left, sp.bottom, m_bounds.right, m_bounds.bottom};
            if (m_children.size() >= 1 && m_children[0]) {
                m_children[0]->Measure(firstRect.Width(), firstRect.Height());
                m_children[0]->Arrange(firstRect);
            }
            if (m_children.size() >= 2 && m_children[1]) {
                m_children[1]->Measure(secondRect.Width(), secondRect.Height());
                m_children[1]->Arrange(secondRect);
            }
        }
    }

    bool HitSplitter(float x, float y) const {
        const Rect sp = SplitterRect();
        return x >= sp.left - 2.0f && x <= sp.right + 2.0f && y >= sp.top - 2.0f && y <= sp.bottom + 2.0f;
    }

    UIElement* FindHitElementAt(float x, float y) override {
        if (!HitTest(x, y)) return nullptr;
        if (HitSplitter(x, y)) return this;
        for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
            if (*it && (*it)->HitTest(x, y)) {
                if (UIElement* deep = (*it)->FindHitElementAt(x, y)) {
                    return deep;
                }
                return it->get();
            }
        }
        return this;
    }

    bool OnMouseDown(float x, float y) override {
        if (!HitTest(x, y)) return false;
        if (HitSplitter(x, y)) {
            m_dragging = true;
            return true;
        }
        for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
            if (*it && (*it)->HitTest(x, y) && (*it)->OnMouseDown(x, y)) {
                return true;
            }
        }
        return false;
    }

    bool OnMouseMove(float x, float y) override {
        bool changed = false;
        if (m_dragging) {
            const float minPx = ScaleValue(minPaneSize);
            if (!vertical) {
                const float total = std::max(1.0f, m_bounds.Width() - ScaleValue(splitterSize));
                const float rel = std::clamp(x - m_bounds.left - ScaleValue(splitterSize) * 0.5f, minPx, std::max(minPx, total - minPx));
                splitRatio = std::clamp(rel / total, 0.12f, 0.88f);
            } else {
                const float total = std::max(1.0f, m_bounds.Height() - ScaleValue(splitterSize));
                const float rel = std::clamp(y - m_bounds.top - ScaleValue(splitterSize) * 0.5f, minPx, std::max(minPx, total - minPx));
                splitRatio = std::clamp(rel / total, 0.12f, 0.88f);
            }
            LayoutPanes();
            return true;
        }
        const bool overSp = HitSplitter(x, y);
        if (m_splitterHovered != overSp) {
            m_splitterHovered = overSp;
            m_cursor = overSp ? (vertical ? CursorType::SizeNS : CursorType::SizeWE) : CursorType::Auto;
            changed = true;
        }
        for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
            if (*it && (*it)->HitTest(x, y)) {
                changed = (*it)->OnMouseMove(x, y) || changed;
            } else if (*it) {
                changed = (*it)->OnMouseLeave() || changed;
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
        for (auto& child : m_children) {
            if (child) changed = child->OnMouseUp(x, y) || changed;
        }
        return changed;
    }

    bool OnMouseLeave() override {
        bool changed = UIElement::OnMouseLeave();
        if (m_splitterHovered && !m_dragging) {
            m_splitterHovered = false;
            m_cursor = CursorType::Auto;
            changed = true;
        }
        return changed;
    }

    void Render(IRenderer& renderer) override {
        if (!renderer.IntersectsDirtyRect(m_bounds)) {
            return;
        }
        LayoutPanes();
        if (m_children.size() >= 1 && m_children[0]) {
            m_children[0]->Render(renderer);
        }
        if (m_children.size() >= 2 && m_children[1]) {
            m_children[1]->Render(renderer);
        }

        const Rect sp = SplitterRect();
        const float r = ScaleValue(4.0f);
        renderer.FillRoundedRect(sp, (m_splitterHovered || m_dragging) ? splitterHoverColor : splitterColor, r);

        const float cx = (sp.left + sp.right) * 0.5f;
        const float cy = (sp.top + sp.bottom) * 0.5f;
        const float hw = vertical ? ScaleValue(24.0f) : ScaleValue(2.5f);
        const float hh = vertical ? ScaleValue(2.5f) : ScaleValue(24.0f);
        const Rect grip{cx - hw * 0.5f, cy - hh * 0.5f, cx + hw * 0.5f, cy + hh * 0.5f};
        renderer.FillRoundedRect(grip, (m_splitterHovered || m_dragging) ? ColorFromHex(0x2764FF) : handleColor, ScaleValue(1.5f));
    }

private:
    bool m_splitterHovered = false;
    bool m_dragging = false;
};
