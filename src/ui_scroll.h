#pragma once
// ScrollViewer
// See ui.h for the full umbrella include.
#include "ui_nav.h"

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

    UIElement* FindDropTargetAt(float x, float y) override {
        if (!UIElement::HitTest(x, y)) return nullptr;
        // Children are arranged at (bounds - scrollOffset), so their own HitTest
        // already accounts for the offset. We just recurse with the same x,y.
        for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
            if (auto* target = (*it)->FindDropTargetAt(x, y)) {
                return target;
            }
        }
        return m_allowDrop ? this : nullptr;
    }

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

        // Focusable children (buttons, inputs) keep their clicks 鈥?no pan steal.
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
            // Drag content with pointer (natural: pull content up 鈫?increase scrollY).
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
    bool HitTest(float x, float y) const override {
        if (m_drag != DragMode::None) {
            return true; // Implicit capture while dragging
        }
        return UIElement::HitTest(x, y);
    }

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

