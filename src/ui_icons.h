#pragma once
// Built-in vector icons (<Icon>) and inline SVG path element (<Path>)
// Dual-backend (Skia & Direct2D) vector rendering via IRenderer::DrawSvgPath.

#include "ui_element.h"

#include <algorithm>
#include <string>

class VectorPath : public UIElement {
public:
    VectorPath() = default;
    explicit VectorPath(std::string pathData) : data(std::move(pathData)) {}

    std::string data;
    float viewBoxWidth = 24.0f;
    float viewBoxHeight = 24.0f;
    Color fill = Color{0.0f, 0.0f, 0.0f, 0.0f};
    Color stroke = ColorFromHex(0xEDEDED);
    float strokeWidth = 1.8f;

protected:
    float MeasurePreferredWidth(float availableWidth) const override {
        const float w = ScaleValue(viewBoxWidth > 0.0f ? viewBoxWidth : 24.0f);
        return availableWidth > 0.0f ? std::min(availableWidth, w) : w;
    }

    float MeasurePreferredHeight(float /*width*/) const override {
        return ScaleValue(viewBoxHeight > 0.0f ? viewBoxHeight : 24.0f);
    }

public:
    void Render(IRenderer& renderer) override {
        if (!renderer.IntersectsDirtyRect(m_bounds) || data.empty()) {
            return;
        }
        renderer.DrawSvgPath(
            data,
            viewBoxWidth,
            viewBoxHeight,
            m_bounds,
            fill,
            stroke,
            ScaleValue(strokeWidth));
    }
};

class VectorIcon : public UIElement {
public:
    explicit VectorIcon(std::string iconName = "search") : name(std::move(iconName)) {}

    std::string name = "search";
    float size = 18.0f;
    Color color = ColorFromHex(0xEDEDED);
    Color fill = Color{0.0f, 0.0f, 0.0f, 0.0f};
    float strokeWidth = 2.0f;

