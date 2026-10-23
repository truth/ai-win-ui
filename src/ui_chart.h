#pragma once
// Chart: multi-series line / bar / area chart with axes, grid, legend and hover tooltip.
// See ui.h for the full umbrella include. Rendering uses IRenderer primitives only.
#include "ui_element.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cwchar>
#include <limits>
#include <string>
#include <utility>
#include <vector>

struct ChartSeries {
    enum class Type { Line, Bar, Area };

    std::wstring name;
    Type type = Type::Line;
    std::vector<float> values;
    // Optional numeric X coordinates (Line/Area only). Empty => category index.
    std::vector<float> xs;
    Color color{};
    bool hasColor = false;
    bool visible = true;
};

class Chart : public UIElement {
public:
    Chart() = default;

    // ---- data -------------------------------------------------------------
    void AddSeries(ChartSeries series) {
        m_series.push_back(std::move(series));
        UpdateSummary();
    }
    size_t SeriesCount() const { return m_series.size(); }
    ChartSeries* SeriesAt(size_t index) {
        return index < m_series.size() ? &m_series[index] : nullptr;
    }
    bool SetSeriesValues(size_t index, std::vector<float> values) {
        if (index >= m_series.size()) return false;
        m_series[index].values = std::move(values);
        UpdateSummary();
        return true;
    }
    bool SetSeriesVisible(size_t index, bool visible) {
        if (index >= m_series.size()) return false;
        m_series[index].visible = visible;
        return true;
    }
    void SetCategories(std::vector<std::wstring> categories) {
        m_categories = std::move(categories);
    }
    void SetYRange(float minValue, float maxValue) {
        m_manualRange = true;
        m_yMin = minValue;
        m_yMax = maxValue;
    }
    void ClearYRange() { m_manualRange = false; }

    // ---- appearance -------------------------------------------------------
    Color background = ColorFromHex(0x121B25);
    Color borderColor = ColorFromHex(0x2C3A47);
    Color gridColor = ColorFromHex(0x233140);
    Color axisColor = ColorFromHex(0x4A5C6E);
    Color textColor = ColorFromHex(0x9FB0C0);
    Color tooltipBackground = ColorFromHex(0x1D2A38);
    float cornerRadius = 10.0f;
    float strokeWidth = 2.0f;
    float fontSize = 12.0f;
    bool showLegend = true;
    bool showGrid = true;
    bool showTooltip = true;

protected:
    float MeasurePreferredWidth(float availableWidth) const override {
        return std::min(availableWidth, ScaleValue(360.0f));
    }
    float MeasurePreferredHeight(float width) const override {
        (void)width;
        return ScaleValue(220.0f);
    }

public:
    // ---- rendering --------------------------------------------------------
    void Render(IRenderer& renderer) override {
        if (!renderer.IntersectsDirtyRect(m_bounds)) return;

        const float radius = ScaleValue(cornerRadius);
        renderer.FillRoundedRect(m_bounds, background, radius);
        renderer.DrawRoundedRect(m_bounds, borderColor, 1.0f, radius);

        BuildLayout();
        DrawLegend(renderer);

        if (m_plot.Width() <= 2.0f || m_plot.Height() <= 2.0f) return;

        DrawYAxisAndGrid(renderer);
        DrawXAxis(renderer);

        // Areas first, then bars, then lines so lines stay readable.
        for (size_t i = 0; i < m_series.size(); ++i) {
            if (m_series[i].visible && m_series[i].type == ChartSeries::Type::Area) DrawArea(renderer, i);
        }
        DrawBars(renderer);
        for (size_t i = 0; i < m_series.size(); ++i) {
            const auto t = m_series[i].type;
            if (m_series[i].visible && (t == ChartSeries::Type::Line || t == ChartSeries::Type::Area)) {
                DrawLine(renderer, i);
            }
        }

        if (m_hoverIndex >= 0 && showTooltip) {
            const float x = PointX(m_hoverSeries, static_cast<size_t>(m_hoverIndex));
            renderer.DrawLine(PointF{x, m_plot.top}, PointF{x, m_plot.bottom}, axisColor, 1.0f);
        }
    }

    void RenderOverlay(IRenderer& renderer) override {
        if (m_hoverIndex < 0 || !showTooltip) return;
        DrawTooltip(renderer);
    }

    // ---- input ------------------------------------------------------------
    bool OnMouseMove(float x, float y) override {
        int newIndex = -1;
        if (x >= m_plot.left && x <= m_plot.right && y >= m_plot.top && y <= m_plot.bottom) {
            newIndex = NearestIndex(x);
        }
        const bool changed = newIndex != m_hoverIndex ||
            (newIndex >= 0 && (x != m_mouseX || y != m_mouseY));
        m_hoverIndex = newIndex;
        m_mouseX = x;
        m_mouseY = y;
        return changed;
    }

