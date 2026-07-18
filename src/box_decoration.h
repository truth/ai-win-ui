#pragma once

#include "graphics_types.h"

class IRenderer;

struct CornerRadius {
    float topLeft = 0.0f;
    float topRight = 0.0f;
    float bottomRight = 0.0f;
    float bottomLeft = 0.0f;

    static CornerRadius Uniform(float r) { return CornerRadius{r, r, r, r}; }

    bool IsZero() const {
        return topLeft <= 0.0f && topRight <= 0.0f &&
               bottomRight <= 0.0f && bottomLeft <= 0.0f;
    }

    bool IsUniform() const {
        return topLeft == topRight && topRight == bottomRight && bottomRight == bottomLeft;
    }

    float MaxRadius() const {
        float m = topLeft;
        if (topRight > m) m = topRight;
        if (bottomRight > m) m = bottomRight;
        if (bottomLeft > m) m = bottomLeft;
        return m;
    }
};

struct BorderSpec {
    Thickness width{};
    Color color{};

    bool IsZero() const {
        return width.left <= 0.0f && width.top <= 0.0f &&
               width.right <= 0.0f && width.bottom <= 0.0f;
    }
};

// Wave2 R7: two-stop linear gradient. angleDegrees: 0 = L→R, 90 = T→B (CSS-like).
struct LinearGradientSpec {
    bool enabled = false;
    Color start = Color{0.0f, 0.0f, 0.0f, 1.0f};
    Color end = Color{1.0f, 1.0f, 1.0f, 1.0f};
    float angleDegrees = 90.0f;
};

// Soft drop shadow (multi-pass approx on both backends).
struct SoftShadowSpec {
    bool enabled = false;
    float offsetX = 0.0f;
    float offsetY = 6.0f;
    float blur = 16.0f;
    Color color = Color{0.0f, 0.0f, 0.0f, 0.35f};
};

struct BoxDecoration {
    Color background = Color{0.0f, 0.0f, 0.0f, 0.0f};
    BorderSpec border{};
    CornerRadius radius{};
    float opacity = 1.0f;
    LinearGradientSpec gradient{};
    SoftShadowSpec shadow{};
};

void DrawBoxDecoration(IRenderer& renderer, const Rect& bounds, const BoxDecoration& decoration);
