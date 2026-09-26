#pragma once

#include "graphics_types.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>

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

struct TransitionDef {
    std::string propertyName; // e.g. "background", "border", "opacity"
    uint32_t durationMs = 0;
    EasingType easing = EasingType::Linear;
};

// Manages the state of an animatable color
struct AnimatableColor {
    Color current{0,0,0,0};
    Color start{0,0,0,0};
    Color target{0,0,0,0};
    uint32_t elapsedMs = 0;
    uint32_t durationMs = 0;
    EasingType easing = EasingType::Linear;
    bool active = false;

    AnimatableColor() = default;
    AnimatableColor(const Color& initial) : current(initial), start(initial), target(initial) {}

    void SetTarget(const Color& newTarget, uint32_t transitionDuration, EasingType transitionEasing) {
        if (target.r == newTarget.r && target.g == newTarget.g && target.b == newTarget.b && target.a == newTarget.a) {
            return;
        }
        start = current;
        target = newTarget;
        durationMs = transitionDuration;
        easing = transitionEasing;
        elapsedMs = 0;
        
        if (durationMs > 0) {
            active = true;
        } else {
            current = target;
            active = false;
        }
    }

    // Returns true if animation changed state and requires a repaint
    bool Tick(uint32_t dt) {
        if (!active) return false;
        
        elapsedMs += dt;
        if (elapsedMs >= durationMs) {
            current = target;
            active = false;
            return true; // We just finished this frame, need one last repaint
        }
        
        float progress = static_cast<float>(elapsedMs) / static_cast<float>(durationMs);
        float t = Easing::Evaluate(progress, easing);
        current = AnimationController::LerpColor(start, target, t);
        return true;
    }
};

} // namespace ai_win_ui