    bool OnMouseLeave() override {
        const bool changed = m_hoverIndex >= 0;
        m_hoverIndex = -1;
        return changed;
    }

    bool OnMouseDown(float x, float y) override {
        for (size_t i = 0; i < m_legendRects.size(); ++i) {
            const Rect& r = m_legendRects[i];
            if (x >= r.left && x <= r.right && y >= r.top && y <= r.bottom) {
                m_series[i].visible = !m_series[i].visible;
                m_hoverIndex = -1;
                return true;
            }
        }
        return false;
    }

    // ---- helpers exposed for tests / parser ------------------------------
    // Nice 1/2/5 x 10^n scale covering [lo, hi]. Returns step.
    static float NiceScale(float lo, float hi, int targetTicks, float& niceLo, float& niceHi) {
        if (!(hi > lo)) {
            hi = lo + 1.0f;
        }
        const float range = NiceNumber(hi - lo, false);
        const float step = NiceNumber(range / static_cast<float>(std::max(2, targetTicks - 1)), true);
        niceLo = std::floor(lo / step) * step;
        niceHi = std::ceil(hi / step) * step;
        return step;
    }

    static Color DefaultPaletteColor(size_t index) {
        static const unsigned palette[] = {
            0x53B3FF, 0xFFB454, 0x5FD38D, 0xFF6B81, 0xB48CFF, 0x4FD1C5};
        return ColorFromHex(palette[index % (sizeof(palette) / sizeof(palette[0]))]);
    }

private:
    static float NiceNumber(float x, bool round) {
        if (x <= 0.0f) return 1.0f;
        const float exp10 = std::floor(std::log10(x));
        const float f = x / std::pow(10.0f, exp10);
        float nf;
        if (round) {
            nf = f < 1.5f ? 1.0f : f < 3.0f ? 2.0f : f < 7.0f ? 5.0f : 10.0f;
        } else {
            nf = f <= 1.0f ? 1.0f : f <= 2.0f ? 2.0f : f <= 5.0f ? 5.0f : 10.0f;
        }
        return nf * std::pow(10.0f, exp10);
    }

    static std::wstring FormatNumber(float value, float step) {
        int decimals = 0;
        if (step > 0.0f && step < 1.0f) {
            decimals = std::min(6, static_cast<int>(std::ceil(-std::log10(step))));
        }
        wchar_t buf[48];
        swprintf(buf, 48, L"%.*f", decimals, static_cast<double>(value));
        return buf;
    }

    Color SeriesColor(size_t i) const {
        return m_series[i].hasColor ? m_series[i].color : DefaultPaletteColor(i);
    }

    size_t MaxPointCount() const {
        size_t n = m_categories.size();
        for (const auto& s : m_series) n = std::max(n, s.values.size());
        return n;
    }

    bool UseNumericX() const {
        if (!m_categories.empty()) return false;
        bool any = false;
        for (const auto& s : m_series) {
            if (s.type == ChartSeries::Type::Bar) return false;
            if (s.visible && !s.xs.empty()) any = true;
        }
        return any;
    }

    void UpdateSummary() {
        size_t points = 0;
        for (const auto& s : m_series) points += s.values.size();
        m_uiaHelpText = "Chart: " + std::to_string(m_series.size()) + " series, " +
                        std::to_string(points) + " points";
    }

    float TextW(const std::wstring& text) const {
        return MeasureTextValue(text, fontSize, 10000.0f, TextWrapMode::NoWrap).width;
    }
    float TextH() const {
        return MeasureTextValue(L"Ag", fontSize, 10000.0f, TextWrapMode::NoWrap).height;
    }

    void DrawText(IRenderer& renderer, const std::wstring& text, const Rect& rect,
                  const Color& color, TextHorizontalAlign align) const {
        TextRenderOptions opts;
        opts.wrap = TextWrapMode::NoWrap;
        opts.horizontalAlign = align;
        opts.verticalAlign = TextVerticalAlign::Center;
        renderer.DrawTextW(text.c_str(), static_cast<UINT32>(text.size()), rect, color,
                           ScaleValue(fontSize), opts);
    }

