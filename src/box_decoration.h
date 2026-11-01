#pragma once

#include "graphics_types.h"
#include "animation_controller.h"
#include <vector>
#include <string>

class IRenderer;

struct BorderSpec {
    Thickness width{};
    Color color{};

    bool IsZero() const {
        return width.left <= 0.0f && width.top <= 0.0f &&
               width.right <= 0.0f && width.bottom <= 0.0f;
    }

    bool IsUniform() const {
        return width.left == width.top && width.top == width.right && width.right == width.bottom;
    }
};

// Wave2 R7: two-stop linear gradient. angleDegrees: 0 = L->R, 90 = T->B (CSS-like).
struct LinearGradientSpec {
    bool enabled = false;
    Color start = Color{0.0f, 0.0f, 0.0f, 1.0f};
    Color end = Color{1.0f, 1.0f, 1.0f, 1.0f};
    float angleDegrees = 90.0f;
};

// Soft drop shadow (Gaussian blur on Skia, multi-pass smooth approx on D2D).
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
    std::vector<ai_win_ui::TransitionDef> transitions;
};

void DrawBoxDecoration(IRenderer& renderer, const Rect& bounds, const BoxDecoration& decoration);
