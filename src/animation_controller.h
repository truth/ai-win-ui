#pragma once

#include "graphics_types.h"
#include <algorithm>
#include <cmath>

namespace ai_win_ui {

enum class EasingType {
    Linear,
    EaseIn,
    EaseOut,
    EaseInOut,
    CubicBezier,
    Spring
};

class Easing {
public:
    static float Evaluate(float t, EasingType type = EasingType::EaseOut) {
        t = std::clamp(t, 0.0f, 1.0f);
        switch (type) {
            case EasingType::Linear:
                return t;
            case EasingType::EaseIn:
                return t * t;
            case EasingType::EaseOut:
                return t * (2.0f - t);
            case EasingType::EaseInOut:
                return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t;
            case EasingType::Spring: {
                const float tension = 0.5f;
                return std::pow(2.0f, -10.0f * t) * std::sin((t - tension / 4.0f) * (2.0f * 3.14159265f) / tension) + 1.0f;
            }
            case EasingType::CubicBezier:
            default:
                return t * t * (3.0f - 2.0f * t);
        }
    }
};

class AnimationController {
public:
    static Color LerpColor(const Color& start, const Color& end, float progress) {
        progress = std::clamp(progress, 0.0f, 1.0f);
        return Color{
            start.r + (end.r - start.r) * progress,
            start.g + (end.g - start.g) * progress,
            start.b + (end.b - start.b) * progress,
            start.a + (end.a - start.a) * progress
        };
    }

    static float LerpFloat(float start, float end, float progress) {
        return start + (end - start) * std::clamp(progress, 0.0f, 1.0f);
    }
};

} // namespace ai_win_ui