    // Compute Y range, ticks, plot rect and legend rects from current bounds / data.
    void BuildLayout() {
        const float pad = ScaleValue(10.0f);
        const float th = TextH();
        const float gap = ScaleValue(6.0f);

        // Legend row.
        m_legendRects.clear();
        float top = m_bounds.top + pad;
        if (showLegend && !m_series.empty()) {
            float x = m_bounds.left + pad;
            const float sw = ScaleValue(10.0f);
            for (const auto& s : m_series) {
                const float w = sw + gap + TextW(s.name) + ScaleValue(14.0f);
                m_legendRects.push_back(Rect::Make(x, top, x + w, top + th + gap));
                x += w;
            }
            top += th + gap + ScaleValue(6.0f);
        }

        // Y range.
        float lo = std::numeric_limits<float>::max();
        float hi = std::numeric_limits<float>::lowest();
        bool zeroBase = false;
        for (const auto& s : m_series) {
            if (!s.visible) continue;
            if (s.type != ChartSeries::Type::Line) zeroBase = true;
            for (float v : s.values) {
                if (!std::isfinite(v)) continue;
                lo = std::min(lo, v);
                hi = std::max(hi, v);
            }
        }
        if (lo > hi) { lo = 0.0f; hi = 1.0f; }
        if (zeroBase) { lo = std::min(lo, 0.0f); hi = std::max(hi, 0.0f); }
        if (m_manualRange) { lo = m_yMin; hi = m_yMax; }
        m_yStep = NiceScale(lo, hi, 5, m_yLo, m_yHi);
        if (m_manualRange) { m_yLo = lo; m_yHi = hi; }

        // Widest Y label decides left gutter.
        float labelW = 0.0f;
        for (float v = m_yLo; v <= m_yHi + m_yStep * 0.001f; v += m_yStep) {
            labelW = std::max(labelW, TextW(FormatNumber(v, m_yStep)));
        }
        const float left = m_bounds.left + pad + labelW + gap;
        const float bottom = m_bounds.bottom - pad - th - gap;
        m_plot = Rect::Make(left, top, m_bounds.right - pad, bottom);

        // Numeric X scale.
        m_numericX = UseNumericX();
        if (m_numericX) {
            float xlo = std::numeric_limits<float>::max();
            float xhi = std::numeric_limits<float>::lowest();
            for (const auto& s : m_series) {
                if (!s.visible) continue;
                for (float x : s.xs) { xlo = std::min(xlo, x); xhi = std::max(xhi, x); }
            }
            if (xlo > xhi) { xlo = 0.0f; xhi = 1.0f; }
            m_xStep = NiceScale(xlo, xhi, 6, m_xLo, m_xHi);
        }
    }

    float ValueToY(float v) const {
        const float t = (m_yHi > m_yLo) ? (v - m_yLo) / (m_yHi - m_yLo) : 0.0f;
        return m_plot.bottom - std::clamp(t, 0.0f, 1.0f) * m_plot.Height();
    }

    float CategoryX(size_t i) const {
        const size_t n = std::max<size_t>(1, MaxPointCount());
        return m_plot.left + (static_cast<float>(i) + 0.5f) * m_plot.Width() / static_cast<float>(n);
    }

    float PointX(size_t seriesIndex, size_t i) const {
        if (m_numericX && seriesIndex < m_series.size() && i < m_series[seriesIndex].xs.size()) {
            const float t = (m_xHi > m_xLo) ? (m_series[seriesIndex].xs[i] - m_xLo) / (m_xHi - m_xLo) : 0.0f;
            return m_plot.left + std::clamp(t, 0.0f, 1.0f) * m_plot.Width();
        }
        return CategoryX(i);
    }

    int NearestIndex(float x) {
        m_hoverSeries = 0;
        for (size_t s = 0; s < m_series.size(); ++s) {
            if (m_series[s].visible && !m_series[s].values.empty()) { m_hoverSeries = s; break; }
        }
        if (m_series.empty()) return -1;
        const size_t n = m_series[m_hoverSeries].values.size();
        int best = -1;
        float bestDist = std::numeric_limits<float>::max();
        for (size_t i = 0; i < n; ++i) {
            const float d = std::abs(PointX(m_hoverSeries, i) - x);
            if (d < bestDist) { bestDist = d; best = static_cast<int>(i); }
        }
        return best;
    }

    void DrawLegend(IRenderer& renderer) const {
        const float sw = ScaleValue(10.0f);
        for (size_t i = 0; i < m_legendRects.size(); ++i) {
            const Rect& r = m_legendRects[i];
            const Color c = m_series[i].visible ? SeriesColor(i) : ColorFromHex(0x4A5C6E);
            const float cy = (r.top + r.bottom) * 0.5f;
            renderer.FillRoundedRect(Rect::Make(r.left, cy - sw * 0.5f, r.left + sw, cy + sw * 0.5f),
                                     c, ScaleValue(2.0f));
            DrawText(renderer, m_series[i].name,
                     Rect::Make(r.left + sw + ScaleValue(6.0f), r.top, r.right, r.bottom),
                     m_series[i].visible ? textColor : ColorFromHex(0x5F7183),
                     TextHorizontalAlign::Start);
        }
    }

