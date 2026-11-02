#pragma once
// ui_financial_chart.h - High-performance Financial / Time-Sharing & Candlestick Chart for ai-win-ui.
// Supports:
//  - ChartMode::TimeSharing: Dual curves (weighted white + leading yellow), volume bars, indicators, dual Y-axis
//  - ChartMode::Candlestick: Candlestick bodies, wicks, MA5/MA10/MA20 moving averages, volume bars, OHLCV header
//  - Crosshair cursor & top info header with mouse hover tracking
#include "ui_element.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

enum class FinancialChartMode {
    TimeSharing,
    Candlestick
};

struct FinancialPoint {
    float price = 0.0f;       // 加权指数/最新价 (白线)
    float leadPrice = 0.0f;   // 领先指数/不加权 (黄线)
    float avgPrice = 0.0f;    // 分时均价 (可选)
    float volume = 0.0f;      // 成交量 (手)
    float indicator1 = 0.0f;  // 副图指标1 (如红线)
    float indicator2 = 0.0f;  // 副图指标2 (如黄线)
    bool isUp = true;         // 相比前一分钟是涨是跌
    std::string timeStr;      // "09:30", "10:15" 等
};

struct CandleBar {
    std::string date;         // "2026-03-12"
    float open = 0.0f;
    float high = 0.0f;
    float low = 0.0f;
    float close = 0.0f;
    float volume = 0.0f;
    float ma5 = 0.0f;
    float ma10 = 0.0f;
    float ma20 = 0.0f;
};

class FinancialChart : public UIElement {
public:
    FinancialChart() = default;

    // ---- Mode Switching ----
    void SetChartMode(FinancialChartMode mode) {
        m_mode = mode;
        m_hoverIndex = -1;
        RecalculateScale();
    }
    FinancialChartMode GetChartMode() const { return m_mode; }

    // ---- Data Interface (TimeSharing) ----
    void SetPreClose(float preClose) {
        m_preClose = preClose > 0.0f ? preClose : 1.0f;
        RecalculateScale();
    }
    float GetPreClose() const { return m_preClose; }

    void SetPoints(std::vector<FinancialPoint> points) {
        m_points = std::move(points);
        RecalculateScale();
    }

    void AppendPoint(const FinancialPoint& pt) {
        m_points.push_back(pt);
        RecalculateScale();
    }

    void ClearPoints() {
        m_points.clear();
        m_hoverIndex = -1;
        RecalculateScale();
    }

    size_t PointCount() const { return m_points.size(); }

    // ---- Data Interface (Candlestick) ----
    void SetCandles(std::vector<CandleBar> candles) {
        m_candles = std::move(candles);
        RecalculateScale();
    }

    void AppendCandle(const CandleBar& candle) {
        m_candles.push_back(candle);
        RecalculateScale();
    }

    const std::vector<CandleBar>& GetCandles() const { return m_candles; }
    size_t CandleCount() const { return m_candles.size(); }

    // ---- Appearance & Color Tokens ----
    Color bg = ColorFromHex(0x000000);              // 纯黑背景
    Color borderCol = ColorFromHex(0x260000);       // 边框暗红
    Color gridCol = ColorFromHex(0x201010);         // 网格虚线/实线
    Color baselineCol = ColorFromHex(0x553333);     // 昨收参考线
    Color whiteLineCol = ColorFromHex(0xFFFFFF);    // 分时加权线 / MA5 (纯白)
    Color yellowLineCol = ColorFromHex(0xE5B800);   // 分时领先线 / MA10 (亮黄)
    Color purpleLineCol = ColorFromHex(0xDA70D6);   // MA20 (淡紫兰花)
    Color upCol = ColorFromHex(0xEE2222);           // 涨 (亮红)
    Color downCol = ColorFromHex(0x00CC00);         // 跌 (荧光绿)
    Color crosshairCol = ColorFromHex(0xDDDDDD);     // 十字光标
    Color textDimCol = ColorFromHex(0x888888);       // 刻度字体暗灰
    Color textBrightCol = ColorFromHex(0xFFFFFF);    // 亮白文本

    bool showCrosshair = true;
    bool showHeaderInfo = true;
    bool modernTheme = false;

    void SetThemeMode(bool modern) {
        modernTheme = modern;
        if (modern) {
            bg = ColorFromHex(0x131722);
            borderCol = ColorFromHex(0x2A2E39);
            gridCol = ColorFromHex(0x1E222D);
            baselineCol = ColorFromHex(0x434651);
            whiteLineCol = ColorFromHex(0x2962FF);
            yellowLineCol = ColorFromHex(0xFFA000);
            purpleLineCol = ColorFromHex(0xAB47BC);
            upCol = ColorFromHex(0xF6465D);
            downCol = ColorFromHex(0x0ECB81);
            crosshairCol = ColorFromHex(0x787B86);
            textDimCol = ColorFromHex(0x787B86);
            textBrightCol = ColorFromHex(0xD1D4DC);
        } else {
            bg = ColorFromHex(0x000000);
            borderCol = ColorFromHex(0x260000);
            gridCol = ColorFromHex(0x201010);
            baselineCol = ColorFromHex(0x553333);
            whiteLineCol = ColorFromHex(0xFFFFFF);
            yellowLineCol = ColorFromHex(0xE5B800);
            purpleLineCol = ColorFromHex(0xDA70D6);
            upCol = ColorFromHex(0xEE2222);
            downCol = ColorFromHex(0x00CC00);
            crosshairCol = ColorFromHex(0xDDDDDD);
            textDimCol = ColorFromHex(0x888888);
            textBrightCol = ColorFromHex(0xFFFFFF);
        }
    }

protected:
    float MeasurePreferredWidth(float availableWidth) const override {
        return availableWidth > 0 ? availableWidth : ScaleValue(600.0f);
    }
    float MeasurePreferredHeight(float availableHeight) const override {
        return availableHeight > 0 ? availableHeight : ScaleValue(450.0f);
    }

