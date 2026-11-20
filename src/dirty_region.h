#pragma once

#include "graphics_types.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <vector>

namespace ai_win_ui {

// Manages up to kMaxRects disjoint/merged dirty rectangles for partial software rasterization (Skia).
// Adjacent small dirty cells (waste <= kAdjacentMergeWasteArea) coalesce into a tight band, while
// distant UI corners (e.g. top-right quotes + bottom-left status bar) remain independent regions.
class DirtyRegion {
public:
    static constexpr size_t kMaxRects = 8;
    static constexpr float kAdjacentMergeWasteArea = 2048.0f;
    static constexpr float kFullRepaintAreaRatio = 0.85f;

    void Clear() {
        m_rects.clear();
        m_bounds = Rect{};
        m_fullDirty = false;
    }

    void SetFull() {
        m_rects.clear();
        m_bounds = Rect{};
        m_fullDirty = true;
    }

    bool IsEmpty() const {
        return !m_fullDirty && m_rects.empty();
    }

    bool IsFull() const {
        return m_fullDirty;
    }

    const std::vector<Rect>& Rects() const {
        return m_rects;
    }

    Rect Bounds() const {
        return m_bounds;
    }

    void AddRect(const Rect& rect) {
        if (m_fullDirty || !IsValidRect(rect)) {
            return;
        }

        // Remove any existing rects strictly contained by the new rect; skip if already covered.
        for (size_t i = 0; i < m_rects.size();) {
            if (ContainsRect(m_rects[i], rect)) {
                return;
            }
            if (ContainsRect(rect, m_rects[i])) {
                m_rects.erase(m_rects.begin() + static_cast<std::ptrdiff_t>(i));
                continue;
            }
            ++i;
        }

        m_rects.push_back(rect);
        CompactNearbyRects(kAdjacentMergeWasteArea);
        EnforceMaxRects();
        RecomputeBounds();
    }

    void NormalizeForViewport(float viewportWidth, float viewportHeight) {
        if (viewportWidth <= 0.0f || viewportHeight <= 0.0f || m_fullDirty) {
            return;
        }

        std::vector<Rect> snapped;
        snapped.reserve(m_rects.size());
        for (const Rect& r : m_rects) {
            const float l = std::max(0.0f, std::floor(std::max(0.0f, r.left)));
            const float t = std::max(0.0f, std::floor(std::max(0.0f, r.top)));
            const float ri = std::min(viewportWidth, std::ceil(std::min(viewportWidth, r.right)));
            const float b = std::min(viewportHeight, std::ceil(std::min(viewportHeight, r.bottom)));
            if (ri > l && b > t) {
                snapped.push_back(Rect::Make(l, t, ri, b));
            }
        }
        m_rects = std::move(snapped);

        CompactNearbyRects(kAdjacentMergeWasteArea);
        EnforceMaxRects();

        if (m_rects.empty()) {
            m_bounds = Rect{};
            return;
        }

        const float viewportArea = viewportWidth * viewportHeight;
        for (const Rect& r : m_rects) {
            if (r.Width() >= viewportWidth - 2.0f && r.Height() >= viewportHeight - 2.0f) {
                SetFull();
                return;
            }
        }
        const float sumArea = TotalArea();
        if (viewportArea > 0.0f && sumArea >= viewportArea * kFullRepaintAreaRatio) {
            SetFull();
            return;
        }

        RecomputeBounds();
    }

    bool Intersects(const Rect& rect) const {
        if (m_fullDirty) {
            return true;
        }
        if (m_rects.empty()) {
            return false;
        }
        if (!RectsIntersect(rect, m_bounds)) {
            return false;
        }
        if (m_rects.size() == 1) {
            return true;
        }
        for (const Rect& r : m_rects) {
            if (RectsIntersect(rect, r)) {
                return true;
            }
        }
        return false;
    }

    float TotalArea() const {
        float total = 0.0f;
        for (const Rect& r : m_rects) {
            total += RectArea(r);
        }
        return total;
    }