    void DrawYAxisAndGrid(IRenderer& renderer) const {
        const float gap = ScaleValue(6.0f);
        const float th = TextH();
        for (float v = m_yLo; v <= m_yHi + m_yStep * 0.001f; v += m_yStep) {
            const float y = ValueToY(v);
            if (showGrid) {
                renderer.DrawLine(PointF{m_plot.left, y}, PointF{m_plot.right, y}, gridColor, 1.0f);
            }
            DrawText(renderer, FormatNumber(v, m_yStep),
                     Rect::Make(m_bounds.left, y - th * 0.5f, m_plot.left - gap, y + th * 0.5f),
                     textColor, TextHorizontalAlign::End);
        }
        renderer.DrawLine(PointF{m_plot.left, m_plot.top}, PointF{m_plot.left, m_plot.bottom}, axisColor, 1.0f);
    }

    void DrawXAxis(IRenderer& renderer) const {
        renderer.DrawLine(PointF{m_plot.left, m_plot.bottom}, PointF{m_plot.right, m_plot.bottom}, axisColor, 1.0f);
        const float th = TextH();
        const float top = m_plot.bottom + ScaleValue(4.0f);

        if (m_numericX) {
            for (float v = m_xLo; v <= m_xHi + m_xStep * 0.001f; v += m_xStep) {
                const float t = (m_xHi > m_xLo) ? (v - m_xLo) / (m_xHi - m_xLo) : 0.0f;
                const float x = m_plot.left + t * m_plot.Width();
                const std::wstring label = FormatNumber(v, m_xStep);
                const float w = TextW(label);
                DrawText(renderer, label, Rect::Make(x - w, top, x + w, top + th), textColor,
                         TextHorizontalAlign::Center);
            }
            return;
        }

        const size_t n = MaxPointCount();
        if (n == 0) return;
        float maxW = 0.0f;
        std::vector<std::wstring> labels(n);
        for (size_t i = 0; i < n; ++i) {
            labels[i] = i < m_categories.size() ? m_categories[i] : std::to_wstring(i + 1);
            maxW = std::max(maxW, TextW(labels[i]));
        }
        const float cell = m_plot.Width() / static_cast<float>(n);
        const size_t skip = std::max<size_t>(1, static_cast<size_t>(std::ceil((maxW + ScaleValue(8.0f)) / std::max(1.0f, cell))));
        for (size_t i = 0; i < n; i += skip) {
            const float x = CategoryX(i);
            DrawText(renderer, labels[i], Rect::Make(x - cell * skip * 0.5f, top, x + cell * skip * 0.5f, top + th),
                     textColor, TextHorizontalAlign::Center);
        }
    }

    std::vector<PointF> BuildPoints(size_t s) const {
        std::vector<PointF> pts;
        const auto& values = m_series[s].values;
        pts.reserve(values.size());
        for (size_t i = 0; i < values.size(); ++i) {
            if (!std::isfinite(values[i])) continue;
            pts.push_back(PointF{PointX(s, i), ValueToY(values[i])});
        }
        return pts;
    }

    void DrawLine(IRenderer& renderer, size_t s) const {
        const auto pts = BuildPoints(s);
        if (pts.empty()) return;
        const Color c = SeriesColor(s);
        if (pts.size() >= 2) {
            renderer.DrawPolyline(pts, c, ScaleValue(strokeWidth));
        }
        const float r = ScaleValue(3.0f);
        for (const auto& p : pts) {
            renderer.FillRoundedRect(Rect::Make(p.x - r, p.y - r, p.x + r, p.y + r), c, r);
        }
    }

    void DrawArea(IRenderer& renderer, size_t s) const {
        auto pts = BuildPoints(s);
        if (pts.size() < 2) return;
        const float zeroY = ValueToY(std::clamp(0.0f, m_yLo, m_yHi));
        std::vector<PointF> poly = pts;
        poly.push_back(PointF{pts.back().x, zeroY});
        poly.push_back(PointF{pts.front().x, zeroY});
        Color fill = SeriesColor(s);
        fill.a = 0.25f;
        renderer.FillPolygon(poly, fill);
    }

