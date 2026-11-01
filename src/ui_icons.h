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
        if (iconName == "plus") {
            return "M12 5v14 M5 12h14";
        }
        if (iconName == "chevron-down") {
            return "M6 9l6 6 6-6";
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

