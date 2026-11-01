#pragma once

#include <algorithm>
#include <cstdint>
#include <memory>

struct Size {
    float width = 0.0f;
    float height = 0.0f;
};

struct PointF {
    float x = 0.0f;
    float y = 0.0f;
};

struct Rect {
    float left = 0.0f;
    float top = 0.0f;
    float right = 0.0f;
    float bottom = 0.0f;

    static Rect Make(float left, float top, float right, float bottom) {
        return Rect{left, top, right, bottom};
    }

    float Width() const {
        return std::max(0.0f, right - left);
    }

    float Height() const {
        return std::max(0.0f, bottom - top);
    }

    bool Contains(float x, float y) const {
        return x >= left && x <= right && y >= top && y <= bottom;
    }
};

struct Thickness {
    float left = 0.0f;
    float top = 0.0f;
    float right = 0.0f;
    float bottom = 0.0f;
};

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

struct Color {
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
    float a = 1.0f;
};

inline Color MakeColor(float r, float g, float b, float a = 1.0f) {
    return Color{r, g, b, a};
}

inline Color ColorFromHex(uint32_t rgb, float alpha = 1.0f) {
    return Color{
        static_cast<float>((rgb >> 16) & 0xFF) / 255.0f,
        static_cast<float>((rgb >> 8) & 0xFF) / 255.0f,
        static_cast<float>(rgb & 0xFF) / 255.0f,
        alpha
    };
}

class BitmapResource {
public:
    virtual ~BitmapResource() = default;
    virtual Size GetSize() const = 0;
};

using BitmapHandle = std::shared_ptr<BitmapResource>;

class SvgResource {
public:
    virtual ~SvgResource() = default;
    virtual Size GetIntrinsicSize() const = 0;
};

using SvgHandle = std::shared_ptr<SvgResource>;