    bool OnMouseLeave() override {
        bool changed = m_isMouseInside || (m_hoverIndex != -1);
        m_isMouseInside = false;
        m_hoverIndex = -1;
        UIElement::OnMouseLeave();
        return changed;
    }

    bool OnMouseMove(float x, float y) override {
        m_isMouseInside = true;
        m_mouseX = x;
        m_mouseY = y;

        int oldHover = m_hoverIndex;
        if (m_chartRect.Width() <= 1.0f) {
            m_hoverIndex = -1;
        } else if (m_mode == FinancialChartMode::TimeSharing) {
            if (m_points.empty()) {
                m_hoverIndex = -1;
            } else {
                float relX = x - m_chartRect.left;
                float ratio = std::clamp(relX / m_chartRect.Width(), 0.0f, 1.0f);
                int idx = static_cast<int>(std::round(ratio * 240.0f));
                if (idx >= static_cast<int>(m_points.size())) {
                    idx = static_cast<int>(m_points.size()) - 1;
                }
                m_hoverIndex = std::max(0, idx);
            }
        } else {
            if (m_candles.empty()) {
                m_hoverIndex = -1;
            } else {
                float relX = x - m_chartRect.left;
                float ratio = std::clamp(relX / m_chartRect.Width(), 0.0f, 1.0f);
                int idx = static_cast<int>(ratio * static_cast<float>(m_candles.size()));
                idx = std::clamp(idx, 0, static_cast<int>(m_candles.size()) - 1);
                m_hoverIndex = idx;
            }
        }

        UIElement::OnMouseMove(x, y);
        return oldHover != m_hoverIndex;
    }

    void Render(IRenderer& renderer) override {
        const Rect bounds = Bounds();
        if (bounds.Width() <= 10.0f || bounds.Height() <= 10.0f) return;

        // 1. 填充背景与外边框
        renderer.FillRect(bounds, bg);
        renderer.DrawRect(bounds, borderCol, 1.0f);

        // 留出左/右文字刻度宽度及顶部 Header / 底部时间刻度
        const float marginL = ScaleValue(54.0f);
        const float marginR = ScaleValue(54.0f);
        const float marginT = ScaleValue(showHeaderInfo ? 26.0f : 6.0f);
        const float marginB = ScaleValue(22.0f);

        m_chartRect = Rect::Make(
            bounds.left + marginL,
            bounds.top + marginT,
            bounds.right - marginR,
            bounds.bottom - marginB
        );

        if (m_chartRect.Width() <= 20.0f || m_chartRect.Height() <= 40.0f) return;

        // 2. 划分三个 Pane
        const float totalH = m_chartRect.Height();
        const float priceH = totalH * 0.58f;
        const float volH = totalH * 0.20f;
        const float gap = totalH * 0.02f;

        m_priceRect = Rect::Make(m_chartRect.left, m_chartRect.top, m_chartRect.right, m_chartRect.top + priceH);
        m_volRect = Rect::Make(m_chartRect.left, m_priceRect.bottom + gap, m_chartRect.right, m_priceRect.bottom + gap + volH);
        m_indRect = Rect::Make(m_chartRect.left, m_volRect.bottom + gap, m_chartRect.right, m_chartRect.bottom);

        // 3. 绘制 Pane 背景与网格
        DrawPaneGrids(renderer);

        // 4. 根据模式分流绘制
        if (m_mode == FinancialChartMode::TimeSharing) {
            if (!m_points.empty()) {
                DrawTimeSharingLines(renderer);
                DrawTimeSharingVolume(renderer);
                DrawTimeSharingIndicators(renderer);
            }
            DrawTimeSharingAxisLabels(renderer);
            DrawTimeSharingHeaderInfo(renderer, bounds);
            if (showCrosshair && m_isMouseInside && m_hoverIndex >= 0 && m_hoverIndex < static_cast<int>(m_points.size())) {
                DrawTimeSharingCrosshair(renderer);
            }
        } else {
            if (!m_candles.empty()) {
                DrawCandlestickBars(renderer);
                DrawCandlestickVolume(renderer);
                DrawCandlestickMAs(renderer);
            }
            DrawCandlestickAxisLabels(renderer);
            DrawCandlestickHeaderInfo(renderer, bounds);
            if (showCrosshair && m_isMouseInside && m_hoverIndex >= 0 && m_hoverIndex < static_cast<int>(m_candles.size())) {
                DrawCandlestickCrosshair(renderer);
            }
        }
    }

private:
    void RecalculateScale() {
        if (m_mode == FinancialChartMode::TimeSharing) {
            if (m_points.empty()) {
                m_maxDiff = m_preClose * 0.01f;
                m_maxVolume = 100.0f;
                return;
            }
            float maxDiff = 0.0f;
            float maxVol = 0.0f;
            for (const auto& pt : m_points) {
                float d1 = std::abs(pt.price - m_preClose);
                float d2 = std::abs(pt.leadPrice - m_preClose);
                maxDiff = std::max(maxDiff, std::max(d1, d2));
                maxVol = std::max(maxVol, pt.volume);
            }
            float minDiff = m_preClose * 0.003f;
            m_maxDiff = std::max(maxDiff, minDiff) * 1.05f;
            m_maxVolume = std::max(maxVol, 10.0f);
        } else {
            if (m_candles.empty()) {
                m_candleMinPrice = 3400.0f;
                m_candleMaxPrice = 3600.0f;
                m_maxVolume = 100.0f;
                return;
            }
            float minP = 1e9f;
            float maxP = -1e9f;
            float maxV = 0.0f;
            for (const auto& c : m_candles) {
                minP = std::min(minP, c.low);
                maxP = std::max(maxP, c.high);
                if (c.ma5 > 0.0f) { minP = std::min(minP, c.ma5); maxP = std::max(maxP, c.ma5); }
                if (c.ma10 > 0.0f) { minP = std::min(minP, c.ma10); maxP = std::max(maxP, c.ma10); }
                if (c.ma20 > 0.0f) { minP = std::min(minP, c.ma20); maxP = std::max(maxP, c.ma20); }
                maxV = std::max(maxV, c.volume);
            }
            float pad = (maxP - minP) * 0.06f;
            if (pad <= 0.01f) pad = 10.0f;
            m_candleMinPrice = minP - pad;
            m_candleMaxPrice = maxP + pad;
            m_maxVolume = std::max(maxV, 10.0f);
        }
    }