    static const char* LookupIconPath(const std::string& iconName, bool& outFilledDefault) {
        outFilledDefault = false;
        if (iconName == "search") {
            return "M11 3a8 8 0 1 0 0 16 8 8 0 1 0 0-16z M21 21l-4.35-4.35";
        }
        if (iconName == "bell") {
            return "M18 8A6 6 0 0 0 6 8c0 7-3 9-3 9h18s-3-2-3-9 M13.73 21a2 2 0 0 1-3.46 0";
        }
        if (iconName == "calendar") {
            return "M5 4h14a2 2 0 0 1 2 2v14a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2V6a2 2 0 0 1 2-2z M16 2v4 M8 2v4 M3 10h18";
        }
        if (iconName == "user") {
            return "M20 21v-2a4 4 0 0 0-4-4H8a4 4 0 0 0-4 4v2 M12 3a4 4 0 1 0 0 8 4 4 0 1 0 0-8z";
        }
        if (iconName == "users" || iconName == "patients") {
            return "M17 21v-2a4 4 0 0 0-4-4H5a4 4 0 0 0-4 4v2 M9 3a4 4 0 1 0 0 8 4 4 0 1 0 0-8z M23 21v-2a4 4 0 0 0-3-3.87 M16 3.13a4 4 0 0 1 0 7.75";
        }
        if (iconName == "check") {
            return "M20 6L9 17l-5-5";
        }
        if (iconName == "check-circle" || iconName == "success") {
            return "M12 2a10 10 0 1 0 0 20 10 10 0 1 0 0-20z M9 12l2 2 4-4";
        }
        if (iconName == "info") {
            return "M12 2a10 10 0 1 0 0 20 10 10 0 1 0 0-20z M12 16v-4 M12 8h.01";
        }
        if (iconName == "alert-circle" || iconName == "danger" || iconName == "error") {
            return "M12 2a10 10 0 1 0 0 20 10 10 0 1 0 0-20z M12 8v4 M12 16h.01";
        }
        if (iconName == "alert-triangle" || iconName == "warning") {
            return "M10.29 3.86L1.82 18a2 2 0 0 0 1.71 3h16.94a2 2 0 0 0 1.71-3L13.71 3.86a2 2 0 0 0-3.42 0z M12 9v4 M12 17h.01";
        }
        if (iconName == "x" || iconName == "close") {
            return "M18 6L6 18 M6 6l12 12";
        }
        if (iconName == "plus") {
            return "M12 5v14 M5 12h14";
        }
        if (iconName == "chevron-down") {
            return "M6 9l6 6 6-6";
        }
        if (iconName == "chevron-up") {
            return "M18 15l-6-6-6 6";
        }
        if (iconName == "chevron-right") {
            return "M9 18l6-6-6-6";
        }
        if (iconName == "chevron-left") {
            return "M15 18l-6-6 6-6";
        }
        if (iconName == "dashboard" || iconName == "grid") {
            return "M3 3h7v7H3z M14 3h7v7h-7z M14 14h7v7h-7z M3 14h7v7H3z";
        }
        if (iconName == "activity" || iconName == "pulse") {
            return "M22 12h-4l-3 9L9 3l-3 9H2";
        }
        if (iconName == "file-text" || iconName == "lab") {
            return "M14 2H6a2 2 0 0 0-2 2v16a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V8z M14 2v6h6 M16 13H8 M16 17H8 M10 9H8";
        }
        if (iconName == "pill" || iconName == "pharmacy") {
            return "M10.5 20.5l10-10a4.95 4.95 0 1 0-7-7l-10 10a4.95 4.95 0 1 0 7 7z M8.5 8.5l7 7";
        }
        if (iconName == "credit-card" || iconName == "billing" || iconName == "revenue") {
            return "M4 5h16a2 2 0 0 1 2 2v10a2 2 0 0 1-2 2H4a2 2 0 0 1-2-2V7a2 2 0 0 1 2-2z M2 10h20";
        }
        if (iconName == "message-square" || iconName == "communications") {
            return "M21 15a2 2 0 0 1-2 2H7l-4 4V5a2 2 0 0 1 2-2h14a2 2 0 0 1 2 2z";
        }
        if (iconName == "settings") {
            return "M12 8a4 4 0 1 0 0 8 4 4 0 1 0 0-8z M12 2v2 M12 20v2 M4.93 4.93l1.41 1.41 M17.66 17.66l1.41 1.41 M2 12h2 M20 12h2 M4.93 19.07l1.41-1.41 M17.66 6.34l1.41-1.41";
        }
        if (iconName == "sliders") {
            return "M4 21v-7 M4 10V3 M12 21v-9 M12 8V3 M20 21v-5 M20 12V3 M1 14h6 M9 8h6 M17 16h6";
        }
        if (iconName == "inbox" || iconName == "empty") {
            return "M22 12h-6l-2 3h-4l-2-3H2 M5.45 5.11L2 12v6a2 2 0 0 0 2 2h16a2 2 0 0 0 2-2v-6l-3.45-6.89A2 2 0 0 0 16.76 4H7.24a2 2 0 0 0-1.79 1.11z";
        }
        if (iconName == "help-circle" || iconName == "support") {
            return "M12 2a10 10 0 1 0 0 20 10 10 0 1 0 0-20z M9.09 9a3 3 0 0 1 5.83 1c0 2-3 3-3 3 M12 17h.01";
        }
        if (iconName == "trending-up") {
            return "M23 6l-9.5 9.5-5-5L1 18 M17 6h6v6";
        }
        if (iconName == "clock") {
            return "M12 2a10 10 0 1 0 0 20 10 10 0 1 0 0-20z M12 6v6l4 2";
        }
        if (iconName == "filter") {
            return "M22 3H2l8 9.46V19l4 2v-8.54L22 3z";
        }
        if (iconName == "share" || iconName == "referrals") {
            return "M4 12v8a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2v-8 M16 6l-4-4-4 4 M12 2v13";
        }
        if (iconName == "heart") {
            return "M20.84 4.61a5.5 5.5 0 0 0-7.78 0L12 5.67l-1.06-1.06a5.5 5.5 0 0 0-7.78 7.78l1.06 1.06L12 21.23l7.78-7.78 1.06-1.06a5.5 5.5 0 0 0 0-7.78z";
        }
        if (iconName == "star") {
            return "M12 2l3.09 6.26L22 9.27l-5 4.87 1.18 6.88L12 17.77l-6.18 3.25L7 14.14 2 9.27l6.91-1.01L12 2z";
        }
        if (iconName == "more-horizontal") {
            return "M12 11a1 1 0 1 0 0 2 1 1 0 1 0 0-2z M19 11a1 1 0 1 0 0 2 1 1 0 1 0 0-2z M5 11a1 1 0 1 0 0 2 1 1 0 1 0 0-2z";
        }
        if (iconName == "folder") {
            return "M22 19a2 2 0 0 1-2 2H4a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h5l2 3h9a2 2 0 0 1 2 2z";
        }
        if (iconName == "file" || iconName == "document") {
            return "M13 2H6a2 2 0 0 0-2 2v16a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V9z M13 2v7h7";
        }
        if (iconName == "home") {
            return "M3 9l9-7 9 7v11a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2z M9 22V12h6v10";
        }
        if (iconName == "code") {
            return "M16 18l6-6-6-6 M8 6l-6 6 6 6";
        }
        if (iconName == "database") {
            return "M12 2C6.48 2 2 3.79 2 6v12c0 2.21 4.48 4 10 4s10-1.79 10-4V6c0-2.21-4.48-4-10-4z M2 12c0 2.21 4.48 4 10 4s10-1.79 10-4 M2 6c0 2.21 4.48 4 10 4s10-1.79 10-4";
        }
        if (iconName == "arrow-up-right") {
            return "M7 17L17 7 M7 7h10v10";
        }
        if (iconName == "dot") {
            outFilledDefault = true;
            return "M12 6a6 6 0 1 0 0 12 6 6 0 1 0 0-12z";
        }
        // Fallback: circle
        return "M12 4a8 8 0 1 0 0 16 8 8 0 1 0 0-16z";
    }

protected:
    float MeasurePreferredWidth(float availableWidth) const override {
        const float w = ScaleValue(size);
        return availableWidth > 0.0f ? std::min(availableWidth, w) : w;
    }

    float MeasurePreferredHeight(float /*width*/) const override {
        return ScaleValue(size);
    }

public:
    void Render(IRenderer& renderer) override {
        if (!renderer.IntersectsDirtyRect(m_bounds)) {
            return;
        }
        bool filledDefault = false;
        const char* pathData = LookupIconPath(name, filledDefault);
        if (!pathData) {
            return;
        }
        const Color effectiveFill = (fill.a > 0.001f) ? fill : (filledDefault ? color : Color{0.0f, 0.0f, 0.0f, 0.0f});
        const Color effectiveStroke = filledDefault ? Color{0.0f, 0.0f, 0.0f, 0.0f} : color;
        renderer.DrawSvgPath(
            pathData,
            24.0f,
            24.0f,
            m_bounds,
            effectiveFill,
            effectiveStroke,
            ScaleValue(strokeWidth));
    }
};