    float CoverageRatio(float viewportWidth, float viewportHeight) const {
        if (viewportWidth <= 0.0f || viewportHeight <= 0.0f) {
            return 0.0f;
        }
        if (m_fullDirty) {
            return 1.0f;
        }
        if (m_rects.empty()) {
            return 0.0f;
        }
        return std::clamp(TotalArea() / (viewportWidth * viewportHeight), 0.0f, 1.0f);
    }

private:
    static bool IsValidRect(const Rect& r) {
        return r.right > r.left && r.bottom > r.top;
    }

    static float RectArea(const Rect& r) {
        return std::max(0.0f, r.right - r.left) * std::max(0.0f, r.bottom - r.top);
    }

    static Rect UnionRect(const Rect& a, const Rect& b) {
        return Rect::Make(
            std::min(a.left, b.left),
            std::min(a.top, b.top),
            std::max(a.right, b.right),
            std::max(a.bottom, b.bottom));
    }

    static bool ContainsRect(const Rect& outer, const Rect& inner) {
        return outer.left <= inner.left && outer.top <= inner.top &&
               outer.right >= inner.right && outer.bottom >= inner.bottom;
    }

    static bool RectsIntersect(const Rect& a, const Rect& b) {
        return !(a.right < b.left || a.left > b.right ||
                 a.bottom < b.top || a.top > b.bottom);
    }

    static float MergeWaste(const Rect& a, const Rect& b) {
        const Rect u = UnionRect(a, b);
        return RectArea(u) - (RectArea(a) + RectArea(b));
    }

    void CompactNearbyRects(float maxWasteArea) {
        bool merged = true;
        while (merged && m_rects.size() > 1) {
            merged = false;

            // Prune strict containments first.
            for (size_t i = 0; i < m_rects.size(); ++i) {
                for (size_t j = i + 1; j < m_rects.size();) {
                    if (ContainsRect(m_rects[i], m_rects[j])) {
                        m_rects.erase(m_rects.begin() + static_cast<std::ptrdiff_t>(j));
                        merged = true;
                        continue;
                    }
                    if (ContainsRect(m_rects[j], m_rects[i])) {
                        m_rects[i] = m_rects[j];
                        m_rects.erase(m_rects.begin() + static_cast<std::ptrdiff_t>(j));
                        merged = true;
                        continue;
                    }
                    ++j;
                }
            }

            if (m_rects.size() <= 1) {
                break;
            }

            size_t bestI = 0;
            size_t bestJ = 0;
            float bestWaste = maxWasteArea;
            bool found = false;
            for (size_t i = 0; i < m_rects.size(); ++i) {
                for (size_t j = i + 1; j < m_rects.size(); ++j) {
                    const float waste = MergeWaste(m_rects[i], m_rects[j]);
                    if (waste <= bestWaste) {
                        bestWaste = waste;
                        bestI = i;
                        bestJ = j;
                        found = true;
                    }
                }
            }
            if (found) {
                m_rects[bestI] = UnionRect(m_rects[bestI], m_rects[bestJ]);
                m_rects.erase(m_rects.begin() + static_cast<std::ptrdiff_t>(bestJ));
                merged = true;
            }
        }
    }

    void EnforceMaxRects() {
        while (m_rects.size() > kMaxRects) {
            size_t bestI = 0;
            size_t bestJ = 1;
            float bestWaste = std::numeric_limits<float>::max();
            for (size_t i = 0; i < m_rects.size(); ++i) {
                for (size_t j = i + 1; j < m_rects.size(); ++j) {
                    const float waste = MergeWaste(m_rects[i], m_rects[j]);
                    if (waste < bestWaste) {
                        bestWaste = waste;
                        bestI = i;
                        bestJ = j;
                    }
                }
            }
            m_rects[bestI] = UnionRect(m_rects[bestI], m_rects[bestJ]);
            m_rects.erase(m_rects.begin() + static_cast<std::ptrdiff_t>(bestJ));
        }
    }

    void RecomputeBounds() {
        if (m_rects.empty()) {
            m_bounds = Rect{};
            return;
        }
        Rect b = m_rects.front();
        for (size_t i = 1; i < m_rects.size(); ++i) {
            b = UnionRect(b, m_rects[i]);
        }
        m_bounds = b;
    }

    std::vector<Rect> m_rects;
    Rect m_bounds{};
    bool m_fullDirty = false;
};

} // namespace ai_win_ui