    void DrawPaneGrids(IRenderer& renderer) {
        renderer.DrawRect(m_priceRect, borderCol, 1.0f);
        renderer.DrawRect(m_volRect, borderCol, 1.0f);
        renderer.DrawRect(m_indRect, borderCol, 1.0f);

        // 价格中轴基准线
        float midY = (m_priceRect.top + m_priceRect.bottom) * 0.5f;
        renderer.DrawDashedLine(PointF{m_priceRect.left, midY}, PointF{m_priceRect.right, midY}, baselineCol, 1.0f, 4.0f, 4.0f);

        // 上下 1/4 虚线
        float q1Y = m_priceRect.top + (midY - m_priceRect.top) * 0.5f;
        float q3Y = midY + (m_priceRect.bottom - midY) * 0.5f;
        renderer.DrawDashedLine(PointF{m_priceRect.left, q1Y}, PointF{m_priceRect.right, q1Y}, gridCol, 1.0f, 2.0f, 4.0f);
        renderer.DrawDashedLine(PointF{m_priceRect.left, q3Y}, PointF{m_priceRect.right, q3Y}, gridCol, 1.0f, 2.0f, 4.0f);

        // 纵向时间分割线: 25%, 50%, 75%
        float xs[3] = {
            m_chartRect.left + m_chartRect.Width() * 0.25f,
            m_chartRect.left + m_chartRect.Width() * 0.50f,
            m_chartRect.left + m_chartRect.Width() * 0.75f,
        };

        for (float x : xs) {
            renderer.DrawDashedLine(PointF{x, m_priceRect.top}, PointF{x, m_priceRect.bottom}, gridCol, 1.0f, 2.0f, 4.0f);
            renderer.DrawDashedLine(PointF{x, m_volRect.top}, PointF{x, m_volRect.bottom}, gridCol, 1.0f, 2.0f, 4.0f);
            renderer.DrawDashedLine(PointF{x, m_indRect.top}, PointF{x, m_indRect.bottom}, gridCol, 1.0f, 2.0f, 4.0f);
        }
    }

    // ==========================================
    // 模式 A: 分时图渲染
    // ==========================================
    void DrawTimeSharingLines(IRenderer& renderer) {
        if (m_points.size() < 2) return;

        float yMin = m_preClose - m_maxDiff;
        float yMax = m_preClose + m_maxDiff;
        float yRange = yMax - yMin;
        if (yRange <= 0.001f) yRange = 1.0f;

        std::vector<PointF> whitePts;
        std::vector<PointF> yellowPts;
        std::vector<PointF> areaPts;
        whitePts.reserve(m_points.size());
        yellowPts.reserve(m_points.size());
        areaPts.reserve(m_points.size() + 2);

        float stepX = m_priceRect.Width() / 240.0f;

        for (size_t i = 0; i < m_points.size(); ++i) {
            float x = m_priceRect.left + static_cast<float>(i) * stepX;
            float yWhite = m_priceRect.bottom - ((m_points[i].price - yMin) / yRange) * m_priceRect.Height();
            float yYellow = m_priceRect.bottom - ((m_points[i].leadPrice - yMin) / yRange) * m_priceRect.Height();

            yWhite = std::clamp(yWhite, m_priceRect.top, m_priceRect.bottom);
            yYellow = std::clamp(yYellow, m_priceRect.top, m_priceRect.bottom);

            whitePts.push_back(PointF{x, yWhite});
            yellowPts.push_back(PointF{x, yYellow});
        }

        // 绘制黄线 (领先指数)
        renderer.DrawPolyline(yellowPts, yellowLineCol, 1.2f);
        // 绘制白线 (加权指数)
        renderer.DrawPolyline(whitePts, whiteLineCol, 1.8f);

        // 绘制白线下淡色半透明渐变面积
        if (!whitePts.empty()) {
            areaPts.push_back(PointF{whitePts.front().x, m_priceRect.bottom});
            for (const auto& pt : whitePts) {
                areaPts.push_back(pt);
            }
            areaPts.push_back(PointF{whitePts.back().x, m_priceRect.bottom});
            Color fillTop = MakeColor(1.0f, 1.0f, 1.0f, 0.10f);
            Color fillBtm = MakeColor(1.0f, 1.0f, 1.0f, 0.01f);
            renderer.FillPolygonLinearGradient(areaPts, m_priceRect.top, m_priceRect.bottom, fillTop, fillBtm);
        }
    }