    void DrawBars(IRenderer& renderer) const {
        std::vector<size_t> bars;
        for (size_t i = 0; i < m_series.size(); ++i) {
            if (m_series[i].visible && m_series[i].type == ChartSeries::Type::Bar) bars.push_back(i);
        }
        if (bars.empty()) return;
        const size_t n = std::max<size_t>(1, MaxPointCount());
        const float cell = m_plot.Width() / static_cast<float>(n);
        const float group = cell * 0.7f;
        const float barW = group / static_cast<float>(bars.size());
        const float zeroY = ValueToY(std::clamp(0.0f, m_yLo, m_yHi));
        for (size_t b = 0; b < bars.size(); ++b) {
            const size_t s = bars[b];
            const Color c = SeriesColor(s);
            for (size_t i = 0; i < m_series[s].values.size(); ++i) {
                const float v = m_series[s].values[i];
                if (!std::isfinite(v)) continue;
                const float x0 = CategoryX(i) - group * 0.5f + static_cast<float>(b) * barW;
                const float y = ValueToY(v);
                renderer.FillRect(Rect::Make(x0 + 1.0f, std::min(y, zeroY), x0 + barW - 1.0f, std::max(y, zeroY)), c);
            }
        }
    }

    void DrawTooltip(IRenderer& renderer) const {
        const size_t idx = static_cast<size_t>(m_hoverIndex);
        std::wstring header;
        if (m_numericX && m_hoverSeries < m_series.size() && idx < m_series[m_hoverSeries].xs.size()) {
            header = FormatNumber(m_series[m_hoverSeries].xs[idx], m_xStep);
        } else {
            header = idx < m_categories.size() ? m_categories[idx] : std::to_wstring(idx + 1);
        }
        std::vector<std::pair<size_t, std::wstring>> lines;
        float maxW = TextW(header);
        for (size_t s = 0; s < m_series.size(); ++s) {
            if (!m_series[s].visible || idx >= m_series[s].values.size()) continue;
            const std::wstring line = m_series[s].name + L": " + FormatNumber(m_series[s].values[idx], m_yStep > 0 ? m_yStep / 10.0f : 1.0f);
            maxW = std::max(maxW, TextW(line) + ScaleValue(16.0f));
            lines.emplace_back(s, line);
        }
        if (lines.empty()) return;

        const float pad = ScaleValue(8.0f);
        const float lineH = TextH() + ScaleValue(2.0f);
        const float w = maxW + pad * 2.0f;
        const float h = lineH * static_cast<float>(lines.size() + 1) + pad * 2.0f;
        float x = m_mouseX + ScaleValue(14.0f);
        float y = m_mouseY - h * 0.5f;
        if (x + w > m_bounds.right) x = m_mouseX - ScaleValue(14.0f) - w;
        x = std::max(x, m_bounds.left);
        y = std::clamp(y, m_bounds.top, std::max(m_bounds.top, m_bounds.bottom - h));

        const Rect box = Rect::Make(x, y, x + w, y + h);
        renderer.FillRoundedRect(box, tooltipBackground, ScaleValue(6.0f));
        renderer.DrawRoundedRect(box, borderColor, 1.0f, ScaleValue(6.0f));
        float ty = y + pad;
        DrawText(renderer, header, Rect::Make(x + pad, ty, x + w - pad, ty + lineH), textColor,
                 TextHorizontalAlign::Start);
        ty += lineH;
        const float sw = ScaleValue(8.0f);
        for (const auto& [s, line] : lines) {
            const float cy = ty + lineH * 0.5f;
            renderer.FillRect(Rect::Make(x + pad, cy - sw * 0.5f, x + pad + sw, cy + sw * 0.5f), SeriesColor(s));
            DrawText(renderer, line, Rect::Make(x + pad + sw + ScaleValue(6.0f), ty, x + w - pad, ty + lineH),
                     ColorFromHex(0xE6EDF3), TextHorizontalAlign::Start);
            ty += lineH;
        }
    }

    std::vector<ChartSeries> m_series;
    std::vector<std::wstring> m_categories;
    bool m_manualRange = false;
    float m_yMin = 0.0f;
    float m_yMax = 1.0f;

    // Layout cache (rebuilt each Render; used by hit testing).
    Rect m_plot{};
    std::vector<Rect> m_legendRects;
    float m_yLo = 0.0f, m_yHi = 1.0f, m_yStep = 1.0f;
    float m_xLo = 0.0f, m_xHi = 1.0f, m_xStep = 1.0f;
    bool m_numericX = false;

    // Hover state.
    int m_hoverIndex = -1;
    size_t m_hoverSeries = 0;
    float m_mouseX = 0.0f;
    float m_mouseY = 0.0f;
};
