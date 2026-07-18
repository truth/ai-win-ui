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

float MaxBorderWidth(const Thickness& w) {
    float m = w.left;
    if (w.top > m) m = w.top;
    if (w.right > m) m = w.right;
    if (w.bottom > m) m = w.bottom;
    return m;
}

} // namespace

void DrawBoxDecoration(IRenderer& renderer, const Rect& bounds, const BoxDecoration& d) {
    if (d.opacity <= 0.0f) {
        return;
    }

    const float radius = d.radius.MaxRadius();
    const bool hasRadius = radius > 0.0f;
    const bool hasGradient = d.gradient.enabled;
    const bool hasBackground = d.background.a > 0.0f || hasGradient;
    const bool hasBorder = !d.border.IsZero() && d.border.color.a > 0.0f;

    // Shadow first (behind fill).
    if (d.shadow.enabled && d.shadow.color.a > 0.001f) {
        renderer.FillSoftShadow(
            bounds,
            radius,
            d.shadow.offsetX,
            d.shadow.offsetY,
            d.shadow.blur,
            WithOpacity(d.shadow.color, d.opacity));
    }

    if (hasGradient) {
        renderer.FillLinearGradient(
            bounds,
            radius,
            WithOpacity(d.gradient.start, d.opacity),
            WithOpacity(d.gradient.end, d.opacity),
            d.gradient.angleDegrees);
    } else if (hasBackground && d.background.a > 0.0f) {
        const Color bg = WithOpacity(d.background, d.opacity);
        if (hasRadius) {
            renderer.FillRoundedRect(bounds, bg, radius);
        } else {
            renderer.FillRect(bounds, bg);
        }
    }

    if (hasBorder) {
        const float strokeWidth = MaxBorderWidth(d.border.width);
        const float halfStroke = strokeWidth * 0.5f;
        const Rect strokeRect = InsetRect(bounds, halfStroke);
        const Color borderColor = WithOpacity(d.border.color, d.opacity);
        if (hasRadius) {
            const float strokeRadius = std::max(0.0f, radius - halfStroke);
            renderer.DrawRoundedRect(strokeRect, borderColor, strokeWidth, strokeRadius);
        } else {
            renderer.DrawRect(strokeRect, borderColor, strokeWidth);
        }
    }
}