    void DrawTimeSharingVolume(IRenderer& renderer) {
        if (m_points.empty() || m_maxVolume <= 0.0f) return;

        float stepX = m_volRect.Width() / 240.0f;
        float barW = std::max(1.0f, stepX * 0.7f);

        for (size_t i = 0; i < m_points.size(); ++i) {
            float x = m_volRect.left + static_cast<float>(i) * stepX;
            float h = (m_points[i].volume / m_maxVolume) * m_volRect.Height();
            h = std::clamp(h, 1.0f, m_volRect.Height());
            float topY = m_volRect.bottom - h;

            Color barCol = m_points[i].isUp ? upCol : downCol;
            Rect r = Rect::Make(x - barW * 0.5f, topY, x + barW * 0.5f, m_volRect.bottom);
            renderer.FillRect(r, barCol);
        }
    }

    void DrawTimeSharingIndicators(IRenderer& renderer) {
        if (m_points.size() < 2) return;

        float maxInd = 100.0f;
        for (const auto& pt : m_points) {
            maxInd = std::max(maxInd, std::max(pt.indicator1, pt.indicator2));
        }

        std::vector<PointF> line1;
        std::vector<PointF> line2;
        float stepX = m_indRect.Width() / 240.0f;

        for (size_t i = 0; i < m_points.size(); ++i) {
            float x = m_indRect.left + static_cast<float>(i) * stepX;
            float y1 = m_indRect.bottom - (m_points[i].indicator1 / maxInd) * m_indRect.Height();
            float y2 = m_indRect.bottom - (m_points[i].indicator2 / maxInd) * m_indRect.Height();
            line1.push_back(PointF{x, std::clamp(y1, m_indRect.top, m_indRect.bottom)});
            line2.push_back(PointF{x, std::clamp(y2, m_indRect.top, m_indRect.bottom)});
        }

        renderer.DrawPolyline(line1, upCol, 1.2f);
        renderer.DrawPolyline(line2, yellowLineCol, 1.2f);
    }

    void DrawTimeSharingAxisLabels(IRenderer& renderer) {
        float yMin = m_preClose - m_maxDiff;
        float yMax = m_preClose + m_maxDiff;
        float maxPct = (m_maxDiff / m_preClose) * 100.0f;

        TextRenderOptions leftOpt;
        leftOpt.wrap = TextWrapMode::NoWrap;
        leftOpt.horizontalAlign = TextHorizontalAlign::End;
        leftOpt.verticalAlign = TextVerticalAlign::Center;

        TextRenderOptions rightOpt;
        rightOpt.wrap = TextWrapMode::NoWrap;
        rightOpt.horizontalAlign = TextHorizontalAlign::Start;
        rightOpt.verticalAlign = TextVerticalAlign::Center;

        wchar_t buf[32];
        swprintf_s(buf, L"%.2f", yMax);
        renderer.DrawTextW(buf, static_cast<UINT32>(wcslen(buf)),
                           Rect::Make(m_chartRect.left - 52.0f, m_priceRect.top - 8.0f, m_chartRect.left - 4.0f, m_priceRect.top + 8.0f),
                           upCol, 11.0f, leftOpt);

        swprintf_s(buf, L"%.2f", m_preClose);
        float midY = (m_priceRect.top + m_priceRect.bottom) * 0.5f;
        renderer.DrawTextW(buf, static_cast<UINT32>(wcslen(buf)),
                           Rect::Make(m_chartRect.left - 52.0f, midY - 8.0f, m_chartRect.left - 4.0f, midY + 8.0f),
                           textDimCol, 11.0f, leftOpt);

        swprintf_s(buf, L"%.2f", yMin);
        renderer.DrawTextW(buf, static_cast<UINT32>(wcslen(buf)),
                           Rect::Make(m_chartRect.left - 52.0f, m_priceRect.bottom - 8.0f, m_chartRect.left - 4.0f, m_priceRect.bottom + 8.0f),
                           downCol, 11.0f, leftOpt);

        swprintf_s(buf, L"+%.2f%%", maxPct);
        renderer.DrawTextW(buf, static_cast<UINT32>(wcslen(buf)),
                           Rect::Make(m_chartRect.right + 4.0f, m_priceRect.top - 8.0f, m_chartRect.right + 52.0f, m_priceRect.top + 8.0f),
                           upCol, 11.0f, rightOpt);

        renderer.DrawTextW(L"0.00%", 5,
                           Rect::Make(m_chartRect.right + 4.0f, midY - 8.0f, m_chartRect.right + 52.0f, midY + 8.0f),
                           textDimCol, 11.0f, rightOpt);

        swprintf_s(buf, L"-%.2f%%", maxPct);
        renderer.DrawTextW(buf, static_cast<UINT32>(wcslen(buf)),
                           Rect::Make(m_chartRect.right + 4.0f, m_priceRect.bottom - 8.0f, m_chartRect.right + 52.0f, m_priceRect.bottom + 8.0f),
                           downCol, 11.0f, rightOpt);

        // 成交量副图刻度
        swprintf_s(buf, L"%.0f", m_maxVolume);
        renderer.DrawTextW(buf, static_cast<UINT32>(wcslen(buf)),
                           Rect::Make(m_chartRect.left - 52.0f, m_volRect.top - 6.0f, m_chartRect.left - 4.0f, m_volRect.top + 8.0f),
                           textDimCol, 10.0f, leftOpt);

        // 底部时间刻度
        TextRenderOptions timeOpt;
        timeOpt.wrap = TextWrapMode::NoWrap;
        timeOpt.verticalAlign = TextVerticalAlign::Start;

        timeOpt.horizontalAlign = TextHorizontalAlign::Start;
        renderer.DrawTextW(L"09:30", 5, Rect::Make(m_chartRect.left, m_chartRect.bottom + 4.0f, m_chartRect.left + 40.0f, m_chartRect.bottom + 20.0f), textDimCol, 11.0f, timeOpt);

        timeOpt.horizontalAlign = TextHorizontalAlign::Center;
        float x1030 = m_chartRect.left + m_chartRect.Width() * 0.25f;
        renderer.DrawTextW(L"10:30", 5, Rect::Make(x1030 - 20.0f, m_chartRect.bottom + 4.0f, x1030 + 20.0f, m_chartRect.bottom + 20.0f), textDimCol, 11.0f, timeOpt);

        float x1130 = m_chartRect.left + m_chartRect.Width() * 0.50f;
        renderer.DrawTextW(L"11:30/13:00", 11, Rect::Make(x1130 - 35.0f, m_chartRect.bottom + 4.0f, x1130 + 35.0f, m_chartRect.bottom + 20.0f), textDimCol, 11.0f, timeOpt);

        float x1400 = m_chartRect.left + m_chartRect.Width() * 0.75f;
        renderer.DrawTextW(L"14:00", 5, Rect::Make(x1400 - 20.0f, m_chartRect.bottom + 4.0f, x1400 + 20.0f, m_chartRect.bottom + 20.0f), textDimCol, 11.0f, timeOpt);

        timeOpt.horizontalAlign = TextHorizontalAlign::End;
        renderer.DrawTextW(L"15:00", 5, Rect::Make(m_chartRect.right - 40.0f, m_chartRect.bottom + 4.0f, m_chartRect.right, m_chartRect.bottom + 20.0f), textDimCol, 11.0f, timeOpt);
    }

