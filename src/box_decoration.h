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

struct BoxDecoration {
    Color background = Color{0.0f, 0.0f, 0.0f, 0.0f};
    BorderSpec border{};
    CornerRadius radius{};
    float opacity = 1.0f;
};

void DrawBoxDecoration(IRenderer& renderer, const Rect& bounds, const BoxDecoration& decoration);
