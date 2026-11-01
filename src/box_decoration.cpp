#include "box_decoration.h"

#include "renderer.h"

#include <algorithm>

namespace {

Color WithOpacity(const Color& c, float opacity) {
    return Color{c.r, c.g, c.b, std::clamp(c.a * opacity, 0.0f, 1.0f)};
}

Rect InsetRect(const Rect& r, float amount) {
    return Rect::Make(
        r.left + amount,
        r.top + amount,
        std::max(r.left + amount, r.right - amount),
        std::max(r.top + amount, r.bottom - amount));
}

CornerRadius InsetRadii(const CornerRadius& rad, float amount) {
    return CornerRadius{
        std::max(0.0f, rad.topLeft - amount),
        std::max(0.0f, rad.topRight - amount),
        std::max(0.0f, rad.bottomRight - amount),
        std::max(0.0f, rad.bottomLeft - amount),
    };
}

} // namespace

void DrawBoxDecoration(IRenderer& renderer, const Rect& bounds, const BoxDecoration& d) {
    if (d.opacity <= 0.0f) {
        return;
    }

    const float maxRadius = d.radius.MaxRadius();
    const bool hasRadius = !d.radius.IsZero();
    const bool uniformRadius = d.radius.IsUniform();
    const bool hasGradient = d.gradient.enabled;
    const bool hasBackground = d.background.a > 0.0f || hasGradient;
    const bool hasBorder = !d.border.IsZero() && d.border.color.a > 0.0f;

    // Shadow first (behind fill).
    if (d.shadow.enabled && d.shadow.color.a > 0.001f) {
        renderer.FillSoftShadow(
            bounds,
            maxRadius,
            d.shadow.offsetX,
            d.shadow.offsetY,
            d.shadow.blur,
            WithOpacity(d.shadow.color, d.opacity));
    }

    if (hasGradient) {
        renderer.FillLinearGradient(
            bounds,
            maxRadius,
            WithOpacity(d.gradient.start, d.opacity),
            WithOpacity(d.gradient.end, d.opacity),
            d.gradient.angleDegrees);
    } else if (hasBackground && d.background.a > 0.0f) {
        const Color bg = WithOpacity(d.background, d.opacity);
        if (hasRadius) {
            if (uniformRadius) {
                renderer.FillRoundedRect(bounds, bg, maxRadius);
            } else {
                renderer.FillRRect(bounds, d.radius, bg);
            }
        } else {
            renderer.FillRect(bounds, bg);
        }
    }

    if (hasBorder) {
        const Color borderColor = WithOpacity(d.border.color, d.opacity);
        const Thickness& w = d.border.width;
        if (d.border.IsUniform()) {
            const float strokeWidth = w.left;
            const float halfStroke = strokeWidth * 0.5f;
            const Rect strokeRect = InsetRect(bounds, halfStroke);
            if (hasRadius) {
                if (uniformRadius) {
                    const float strokeRadius = std::max(0.0f, maxRadius - halfStroke);
                    renderer.DrawRoundedRect(strokeRect, borderColor, strokeWidth, strokeRadius);
                } else {
                    renderer.DrawRRect(strokeRect, InsetRadii(d.radius, halfStroke), borderColor, strokeWidth);
                }
            } else {
                renderer.DrawRect(strokeRect, borderColor, strokeWidth);
            }
        } else {
            // Non-uniform per-edge borders (e.g., bottom divider border="0,0,0,1" or left accent bar border="3,0,0,0").
            if (w.top > 0.0f) {
                renderer.FillRect(
                    Rect::Make(bounds.left, bounds.top, bounds.right, std::min(bounds.bottom, bounds.top + w.top)),
                    borderColor);
            }
            if (w.bottom > 0.0f) {
                renderer.FillRect(
                    Rect::Make(bounds.left, std::max(bounds.top, bounds.bottom - w.bottom), bounds.right, bounds.bottom),
                    borderColor);
            }
            if (w.left > 0.0f) {
                renderer.FillRect(
                    Rect::Make(bounds.left, bounds.top, std::min(bounds.right, bounds.left + w.left), bounds.bottom),
                    borderColor);
            }
            if (w.right > 0.0f) {
                renderer.FillRect(
                    Rect::Make(std::max(bounds.left, bounds.right - w.right), bounds.top, bounds.right, bounds.bottom),
                    borderColor);
            }
        }
    }
}