    void DrawTimeSharingHeaderInfo(IRenderer& renderer, const Rect& bounds) {
        if (!showHeaderInfo) return;

        const FinancialPoint& cur = (m_hoverIndex >= 0 && m_hoverIndex < static_cast<int>(m_points.size()))
                                    ? m_points[m_hoverIndex]
                                    : (!m_points.empty() ? m_points.back() : FinancialPoint{});

        float diff = cur.price - m_preClose;
        float pct = (diff / m_preClose) * 100.0f;
        Color valCol = diff >= 0 ? upCol : downCol;

        wchar_t buf[256];
        swprintf_s(buf, L"上证指数 领先: %.2f  最新: %.2f  涨跌: %+.2f (%+.2f%%)  总手: %.0f  时间: %hs",
                   cur.leadPrice, cur.price, diff, pct, cur.volume,
                   cur.timeStr.empty() ? "15:00" : cur.timeStr.c_str());

        TextRenderOptions headerOpt;
        headerOpt.wrap = TextWrapMode::NoWrap;
        headerOpt.horizontalAlign = TextHorizontalAlign::Start;
        headerOpt.verticalAlign = TextVerticalAlign::Center;

        renderer.DrawTextW(buf, static_cast<UINT32>(wcslen(buf)),
                           Rect::Make(bounds.left + 10.0f, bounds.top + 2.0f, bounds.right - 10.0f, bounds.top + 24.0f),
                           valCol, 12.0f, headerOpt);
    }

    void DrawTimeSharingCrosshair(IRenderer& renderer) {
        const FinancialPoint& pt = m_points[m_hoverIndex];
        float stepX = m_priceRect.Width() / 240.0f;
        float crossX = m_priceRect.left + static_cast<float>(m_hoverIndex) * stepX;

        float yMin = m_preClose - m_maxDiff;
        float yMax = m_preClose + m_maxDiff;
        float yRange = yMax - yMin;
        if (yRange <= 0.001f) yRange = 1.0f;

        float crossY = m_priceRect.bottom - ((pt.price - yMin) / yRange) * m_priceRect.Height();
        crossY = std::clamp(crossY, m_priceRect.top, m_priceRect.bottom);

        renderer.DrawDashedLine(PointF{crossX, m_priceRect.top}, PointF{crossX, m_indRect.bottom}, crosshairCol, 1.0f, 3.0f, 3.0f);
        renderer.DrawDashedLine(PointF{m_priceRect.left, crossY}, PointF{m_priceRect.right, crossY}, crosshairCol, 1.0f, 3.0f, 3.0f);

        TextRenderOptions badgeOpt;
        badgeOpt.wrap = TextWrapMode::NoWrap;
        badgeOpt.horizontalAlign = TextHorizontalAlign::Center;
        badgeOpt.verticalAlign = TextVerticalAlign::Center;

        wchar_t priceBuf[32];
        swprintf_s(priceBuf, L"%.2f", pt.price);
        Rect lBadge = Rect::Make(m_priceRect.left - 52.0f, crossY - 9.0f, m_priceRect.left - 2.0f, crossY + 9.0f);
        renderer.FillRect(lBadge, ColorFromHex(0x333333));
        renderer.DrawRect(lBadge, crosshairCol, 1.0f);
        renderer.DrawTextW(priceBuf, static_cast<UINT32>(wcslen(priceBuf)), lBadge, textBrightCol, 11.0f, badgeOpt);

        float pct = ((pt.price - m_preClose) / m_preClose) * 100.0f;
        wchar_t pctBuf[32];
        swprintf_s(pctBuf, L"%+.2f%%", pct);
        Rect rBadge = Rect::Make(m_priceRect.right + 2.0f, crossY - 9.0f, m_priceRect.right + 52.0f, crossY + 9.0f);
        renderer.FillRect(rBadge, ColorFromHex(0x333333));
        renderer.DrawRect(rBadge, crosshairCol, 1.0f);
        renderer.DrawTextW(pctBuf, static_cast<UINT32>(wcslen(pctBuf)), rBadge, pt.price >= m_preClose ? upCol : downCol, 11.0f, badgeOpt);

        wchar_t timeBuf[32];
        swprintf_s(timeBuf, L"%hs", pt.timeStr.empty() ? "--:--" : pt.timeStr.c_str());
        Rect bBadge = Rect::Make(crossX - 25.0f, m_chartRect.bottom + 2.0f, crossX + 25.0f, m_chartRect.bottom + 18.0f);
        renderer.FillRect(bBadge, ColorFromHex(0x333333));
        renderer.DrawRect(bBadge, crosshairCol, 1.0f);
        renderer.DrawTextW(timeBuf, static_cast<UINT32>(wcslen(timeBuf)), bBadge, textBrightCol, 10.0f, badgeOpt);
    }

    // ==========================================
    // 模式 B: K 线图 (Candlestick) 渲染
    // ==========================================
    void DrawCandlestickBars(IRenderer& renderer) {
        if (m_candles.empty()) return;

        float n = static_cast<float>(m_candles.size());
        float stepX = m_priceRect.Width() / n;
        float barW = std::max(2.0f, stepX * 0.70f);

        float yRange = m_candleMaxPrice - m_candleMinPrice;
        if (yRange <= 0.01f) yRange = 1.0f;

        auto PriceToY = [&](float p) {
            float ratio = (p - m_candleMinPrice) / yRange;
            float y = m_priceRect.bottom - ratio * m_priceRect.Height();
            return std::clamp(y, m_priceRect.top, m_priceRect.bottom);
        };

        for (size_t i = 0; i < m_candles.size(); ++i) {
            const auto& c = m_candles[i];
            float cx = m_priceRect.left + (static_cast<float>(i) + 0.5f) * stepX;

            float yOpen = PriceToY(c.open);
            float yClose = PriceToY(c.close);
            float yHigh = PriceToY(c.high);
            float yLow = PriceToY(c.low);

            bool isUp = (c.close >= c.open);
            Color barCol = isUp ? upCol : downCol;

            // 1. 绘制上下影线 (一条贯穿 high 到 low 的细线)
            renderer.DrawLine(PointF{cx, yHigh}, PointF{cx, yLow}, barCol, 1.0f);

            // 2. 绘制实体蜡烛
            float topY = std::min(yOpen, yClose);
            float btmY = std::max(yOpen, yClose);
            float entityH = std::max(1.5f, btmY - topY);

            Rect entityRect = Rect::Make(cx - barW * 0.5f, topY, cx + barW * 0.5f, topY + entityH);
            renderer.FillRect(entityRect, barCol);
        }
    }

    void DrawCandlestickVolume(IRenderer& renderer) {
        if (m_candles.empty() || m_maxVolume <= 0.0f) return;

        float n = static_cast<float>(m_candles.size());
        float stepX = m_volRect.Width() / n;
        float barW = std::max(2.0f, stepX * 0.70f);

        for (size_t i = 0; i < m_candles.size(); ++i) {
            const auto& c = m_candles[i];
            float cx = m_volRect.left + (static_cast<float>(i) + 0.5f) * stepX;

            float h = (c.volume / m_maxVolume) * m_volRect.Height();
            h = std::clamp(h, 1.0f, m_volRect.Height());
            float topY = m_volRect.bottom - h;

            Color barCol = (c.close >= c.open) ? upCol : downCol;
            Rect r = Rect::Make(cx - barW * 0.5f, topY, cx + barW * 0.5f, m_volRect.bottom);
            renderer.FillRect(r, barCol);
        }
    }

    void DrawCandlestickMAs(IRenderer& renderer) {
        if (m_candles.size() < 2) return;

        float n = static_cast<float>(m_candles.size());
        float stepX = m_priceRect.Width() / n;
        float yRange = m_candleMaxPrice - m_candleMinPrice;
        if (yRange <= 0.01f) yRange = 1.0f;

        auto PriceToY = [&](float p) {
            float ratio = (p - m_candleMinPrice) / yRange;
            return std::clamp(m_priceRect.bottom - ratio * m_priceRect.Height(), m_priceRect.top, m_priceRect.bottom);
        };

        std::vector<PointF> ma5Pts;
        std::vector<PointF> ma10Pts;
        std::vector<PointF> ma20Pts;

        for (size_t i = 0; i < m_candles.size(); ++i) {
            const auto& c = m_candles[i];
            float cx = m_priceRect.left + (static_cast<float>(i) + 0.5f) * stepX;

            if (c.ma5 > 0.0f) ma5Pts.push_back(PointF{cx, PriceToY(c.ma5)});
            if (c.ma10 > 0.0f) ma10Pts.push_back(PointF{cx, PriceToY(c.ma10)});
            if (c.ma20 > 0.0f) ma20Pts.push_back(PointF{cx, PriceToY(c.ma20)});
        }

        if (ma5Pts.size() >= 2) renderer.DrawPolyline(ma5Pts, whiteLineCol, 1.2f);
        if (ma10Pts.size() >= 2) renderer.DrawPolyline(ma10Pts, yellowLineCol, 1.2f);
        if (ma20Pts.size() >= 2) renderer.DrawPolyline(ma20Pts, purpleLineCol, 1.2f);

        // 在 IndicatorPane (副图3) 绘制成交量/均量线或 MACD 占位
        float midIndY = (m_indRect.top + m_indRect.bottom) * 0.5f;
        renderer.DrawDashedLine(PointF{m_indRect.left, midIndY}, PointF{m_indRect.right, midIndY}, gridCol, 1.0f, 2.0f, 4.0f);
    }

    void DrawCandlestickAxisLabels(IRenderer& renderer) {
        TextRenderOptions leftOpt;
        leftOpt.wrap = TextWrapMode::NoWrap;
        leftOpt.horizontalAlign = TextHorizontalAlign::End;
        leftOpt.verticalAlign = TextVerticalAlign::Center;

        TextRenderOptions rightOpt;
        rightOpt.wrap = TextWrapMode::NoWrap;
        rightOpt.horizontalAlign = TextHorizontalAlign::Start;
        rightOpt.verticalAlign = TextVerticalAlign::Center;

        wchar_t buf[32];
        // 最高价
        swprintf_s(buf, L"%.2f", m_candleMaxPrice);
        renderer.DrawTextW(buf, static_cast<UINT32>(wcslen(buf)),
                           Rect::Make(m_chartRect.left - 52.0f, m_priceRect.top - 8.0f, m_chartRect.left - 4.0f, m_priceRect.top + 8.0f),
                           upCol, 11.0f, leftOpt);

        // 中位价
        float midPrice = (m_candleMaxPrice + m_candleMinPrice) * 0.5f;
        swprintf_s(buf, L"%.2f", midPrice);
        float midY = (m_priceRect.top + m_priceRect.bottom) * 0.5f;
        renderer.DrawTextW(buf, static_cast<UINT32>(wcslen(buf)),
                           Rect::Make(m_chartRect.left - 52.0f, midY - 8.0f, m_chartRect.left - 4.0f, midY + 8.0f),
                           textDimCol, 11.0f, leftOpt);

        // 最低价
        swprintf_s(buf, L"%.2f", m_candleMinPrice);
        renderer.DrawTextW(buf, static_cast<UINT32>(wcslen(buf)),
                           Rect::Make(m_chartRect.left - 52.0f, m_priceRect.bottom - 8.0f, m_chartRect.left - 4.0f, m_priceRect.bottom + 8.0f),
                           downCol, 11.0f, leftOpt);

        // 右轴相对区间幅度
        float ampPct = ((m_candleMaxPrice - m_candleMinPrice) / m_candleMinPrice) * 100.0f;
        swprintf_s(buf, L"+%.2f%%", ampPct * 0.5f);
        renderer.DrawTextW(buf, static_cast<UINT32>(wcslen(buf)),
                           Rect::Make(m_chartRect.right + 4.0f, m_priceRect.top - 8.0f, m_chartRect.right + 52.0f, m_priceRect.top + 8.0f),
                           upCol, 11.0f, rightOpt);

        renderer.DrawTextW(L"0.00%", 5,
                           Rect::Make(m_chartRect.right + 4.0f, midY - 8.0f, m_chartRect.right + 52.0f, midY + 8.0f),
                           textDimCol, 11.0f, rightOpt);

        swprintf_s(buf, L"-%.2f%%", ampPct * 0.5f);
        renderer.DrawTextW(buf, static_cast<UINT32>(wcslen(buf)),
                           Rect::Make(m_chartRect.right + 4.0f, m_priceRect.bottom - 8.0f, m_chartRect.right + 52.0f, m_priceRect.bottom + 8.0f),
                           downCol, 11.0f, rightOpt);

        // 成交量副图最大值
        swprintf_s(buf, L"%.0f", m_maxVolume);
        renderer.DrawTextW(buf, static_cast<UINT32>(wcslen(buf)),
                           Rect::Make(m_chartRect.left - 52.0f, m_volRect.top - 6.0f, m_chartRect.left - 4.0f, m_volRect.top + 8.0f),
                           textDimCol, 10.0f, leftOpt);

        // 底部日期采样刻度 (起始日、1/4、1/2、3/4、最新日)
        if (!m_candles.empty()) {
            TextRenderOptions timeOpt;
            timeOpt.wrap = TextWrapMode::NoWrap;
            timeOpt.verticalAlign = TextVerticalAlign::Start;

            size_t idxs[4] = {
                0,
                m_candles.size() / 3,
                (m_candles.size() * 2) / 3,
                m_candles.size() - 1
            };
            float stepX = m_chartRect.Width() / static_cast<float>(m_candles.size());

            for (int k = 0; k < 4; ++k) {
                size_t id = idxs[k];
                float x = m_chartRect.left + (static_cast<float>(id) + 0.5f) * stepX;
                timeOpt.horizontalAlign = (k == 0) ? TextHorizontalAlign::Start : (k == 3 ? TextHorizontalAlign::End : TextHorizontalAlign::Center);

                wchar_t dateBuf[32];
                swprintf_s(dateBuf, L"%hs", m_candles[id].date.c_str());
                renderer.DrawTextW(dateBuf, static_cast<UINT32>(wcslen(dateBuf)),
                                   Rect::Make(x - 30.0f, m_chartRect.bottom + 4.0f, x + 30.0f, m_chartRect.bottom + 20.0f),
                                   textDimCol, 10.0f, timeOpt);
            }
        }
    }

    void DrawCandlestickHeaderInfo(IRenderer& renderer, const Rect& bounds) {
        if (!showHeaderInfo) return;

        const CandleBar& cur = (m_hoverIndex >= 0 && m_hoverIndex < static_cast<int>(m_candles.size()))
                               ? m_candles[m_hoverIndex]
                               : (!m_candles.empty() ? m_candles.back() : CandleBar{});

        float diff = cur.close - cur.open;
        float pct = cur.open > 0.0f ? (diff / cur.open) * 100.0f : 0.0f;
        Color valCol = diff >= 0 ? upCol : downCol;

        wchar_t buf[300];
        swprintf_s(buf, L"上证指数 日K  开:%.2f 高:%.2f 低:%.2f 收:%.2f %+.2f(%+.2f%%)  MA5:%.2f MA10:%.2f MA20:%.2f  量:%.0f  %hs",
                   cur.open, cur.high, cur.low, cur.close, diff, pct,
                   cur.ma5, cur.ma10, cur.ma20, cur.volume, cur.date.c_str());

        TextRenderOptions headerOpt;
        headerOpt.wrap = TextWrapMode::NoWrap;
        headerOpt.horizontalAlign = TextHorizontalAlign::Start;
        headerOpt.verticalAlign = TextVerticalAlign::Center;

        renderer.DrawTextW(buf, static_cast<UINT32>(wcslen(buf)),
                           Rect::Make(bounds.left + 10.0f, bounds.top + 2.0f, bounds.right - 10.0f, bounds.top + 24.0f),
                           valCol, 11.5f, headerOpt);
    }

    void DrawCandlestickCrosshair(IRenderer& renderer) {
        const CandleBar& c = m_candles[m_hoverIndex];
        float n = static_cast<float>(m_candles.size());
        float stepX = m_priceRect.Width() / n;
        float crossX = m_priceRect.left + (static_cast<float>(m_hoverIndex) + 0.5f) * stepX;

        float yRange = m_candleMaxPrice - m_candleMinPrice;
        if (yRange <= 0.01f) yRange = 1.0f;

        float crossY = m_priceRect.bottom - ((c.close - m_candleMinPrice) / yRange) * m_priceRect.Height();
        crossY = std::clamp(crossY, m_priceRect.top, m_priceRect.bottom);

        // 垂直贯穿线与水平参考线
        renderer.DrawDashedLine(PointF{crossX, m_priceRect.top}, PointF{crossX, m_indRect.bottom}, crosshairCol, 1.0f, 3.0f, 3.0f);
        renderer.DrawDashedLine(PointF{m_priceRect.left, crossY}, PointF{m_priceRect.right, crossY}, crosshairCol, 1.0f, 3.0f, 3.0f);

        TextRenderOptions badgeOpt;
        badgeOpt.wrap = TextWrapMode::NoWrap;
        badgeOpt.horizontalAlign = TextHorizontalAlign::Center;
        badgeOpt.verticalAlign = TextVerticalAlign::Center;

        // 左轴当前价浮动标签
        wchar_t priceBuf[32];
        swprintf_s(priceBuf, L"%.2f", c.close);
        Rect lBadge = Rect::Make(m_priceRect.left - 52.0f, crossY - 9.0f, m_priceRect.left - 2.0f, crossY + 9.0f);
        renderer.FillRect(lBadge, ColorFromHex(0x333333));
        renderer.DrawRect(lBadge, crosshairCol, 1.0f);
        renderer.DrawTextW(priceBuf, static_cast<UINT32>(wcslen(priceBuf)), lBadge, textBrightCol, 11.0f, badgeOpt);

        // 底部日期浮动标签
        wchar_t dateBuf[32];
        swprintf_s(dateBuf, L"%hs", c.date.c_str());
        Rect bBadge = Rect::Make(crossX - 35.0f, m_chartRect.bottom + 2.0f, crossX + 35.0f, m_chartRect.bottom + 18.0f);
        renderer.FillRect(bBadge, ColorFromHex(0x333333));
        renderer.DrawRect(bBadge, crosshairCol, 1.0f);
        renderer.DrawTextW(dateBuf, static_cast<UINT32>(wcslen(dateBuf)), bBadge, textBrightCol, 10.0f, badgeOpt);
    }

    FinancialChartMode m_mode = FinancialChartMode::TimeSharing;
    float m_preClose = 3518.76f;
    float m_maxDiff = 30.0f;
    float m_maxVolume = 10000.0f;

    // Candlestick 标尺
    float m_candleMinPrice = 3400.0f;
    float m_candleMaxPrice = 3600.0f;

    std::vector<FinancialPoint> m_points;
    std::vector<CandleBar> m_candles;

    Rect m_chartRect{};
    Rect m_priceRect{};
    Rect m_volRect{};
    Rect m_indRect{};

    bool m_isMouseInside = false;
    float m_mouseX = 0.0f;
    float m_mouseY = 0.0f;
    int m_hoverIndex = -1;
};
