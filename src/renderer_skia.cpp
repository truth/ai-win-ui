#include "renderer.h"
#include "skia_text_layout.h"
#include "skia_font_shared.h"

#if defined(AI_WIN_UI_HAS_VCPKG_SKIA) || defined(AI_WIN_UI_HAS_LOCAL_SKIA)
#include "include/core/SkBlendMode.h"
#include "include/core/SkBlurTypes.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkColor.h"
#include "include/core/SkColorFilter.h"
#include "include/core/SkData.h"
#include "include/core/SkFontMetrics.h"
#include "include/core/SkImage.h"
#include "include/core/SkImageInfo.h"
#include "include/core/SkMaskFilter.h"
#include "include/core/SkPaint.h"
#include "include/core/SkPath.h"
#include "include/core/SkPathEffect.h"
#include "include/core/SkPoint.h"
#include "include/core/SkRRect.h"
#include "include/core/SkRegion.h"
#include "include/core/SkSamplingOptions.h"
#include "include/core/SkStream.h"
#include "include/core/SkSize.h"
#include "include/core/SkSurface.h"
#include "include/core/SkSurfaceProps.h"
#include "include/core/SkTextBlob.h"
#include "include/core/SkTileMode.h"
#include "include/core/SkTypes.h"
#include "include/effects/SkDashPathEffect.h"
#include "include/effects/SkGradientShader.h"
#include "include/utils/SkParsePath.h"

#include "modules/svg/include/SkSVGDOM.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <cwchar>
#include <list>
#include <memory>
#include <string>
#include <unordered_map>

namespace {

SkColor ToSkColor(const Color& color) {
    const auto clamp = [](float value) -> U8CPU {
        return static_cast<U8CPU>(std::clamp(value, 0.0f, 1.0f) * 255.0f);
    };
    return SkColorSetARGB(clamp(color.a), clamp(color.r), clamp(color.g), clamp(color.b));
}

SkRect ToSkRect(const Rect& rect) {
    return SkRect::MakeLTRB(rect.left, rect.top, rect.right, rect.bottom);
}

SkRRect ToSkRRect(const Rect& rect, const CornerRadius& r) {
    SkRRect rrect;
    const SkVector radii[4] = {
        {r.topLeft, r.topLeft},
        {r.topRight, r.topRight},
        {r.bottomRight, r.bottomRight},
        {r.bottomLeft, r.bottomLeft},
    };
    rrect.setRectRadii(ToSkRect(rect), radii);
    return rrect;
}

class SkiaBitmapResource final : public BitmapResource {
public:
    explicit SkiaBitmapResource(sk_sp<SkImage> image)
        : m_image(std::move(image)) {
        if (m_image) {
            m_size = Size{
                static_cast<float>(m_image->width()),
                static_cast<float>(m_image->height()),
            };
        }
    }

    Size GetSize() const override {
        return m_size;
    }

    const sk_sp<SkImage>& Image() const {
        return m_image;
    }

private:
    Size m_size{};
    sk_sp<SkImage> m_image;
};

class SkiaSvgResource final : public SvgResource {
public:
    explicit SkiaSvgResource(sk_sp<SkSVGDOM> dom)
        : m_dom(std::move(dom)) {
        if (m_dom) {
            const SkSize sz = m_dom->containerSize();
            if (sz.width() > 0.0f && sz.height() > 0.0f) {
                m_intrinsic = Size{sz.width(), sz.height()};
            } else {
                m_intrinsic = Size{24.0f, 24.0f};
            }
        }
    }

    Size GetIntrinsicSize() const override { return m_intrinsic; }
    const sk_sp<SkSVGDOM>& Dom() const { return m_dom; }

private:
    Size m_intrinsic{24.0f, 24.0f};
    sk_sp<SkSVGDOM> m_dom;
};

class SkiaRenderer final : public IRenderer {
public:
    ~SkiaRenderer() override {
        m_textBlobMap.clear();
        m_textBlobList.clear();
        m_fontMetricsCache.clear();
        m_dashEffectCache.clear();
        m_shadowCache.clear();
        m_svgPathCache.clear();
        ReleasePresentSurface();
    }

    RendererBackend Backend() const override {
        return RendererBackend::Skia;
    }

    void SetPresentMode(PresentMode mode) override {
        if (m_presentMode == mode) {
            return;
        }
        m_presentMode = mode;
        m_width = 0;
        m_height = 0;
        RecreateSurface();
    }

    PresentMode GetPresentMode() const override {
        return m_presentMode;
    }

    bool SampleOpaque(int x, int y, uint8_t alphaThreshold) const override {
        if (m_presentMode != PresentMode::Layered || !m_dibBits || m_width == 0 || m_height == 0) {
            return true;
        }
        if (x < 0 || y < 0 || x >= static_cast<int>(m_width) || y >= static_cast<int>(m_height)) {
            return false;
        }
        const size_t index = (static_cast<size_t>(y) * m_width + static_cast<size_t>(x)) * 4u;
        const auto* bytes = static_cast<const uint8_t*>(m_dibBits);
        return bytes[index + 3] >= alphaThreshold;
    }

    bool Initialize(HWND hwnd) override {
        m_hwnd = hwnd;
        EnsureFontManager();
        RecreateSurface();
        return true;
    }

    void Resize(UINT width, UINT height) override {
        width = std::max(1u, width);
        height = std::max(1u, height);
        if (width == m_width && height == m_height && m_surface) {
            return;
        }
        m_width = width;
        m_height = height;
        RecreateSurface();
    }

    void BeginFrame(const Color& clearColor) override {
        if (!EnsureSurface()) {
            return;
        }
        SkCanvas* canvas = m_surface->getCanvas();
        if (!canvas) {
            return;
        }

        canvas->resetMatrix();
        canvas->restoreToCount(1);
        if (HasDirtyRect() && m_presentMode == PresentMode::Hwnd && !m_surfaceNeedsFullPaint) {
            const auto& dirtyRects = GetDirtyRects();
            canvas->save();
            if (dirtyRects.size() <= 1) {
                canvas->clipRect(ToSkRect(GetDirtyRect()), false);
            } else {
                constexpr size_t kStackRectCap = 16;
                SkIRect stackRects[kStackRectCap];
                std::vector<SkIRect> heapRects;
                SkIRect* rects = stackRects;
                if (dirtyRects.size() > kStackRectCap) {
                    heapRects.resize(dirtyRects.size());
                    rects = heapRects.data();
                }
                int count = 0;
                for (const Rect& d : dirtyRects) {
                    SkIRect ir = SkIRect::MakeLTRB(
                        std::max(0, static_cast<int>(std::floor(d.left))),
                        std::max(0, static_cast<int>(std::floor(d.top))),
                        std::min(static_cast<int>(m_width), static_cast<int>(std::ceil(d.right))),
                        std::min(static_cast<int>(m_height), static_cast<int>(std::ceil(d.bottom))));
                    if (!ir.isEmpty()) {
                        rects[count++] = ir;
                    }
                }
                if (count > 0) {
                    SkRegion clipRegion;
                    clipRegion.setRects(rects, count);
                    canvas->clipRegion(clipRegion, SkClipOp::kIntersect);
                } else {
                    canvas->clipRect(ToSkRect(GetDirtyRect()), false);
                }
            }
            canvas->drawColor(ToSkColor(clearColor), SkBlendMode::kSrc);
        } else {
            ClearDirtyRect();
            canvas->clear(ToSkColor(clearColor));
            m_surfaceNeedsFullPaint = false;
        }
        m_frameDirty = true;
    }

    void EndFrame() override {
        if (!m_surface || !m_dibBits || !m_hwnd || m_width == 0 || m_height == 0) {
            return;
        }
        if (IsIconic(m_hwnd)) {
            m_frameDirty = false;
            return;
        }
        if (!m_frameDirty) {
            return;
        }

        if (m_presentMode == PresentMode::Layered) {
            PresentLayered();
        } else {
            PresentHwnd();
        }
        m_frameDirty = false;
    }

    void FillRect(const Rect& rect, const Color& color) override {
        if (!IntersectsDirtyRect(Rect::Make(rect.left - 1.0f, rect.top - 1.0f, rect.right + 1.0f, rect.bottom + 1.0f))) {
            return;
        }
        if (SkCanvas* canvas = Canvas()) {
            SkPaint paint;
            paint.setColor(ToSkColor(color));
            paint.setStyle(SkPaint::kFill_Style);
            paint.setAntiAlias(true);
            canvas->drawRect(ToSkRect(rect), paint);
        }
    }

    void FillRoundedRect(const Rect& rect, const Color& color, float radius) override {
        if (!IntersectsDirtyRect(Rect::Make(rect.left - 1.0f, rect.top - 1.0f, rect.right + 1.0f, rect.bottom + 1.0f))) {
            return;
        }
        if (SkCanvas* canvas = Canvas()) {
            SkPaint paint;
            paint.setColor(ToSkColor(color));
            paint.setStyle(SkPaint::kFill_Style);
            paint.setAntiAlias(true);
            canvas->drawRoundRect(ToSkRect(rect), radius, radius, paint);
        }
    }

    void FillRRect(const Rect& rect, const CornerRadius& radii, const Color& color) override {
        if (!IntersectsDirtyRect(Rect::Make(rect.left - 1.0f, rect.top - 1.0f, rect.right + 1.0f, rect.bottom + 1.0f))) {
            return;
        }
        if (SkCanvas* canvas = Canvas()) {
            SkPaint paint;
            paint.setColor(ToSkColor(color));
            paint.setStyle(SkPaint::kFill_Style);
            paint.setAntiAlias(true);
            canvas->drawRRect(ToSkRRect(rect, radii), paint);
        }
    }

    void FillLinearGradient(const Rect& rect,
                            float cornerRadius,
                            const Color& start,
                            const Color& end,
                            float angleDegrees) override {
        if (!IntersectsDirtyRect(Rect::Make(rect.left - 1.0f, rect.top - 1.0f, rect.right + 1.0f, rect.bottom + 1.0f))) {
            return;
        }
        if (SkCanvas* canvas = Canvas()) {
            const float cx = (rect.left + rect.right) * 0.5f;
            const float cy = (rect.top + rect.bottom) * 0.5f;
            const float rad = angleDegrees * 3.14159265f / 180.0f;
            const float c = std::cos(rad);
            const float s = std::sin(rad);
            const float maxDist = (rect.Width() * 0.5f * std::abs(c)) + (rect.Height() * 0.5f * std::abs(s));

            const SkPoint pts[2] = {
                SkPoint::Make(cx - c * maxDist, cy - s * maxDist),
                SkPoint::Make(cx + c * maxDist, cy + s * maxDist),
            };
            const SkColor colors[2] = {ToSkColor(start), ToSkColor(end)};
            SkPaint paint;
            paint.setAntiAlias(true);
            paint.setStyle(SkPaint::kFill_Style);
            paint.setShader(SkGradientShader::MakeLinear(
                pts, colors, nullptr, 2, SkTileMode::kClamp));
            if (cornerRadius > 0.5f) {
                canvas->drawRoundRect(ToSkRect(rect), cornerRadius, cornerRadius, paint);
            } else {
                canvas->drawRect(ToSkRect(rect), paint);
            }
        }
    }

    void FillSoftShadow(const Rect& rect,
                        float cornerRadius,
                        float offsetX,
                        float offsetY,
                        float blur,
                        const Color& color) override {
        if (color.a <= 0.001f) {
            return;
        }
        const float blurPx = std::max(0.0f, blur);
        const SkRect shadowRect = SkRect::MakeLTRB(
            rect.left + offsetX,
            rect.top + offsetY,
            rect.right + offsetX,
            rect.bottom + offsetY);
        const float sigma = blurPx * 0.45f;
        const float dirtyPad = (sigma > 0.25f) ? (sigma * 3.0f + 4.0f) : 2.0f;
        if (!IntersectsDirtyRect(Rect::Make(
                shadowRect.left() - dirtyPad, shadowRect.top() - dirtyPad,
                shadowRect.right() + dirtyPad, shadowRect.bottom() + dirtyPad))) {
            return;
        }
        if (SkCanvas* canvas = Canvas()) {

            // Visual Rendering v2 + P1 Shadow Raster Cache:
            // Cache blurred shadow as an offscreen SkImage keyed by quantized size, radius, blur, and color.
            if (sigma > 0.25f) {
                const float w = rect.Width();
                const float h = rect.Height();
                if (w > 1.0f && h > 1.0f && w <= 2200.0f && h <= 1600.0f) {
                    const ShadowCacheKey key{
                        static_cast<int32_t>(std::lround(w * 2.0f)),
                        static_cast<int32_t>(std::lround(h * 2.0f)),
                        static_cast<int32_t>(std::lround(cornerRadius * 2.0f)),
                        static_cast<int32_t>(std::lround(blurPx * 2.0f)),
                        ToSkColor(color)
                    };
                    auto it = m_shadowCache.find(key);
                    if (it == m_shadowCache.end()) {
                        if (m_shadowCache.size() >= 256) {
                            m_shadowCache.clear();
                        }
                        // Two-hit admission: skip offscreen allocation on one-off transient resize sizes.
                        it = m_shadowCache.emplace(key, ShadowCacheEntry{nullptr, 0, 1}).first;
                    } else if (!it->second.image && ++it->second.hits >= 2) {
                        const int pad = static_cast<int>(std::ceil(sigma * 3.0f + 3.0f));
                        const int imgW = static_cast<int>(std::ceil(w)) + pad * 2;
                        const int imgH = static_cast<int>(std::ceil(h)) + pad * 2;
                        if (sk_sp<SkSurface> off = SkSurfaces::Raster(SkImageInfo::MakeN32Premul(imgW, imgH))) {
                            if (sk_sp<SkMaskFilter> mf = SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, sigma)) {
                                SkPaint sp;
                                sp.setColor(ToSkColor(color));
                                sp.setStyle(SkPaint::kFill_Style);
                                sp.setAntiAlias(true);
                                sp.setMaskFilter(std::move(mf));
                                const SkRect localRect = SkRect::MakeXYWH(
                                    static_cast<float>(pad),
                                    static_cast<float>(pad),
                                    w,
                                    h);
                                if (cornerRadius > 0.5f) {
                                    off->getCanvas()->drawRoundRect(localRect, cornerRadius, cornerRadius, sp);
                                } else {
                                    off->getCanvas()->drawRect(localRect, sp);
                                }
                                it->second.image = off->makeImageSnapshot();
                                it->second.pad = pad;
                            }
                        }
                    }
                    if (it != m_shadowCache.end() && it->second.image) {
                        canvas->drawImage(
                            it->second.image.get(),
                            shadowRect.left() - static_cast<float>(it->second.pad),
                            shadowRect.top() - static_cast<float>(it->second.pad));
                        return;
                    }
                }
                if (sk_sp<SkMaskFilter> mf = SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, sigma)) {
                    SkPaint paint;
                    paint.setColor(ToSkColor(color));
                    paint.setStyle(SkPaint::kFill_Style);
                    paint.setAntiAlias(true);
                    paint.setMaskFilter(std::move(mf));
                    if (cornerRadius > 0.5f) {
                        canvas->drawRoundRect(shadowRect, cornerRadius, cornerRadius, paint);
                    } else {
                        canvas->drawRect(shadowRect, paint);
                    }
                    return;
                }
            }

            // Fallback when blur is near zero
            SkPaint paint;
            paint.setColor(ToSkColor(color));
            paint.setStyle(SkPaint::kFill_Style);
            paint.setAntiAlias(true);
            if (cornerRadius > 0.5f) {
                canvas->drawRoundRect(shadowRect, cornerRadius, cornerRadius, paint);
            } else {
                canvas->drawRect(shadowRect, paint);
            }
        }
    }

    void DrawRect(const Rect& rect, const Color& color, float strokeWidth) override {
        const float pad = strokeWidth * 0.5f + 1.5f;
        if (!IntersectsDirtyRect(Rect::Make(rect.left - pad, rect.top - pad, rect.right + pad, rect.bottom + pad))) {
            return;
        }
        if (SkCanvas* canvas = Canvas()) {
            SkPaint paint;
            paint.setColor(ToSkColor(color));
            paint.setStyle(SkPaint::kStroke_Style);
            paint.setStrokeWidth(strokeWidth);
            paint.setAntiAlias(true);
            canvas->drawRect(ToSkRect(rect), paint);
        }
    }

    void DrawRoundedRect(const Rect& rect, const Color& color, float strokeWidth, float radius) override {
        const float pad = strokeWidth * 0.5f + 1.5f;
        if (!IntersectsDirtyRect(Rect::Make(rect.left - pad, rect.top - pad, rect.right + pad, rect.bottom + pad))) {
            return;
        }
        if (SkCanvas* canvas = Canvas()) {
            SkPaint paint;
            paint.setColor(ToSkColor(color));
            paint.setStyle(SkPaint::kStroke_Style);
            paint.setStrokeWidth(strokeWidth);
            paint.setAntiAlias(true);
            canvas->drawRoundRect(ToSkRect(rect), radius, radius, paint);
        }
    }

    void DrawRRect(const Rect& rect, const CornerRadius& radii, const Color& color, float strokeWidth) override {
        const float pad = strokeWidth * 0.5f + 1.5f;
        if (!IntersectsDirtyRect(Rect::Make(rect.left - pad, rect.top - pad, rect.right + pad, rect.bottom + pad))) {
            return;
        }
        if (SkCanvas* canvas = Canvas()) {
            SkPaint paint;
            paint.setColor(ToSkColor(color));
            paint.setStyle(SkPaint::kStroke_Style);
            paint.setStrokeWidth(strokeWidth);
            paint.setAntiAlias(true);
            canvas->drawRRect(ToSkRRect(rect, radii), paint);
        }
    }

    void DrawLine(const PointF& start, const PointF& end, const Color& color, float strokeWidth) override {
        const float pad = strokeWidth * 0.5f + 2.0f;
        if (!IntersectsDirtyRect(Rect::Make(
                std::min(start.x, end.x) - pad, std::min(start.y, end.y) - pad,
                std::max(start.x, end.x) + pad, std::max(start.y, end.y) + pad))) {
            return;
        }
        if (SkCanvas* canvas = Canvas()) {
            SkPaint paint;
            paint.setColor(ToSkColor(color));
            paint.setStyle(SkPaint::kStroke_Style);
            paint.setStrokeWidth(strokeWidth);
            paint.setAntiAlias(true);
            canvas->drawLine(start.x, start.y, end.x, end.y, paint);
        }
    }

    void DrawDashedLine(const PointF& start,
                        const PointF& end,
                        const Color& color,
                        float strokeWidth,
                        float dashLength,
                        float gapLength) override {
        const float pad = strokeWidth * 0.5f + 2.0f;
        if (!IntersectsDirtyRect(Rect::Make(
                std::min(start.x, end.x) - pad, std::min(start.y, end.y) - pad,
                std::max(start.x, end.x) + pad, std::max(start.y, end.y) + pad))) {
            return;
        }
        if (SkCanvas* canvas = Canvas()) {
            SkPaint paint;
            paint.setColor(ToSkColor(color));
            paint.setStyle(SkPaint::kStroke_Style);
            paint.setStrokeWidth(strokeWidth);
            paint.setAntiAlias(true);
            const float d = std::max(1.0f, dashLength);
            const float g = std::max(1.0f, gapLength);
            const uint64_t key =
                (static_cast<uint64_t>(static_cast<uint32_t>(std::lround(d * 100.0f))) << 32) |
                static_cast<uint64_t>(static_cast<uint32_t>(std::lround(g * 100.0f)));
            sk_sp<SkPathEffect> effect;
            if (auto it = m_dashEffectCache.find(key); it != m_dashEffectCache.end()) {
                effect = it->second;
            } else {
                const SkScalar intervals[2] = {d, g};
                effect = SkDashPathEffect::Make(intervals, 2, 0.0f);
                if (m_dashEffectCache.size() >= 64) {
                    m_dashEffectCache.clear();
                }
                m_dashEffectCache.emplace(key, effect);
            }
            paint.setPathEffect(std::move(effect));
            canvas->drawLine(start.x, start.y, end.x, end.y, paint);
        }
    }

    void DrawPolyline(const std::vector<PointF>& points, const Color& color, float strokeWidth) override {
        if (points.size() < 2) {
            return;
        }
        if (SkCanvas* canvas = Canvas()) {
            SkPaint paint;
            paint.setColor(ToSkColor(color));
            paint.setStyle(SkPaint::kStroke_Style);
            paint.setStrokeWidth(strokeWidth);
            paint.setStrokeCap(SkPaint::kRound_Cap);
            paint.setStrokeJoin(SkPaint::kRound_Join);
            paint.setAntiAlias(true);

            SkPath path;
            path.moveTo(points.front().x, points.front().y);
            for (size_t i = 1; i < points.size(); ++i) {
                path.lineTo(points[i].x, points[i].y);
            }
            canvas->drawPath(path, paint);
        }
    }

    void FillPolygon(const std::vector<PointF>& points, const Color& color) override {
        if (points.size() < 3) {
            return;
        }
        if (SkCanvas* canvas = Canvas()) {
            SkPaint paint;
            paint.setColor(ToSkColor(color));
            paint.setStyle(SkPaint::kFill_Style);
            paint.setAntiAlias(true);
            SkPath path;
            path.moveTo(points.front().x, points.front().y);
            for (size_t i = 1; i < points.size(); ++i) {
                path.lineTo(points[i].x, points[i].y);
            }
            path.close();
            canvas->drawPath(path, paint);
        }
    }

    void FillPolygonLinearGradient(const std::vector<PointF>& points,
                                   float topY,
                                   float bottomY,
                                   const Color& topColor,
                                   const Color& bottomColor) override {
        if (points.size() < 3) {
            return;
        }
        if (SkCanvas* canvas = Canvas()) {
            const SkPoint pts[2] = {
                SkPoint::Make(0.0f, topY),
                SkPoint::Make(0.0f, std::max(topY + 1.0f, bottomY)),
            };
            const SkColor colors[2] = {ToSkColor(topColor), ToSkColor(bottomColor)};
            SkPaint paint;
            paint.setStyle(SkPaint::kFill_Style);
            paint.setAntiAlias(true);
            paint.setShader(SkGradientShader::MakeLinear(pts, colors, nullptr, 2, SkTileMode::kClamp));
            SkPath path;
            path.moveTo(points.front().x, points.front().y);
            for (size_t i = 1; i < points.size(); ++i) {
                path.lineTo(points[i].x, points[i].y);
            }
            path.close();
            canvas->drawPath(path, paint);
        }
    }

    void DrawSvgPath(const std::string& svgPathData,
                     float viewBoxWidth,
                     float viewBoxHeight,
                     const Rect& destRect,
                     const Color& fill,
                     const Color& stroke,
                     float strokeWidth) override {
        SkCanvas* canvas = Canvas();
        if (!canvas || svgPathData.empty() || destRect.Width() <= 0.5f || destRect.Height() <= 0.5f) {
            return;
        }
        SkPath path;
        if (auto it = m_svgPathCache.find(svgPathData); it != m_svgPathCache.end()) {
            path = it->second;
        } else {
            if (!SkParsePath::FromSVGString(svgPathData.c_str(), &path)) {
                return;
            }
            if (m_svgPathCache.size() >= 256) {
                m_svgPathCache.clear();
            }
            m_svgPathCache.emplace(svgPathData, path);
        }
        const float vbW = std::max(1.0f, viewBoxWidth);
        const float vbH = std::max(1.0f, viewBoxHeight);
        const float scale = std::min(destRect.Width() / vbW, destRect.Height() / vbH);
        const float offsetX = destRect.left + (destRect.Width() - vbW * scale) * 0.5f;
        const float offsetY = destRect.top + (destRect.Height() - vbH * scale) * 0.5f;

        SkAutoCanvasRestore restore(canvas, /*doSave=*/true);
        canvas->translate(offsetX, offsetY);
        canvas->scale(scale, scale);

        if (fill.a > 0.001f) {
            SkPaint fillPaint;
            fillPaint.setColor(ToSkColor(fill));
            fillPaint.setStyle(SkPaint::kFill_Style);
            fillPaint.setAntiAlias(true);
            canvas->drawPath(path, fillPaint);
        }
        if (stroke.a > 0.001f && strokeWidth > 0.0f) {
            SkPaint strokePaint;
            strokePaint.setColor(ToSkColor(stroke));
            strokePaint.setStyle(SkPaint::kStroke_Style);
            // Keep strokeWidth in destination pixel space by dividing by scale
            strokePaint.setStrokeWidth(strokeWidth / std::max(0.01f, scale));
            strokePaint.setStrokeCap(SkPaint::kRound_Cap);
            strokePaint.setStrokeJoin(SkPaint::kRound_Join);
            strokePaint.setAntiAlias(true);
            canvas->drawPath(path, strokePaint);
        }
    }

    void PushRoundedClip(const Rect& rect, float radius) override {
        if (SkCanvas* canvas = Canvas()) {
            canvas->save();
            SkRRect clip;
            clip.setRectXY(ToSkRect(rect), radius, radius);
            canvas->clipRRect(clip, true);
        }
    }

    void PopLayer() override {
        if (SkCanvas* canvas = Canvas()) {
            canvas->restore();
        }
    }

    void DrawTextW(const wchar_t* text,
                   UINT32 len,
                   const Rect& rect,
                   const Color& color,
                   float fontSize,
                   const TextRenderOptions& options) override {
        if (!text || len == 0 || fontSize <= 0.0f || rect.Width() <= 0.0f || rect.Height() <= 0.0f) {
            return;
        }
        if (!IntersectsDirtyRect(Rect::Make(
                rect.left - 2.0f, rect.top - 2.0f,
                rect.right + 2.0f, rect.bottom + 2.0f))) {
            return;
        }

        SkCanvas* canvas = Canvas();
        if (!canvas) {
            return;
        }

        EnsureFontManager();

        const uint16_t effectiveWeight = options.EffectiveWeight();
        sk_sp<SkTypeface> styleFace = m_defaultTypeface;
        if (options.bold || options.italic || effectiveWeight != 400 || !options.fontFamily.empty()) {
            styleFace = skia_font::CreateDefaultTypeface(
                m_fontMgr.get(), options.bold, options.italic, effectiveWeight, options.fontFamily);
            if (!styleFace) {
                styleFace = m_defaultTypeface;
            }
        }
        if (!styleFace) {
            return;
        }

        SkPaint paint;
        paint.setColor(ToSkColor(color));
        paint.setStyle(SkPaint::kFill_Style);
        paint.setAntiAlias(true);

        const bool hasNewline =
            (std::wmemchr(text, L'\n', len) != nullptr) ||
            (std::wmemchr(text, L'\r', len) != nullptr);
        if (!hasNewline) {
            const TextBlobCacheValue& entry = GetOrCreateTextBlob(styleFace.get(), fontSize, text, len);
            const float widthBudget = std::max(1.0f, rect.Width());
            const bool fitsSingleLine =
                (options.wrap == TextWrapMode::NoWrap && !options.ellipsis) ||
                (options.wrap == TextWrapMode::Wrap && (widthBudget <= 1.0f || entry.width <= widthBudget)) ||
                (options.wrap == TextWrapMode::NoWrap && options.ellipsis && entry.width <= widthBudget + 0.5f);
            if (fitsSingleLine) {
                if (!entry.blob) {
                    return;
                }
                const CachedFontMetrics metrics = GetOrCreateFontMetrics(styleFace.get(), fontSize);
                float startTop = rect.top;
                if (options.verticalAlign == TextVerticalAlign::Center) {
                    startTop = rect.top + std::max(0.0f, (rect.Height() - metrics.textHeight) * 0.5f);
                } else if (options.verticalAlign == TextVerticalAlign::End) {
                    startTop = rect.bottom - metrics.textHeight;
                }
                const float baseline = startTop + metrics.ascent;

                float drawX = rect.left;
                if (options.horizontalAlign == TextHorizontalAlign::Center) {
                    drawX = rect.left + std::max(0.0f, (rect.Width() - entry.width) * 0.5f);
                } else if (options.horizontalAlign == TextHorizontalAlign::End) {
                    drawX = rect.right - entry.width;
                }

                canvas->save();
                canvas->clipRect(ToSkRect(rect), false);
                canvas->drawTextBlob(entry.blob.get(), drawX, baseline, paint);
                canvas->restore();
                return;
            }
        }

        std::shared_ptr<const SkiaTextLayout> layout = AcquireSkiaTextLayout(
            text,
            len,
            fontSize,
            std::max(1.0f, rect.Width()),
            options.wrap,
            options.bold,
            options.italic,
            options.ellipsis,
            effectiveWeight,
            options.fontFamily,
            options.lineHeightFactor);
        if (!layout || layout->lines.empty()) {
            return;
        }

        float startTop = rect.top;
        if (options.verticalAlign == TextVerticalAlign::Center) {
            startTop = rect.top + std::max(0.0f, (rect.Height() - layout->blockHeight) * 0.5f);
        } else if (options.verticalAlign == TextVerticalAlign::End) {
            startTop = rect.bottom - layout->blockHeight;
        }

        canvas->save();
        canvas->clipRect(ToSkRect(rect), false);

        for (size_t i = 0; i < layout->lines.size(); ++i) {
            const float baseline = startTop + layout->ascent + static_cast<float>(i) * layout->lineHeight;
            if (baseline - layout->ascent > rect.bottom) {
                break;
            }
            const auto& line = layout->lines[i];
            if (line.text.empty()) {
                continue;
            }

            float drawX = rect.left;
            if (options.horizontalAlign == TextHorizontalAlign::Center) {
                drawX = rect.left + std::max(0.0f, (rect.Width() - line.width) * 0.5f);
            } else if (options.horizontalAlign == TextHorizontalAlign::End) {
                drawX = rect.right - line.width;
            }

            const TextBlobCacheValue& lineEntry = GetOrCreateTextBlob(
                styleFace.get(),
                fontSize,
                line.text.c_str(),
                static_cast<uint32_t>(line.text.size()));
            if (lineEntry.blob) {
                canvas->drawTextBlob(lineEntry.blob.get(), drawX, baseline, paint);
            }
        }

        canvas->restore();
    }

    Size GetBitmapSize(BitmapHandle bitmap) override {
        return bitmap ? bitmap->GetSize() : Size{};
    }

    BitmapHandle CreateBitmapFromBytes(const uint8_t* data, size_t size) override {
        if (!data || size == 0) {
            return nullptr;
        }

        sk_sp<SkData> encoded = SkData::MakeWithCopy(data, size);
        if (!encoded) {
            return nullptr;
        }

        sk_sp<SkImage> image = SkImages::DeferredFromEncodedData(encoded);
        if (!image) {
            return nullptr;
        }

        return std::make_shared<SkiaBitmapResource>(std::move(image));
    }

    void DrawBitmap(BitmapHandle bitmap, const Rect& rect) override {
        SkCanvas* canvas = Canvas();
        if (!canvas || !bitmap) {
            return;
        }

        auto skiaBitmap = std::dynamic_pointer_cast<SkiaBitmapResource>(bitmap);
        if (!skiaBitmap || !skiaBitmap->Image()) {
            return;
        }

        canvas->drawImageRect(
            skiaBitmap->Image(),
            ToSkRect(rect),
            m_bitmapSampling,
            nullptr);
    }

    SvgHandle CreateSvgFromBytes(const uint8_t* data, size_t size, const char* cacheKey) override {
        if (!data || size == 0) {
            return nullptr;
        }
        std::string key;
        if (cacheKey && cacheKey[0]) {
            key = cacheKey;
        } else {
            uint64_t h = 14695981039346656037ull;
            for (size_t i = 0; i < size; ++i) {
                h ^= static_cast<uint64_t>(data[i]);
                h *= 1099511628211ull;
            }
            key = "bytes:" + std::to_string(h) + ":" + std::to_string(size);
        }
        auto it = m_svgCacheMap.find(key);
        if (it != m_svgCacheMap.end()) {
            m_svgCacheList.splice(m_svgCacheList.begin(), m_svgCacheList, it->second);
            return it->second->handle;
        }

        EnsureFontManager();
        auto stream = SkMemoryStream::MakeCopy(data, size);
        if (!stream) {
            return nullptr;
        }
        sk_sp<SkSVGDOM> dom = SkSVGDOM::Builder()
            .setFontManager(m_fontMgr)
            .make(*stream);
        if (!dom) {
            return nullptr;
        }
        auto handle = std::make_shared<SkiaSvgResource>(std::move(dom));
        if (m_svgCacheList.size() >= kMaxSvgCacheSize) {
            auto last = m_svgCacheList.end();
            --last;
            m_svgCacheMap.erase(last->key);
            m_svgCacheList.pop_back();
        }
        m_svgCacheList.push_front(SvgCacheEntry{key, handle});
        m_svgCacheMap[key] = m_svgCacheList.begin();
        return handle;
    }

    Size GetSvgSize(SvgHandle svg) override {
        return svg ? svg->GetIntrinsicSize() : Size{};
    }

    void DrawSvg(SvgHandle svg, const Rect& rect, bool hasTint, const Color& tint) override {
        SkCanvas* canvas = Canvas();
        if (!canvas || !svg) {
            return;
        }
        auto skiaSvg = std::dynamic_pointer_cast<SkiaSvgResource>(svg);
        if (!skiaSvg || !skiaSvg->Dom()) {
            return;
        }
        SkAutoCanvasRestore restore(canvas, /*doSave=*/true);
        if (hasTint && tint.a > 0.001f) {
            SkPaint layerPaint;
            layerPaint.setColorFilter(SkColorFilters::Blend(ToSkColor(tint), SkBlendMode::kSrcIn));
            const SkRect layerBounds = ToSkRect(rect);
            canvas->saveLayer(&layerBounds, &layerPaint);
        }
        canvas->translate(rect.left, rect.top);
        skiaSvg->Dom()->setContainerSize(SkSize::Make(rect.Width(), rect.Height()));
        skiaSvg->Dom()->render(canvas);
        if (hasTint && tint.a > 0.001f) {
            canvas->restore();
        }
    }

private:
    void EnsureFontManager() {
        if (!m_fontMgr) {
            m_fontMgr = skia_font::CreateDefaultFontManager();
        }
        if (!m_defaultTypeface) {
            m_defaultTypeface = skia_font::CreateDefaultTypeface(m_fontMgr.get());
        }
    }

    bool EnsureSurface() {
        return m_surface || RecreateSurface();
    }

    void ReleasePresentSurface() {
        m_surface.reset();
        if (m_memDC) {
            if (m_oldBitmap) {
                SelectObject(m_memDC, m_oldBitmap);
                m_oldBitmap = nullptr;
            }
            DeleteDC(m_memDC);
            m_memDC = nullptr;
        }
        if (m_dib) {
            DeleteObject(m_dib);
            m_dib = nullptr;
        }
        m_dibBits = nullptr;
        m_dibCapWidth = 0;
        m_dibCapHeight = 0;
    }

    bool CreatePresentSurface(UINT capWidth, UINT capHeight) {
        ReleasePresentSurface();
        if (capWidth == 0 || capHeight == 0) {
            return false;
        }

        HDC screenDc = GetDC(nullptr);
        if (!screenDc) {
            return false;
        }
        m_memDC = CreateCompatibleDC(screenDc);
        ReleaseDC(nullptr, screenDc);
        if (!m_memDC) {
            return false;
        }

        BITMAPINFO bmi{};
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = static_cast<LONG>(capWidth);
        bmi.bmiHeader.biHeight = -static_cast<LONG>(capHeight); // top-down
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;

        m_dib = CreateDIBSection(m_memDC, &bmi, DIB_RGB_COLORS, &m_dibBits, nullptr, 0);
        if (!m_dib || !m_dibBits) {
            ReleasePresentSurface();
            return false;
        }
        m_oldBitmap = static_cast<HBITMAP>(SelectObject(m_memDC, m_dib));
        m_dibCapWidth = capWidth;
        m_dibCapHeight = capHeight;
        return true;
    }

    void PresentLayered() {
        if (!m_memDC) {
            return;
        }
        POINT src{0, 0};
        SIZE size{static_cast<LONG>(m_width), static_cast<LONG>(m_height)};
        BLENDFUNCTION blend{};
        blend.BlendOp = AC_SRC_OVER;
        blend.SourceConstantAlpha = 255;
        blend.AlphaFormat = AC_SRC_ALPHA;
        UpdateLayeredWindow(
            m_hwnd,
            nullptr,
            nullptr,
            &size,
            m_memDC,
            &src,
            0,
            &blend,
            ULW_ALPHA);
    }

    void PresentHwnd() {
        if (!m_memDC) {
            return;
        }
        HDC dc = GetDC(m_hwnd);
        if (!dc) {
            return;
        }
        if (HasDirtyRect()) {
            for (const Rect& d : GetDirtyRects()) {
                const int bx = std::max(0, static_cast<int>(std::floor(d.left)));
                const int by = std::max(0, static_cast<int>(std::floor(d.top)));
                const int br = std::min(static_cast<int>(m_width), static_cast<int>(std::ceil(d.right)));
                const int bb = std::min(static_cast<int>(m_height), static_cast<int>(std::ceil(d.bottom)));
                const int bw = br - bx;
                const int bh = bb - by;
                if (bw > 0 && bh > 0) {
                    BitBlt(
                        dc,
                        bx,
                        by,
                        bw,
                        bh,
                        m_memDC,
                        bx,
                        by,
                        SRCCOPY);
                }
            }
        } else if (m_width > 0 && m_height > 0) {
            BitBlt(
                dc,
                0,
                0,
                static_cast<int>(m_width),
                static_cast<int>(m_height),
                m_memDC,
                0,
                0,
                SRCCOPY);
        }
        ReleaseDC(m_hwnd, dc);
    }

    struct TextBlobCacheKey {
        uint32_t typefaceId = 0;
        int32_t fontSizeQ = 0;
        std::wstring text;

        bool operator==(const TextBlobCacheKey& o) const {
            return typefaceId == o.typefaceId && fontSizeQ == o.fontSizeQ && text == o.text;
        }
    };

    struct TextBlobCacheKeyHash {
        size_t operator()(const TextBlobCacheKey& k) const {
            size_t h = std::hash<std::wstring>{}(k.text);
            h ^= std::hash<uint32_t>{}(k.typefaceId) + 0x9e3779b9 + (h << 6) + (h >> 2);
            h ^= std::hash<int32_t>{}(k.fontSizeQ) + 0x9e3779b9 + (h << 6) + (h >> 2);
            return h;
        }
    };

    struct TextBlobCacheValue {
        sk_sp<SkTextBlob> blob;
        float width = 0.0f;
    };

    struct TextBlobLruEntry {
        TextBlobCacheKey key;
        TextBlobCacheValue value;
    };

    struct FontMetricsCacheKey {
        uint32_t typefaceId = 0;
        int32_t fontSizeQ = 0;

        bool operator==(const FontMetricsCacheKey& o) const {
            return typefaceId == o.typefaceId && fontSizeQ == o.fontSizeQ;
        }
    };

    struct FontMetricsCacheKeyHash {
        size_t operator()(const FontMetricsCacheKey& k) const {
            return (static_cast<size_t>(k.typefaceId) * 2654435761u) ^
                   (static_cast<size_t>(static_cast<uint32_t>(k.fontSizeQ)) << 1);
        }
    };

    struct CachedFontMetrics {
        float ascent = 0.0f;
        float descent = 0.0f;
        float spacing = 0.0f;
        float textHeight = 0.0f;
    };

    CachedFontMetrics GetOrCreateFontMetrics(SkTypeface* typeface, float fontSize) {
        const uint32_t tid = typeface ? typeface->uniqueID() : 0u;
        const int32_t fsQ = static_cast<int32_t>(std::lround(fontSize * 64.0f));
        const FontMetricsCacheKey key{tid, fsQ};
        if (auto it = m_fontMetricsCache.find(key); it != m_fontMetricsCache.end()) {
            return it->second;
        }
        SkFont font = skia_font::CreateSkiaFont(fontSize, typeface);
        SkFontMetrics metrics{};
        font.getMetrics(&metrics);
        CachedFontMetrics out;
        out.ascent = std::isfinite(metrics.fAscent) ? -metrics.fAscent : fontSize * 0.8f;
        out.descent = std::isfinite(metrics.fDescent) ? metrics.fDescent : fontSize * 0.25f;
        out.textHeight = out.ascent + out.descent;
        out.spacing = std::max(font.getSpacing(), out.textHeight);
        if (m_fontMetricsCache.size() >= 256) {
            m_fontMetricsCache.clear();
        }
        m_fontMetricsCache.emplace(key, out);
        return out;
    }

    sk_sp<SkTextBlob> BuildLineTextBlob(SkTypeface* defaultTypeface,
                                        float fontSize,
                                        const wchar_t* text,
                                        uint32_t len,
                                        float* outWidth) {
        if (outWidth) {
            *outWidth = 0.0f;
        }
        if (!text || len == 0 || fontSize <= 0.0f) {
            return nullptr;
        }

        constexpr uint32_t kStackCap = 128;
        if (defaultTypeface) {
            SkFont defaultFont = skia_font::CreateSkiaFont(fontSize, defaultTypeface);
            SkGlyphID stackGlyphs[kStackCap];
            std::vector<SkGlyphID> heapGlyphs;
            SkGlyphID* glyphs = stackGlyphs;
            if (len > kStackCap) {
                heapGlyphs.resize(len);
                glyphs = heapGlyphs.data();
            }
            const int glyphCount = defaultFont.textToGlyphs(
                text,
                static_cast<size_t>(len) * sizeof(wchar_t),
                SkTextEncoding::kUTF16,
                glyphs,
                static_cast<int>(len));
            if (glyphCount > 0) {
                bool allSupported = true;
                for (int i = 0; i < glyphCount; ++i) {
                    if (glyphs[i] == 0) {
                        allSupported = false;
                        break;
                    }
                }
                if (allSupported) {
                    SkScalar stackWidths[kStackCap];
                    std::vector<SkScalar> heapWidths;
                    SkScalar* widths = stackWidths;
                    if (static_cast<uint32_t>(glyphCount) > kStackCap) {
                        heapWidths.resize(static_cast<size_t>(glyphCount));
                        widths = heapWidths.data();
                    }
                    defaultFont.getWidths(glyphs, glyphCount, widths);
                    SkTextBlobBuilder builder;
                    const SkTextBlobBuilder::RunBuffer& run = builder.allocRunPosH(defaultFont, glyphCount, 0.0f);
                    std::memcpy(run.glyphs, glyphs, static_cast<size_t>(glyphCount) * sizeof(SkGlyphID));
                    float x = 0.0f;
                    for (int i = 0; i < glyphCount; ++i) {
                        run.pos[i] = x;
                        x += widths[i];
                    }
                    if (outWidth) {
                        *outWidth = x;
                    }
                    return builder.make();
                }
            }
        }

        SkTextBlobBuilder builder;
        float runX = 0.0f;
        std::vector<SkGlyphID> runHeapGlyphs;
        std::vector<SkScalar> runHeapWidths;
        skia_font::IterateTextRuns(
            m_fontMgr.get(),
            defaultTypeface,
            text,
            len,
            [&](const wchar_t* runText, uint32_t runLen, SkTypeface* runFace) {
                SkFont runFont = skia_font::CreateSkiaFont(fontSize, runFace);
                SkGlyphID runStackGlyphs[kStackCap];
                SkGlyphID* runGlyphs = runStackGlyphs;
                if (runLen > kStackCap) {
                    runHeapGlyphs.resize(runLen);
                    runGlyphs = runHeapGlyphs.data();
                }
                const int runGlyphCount = runFont.textToGlyphs(
                    runText,
                    static_cast<size_t>(runLen) * sizeof(wchar_t),
                    SkTextEncoding::kUTF16,
                    runGlyphs,
                    static_cast<int>(runLen));
                if (runGlyphCount <= 0) {
                    return;
                }
                SkScalar runStackWidths[kStackCap];
                SkScalar* runWidths = runStackWidths;
                if (static_cast<uint32_t>(runGlyphCount) > kStackCap) {
                    runHeapWidths.resize(static_cast<size_t>(runGlyphCount));
                    runWidths = runHeapWidths.data();
                }
                runFont.getWidths(runGlyphs, runGlyphCount, runWidths);
                const SkTextBlobBuilder::RunBuffer& run = builder.allocRunPosH(runFont, runGlyphCount, 0.0f);
                std::memcpy(run.glyphs, runGlyphs, static_cast<size_t>(runGlyphCount) * sizeof(SkGlyphID));
                for (int i = 0; i < runGlyphCount; ++i) {
                    run.pos[i] = runX;
                    runX += runWidths[i];
                }
            });
        if (outWidth) {
            *outWidth = runX;
        }
        return builder.make();
    }

    const TextBlobCacheValue& GetOrCreateTextBlob(SkTypeface* typeface,
                                                  float fontSize,
                                                  const wchar_t* text,
                                                  uint32_t len) {
        static const TextBlobCacheValue s_empty{};
        if (!text || len == 0 || fontSize <= 0.0f) {
            return s_empty;
        }
        m_blobLookupKey.typefaceId = typeface ? typeface->uniqueID() : 0u;
        m_blobLookupKey.fontSizeQ = static_cast<int32_t>(std::lround(fontSize * 64.0f));
        m_blobLookupKey.text.assign(text, len);

        if (auto it = m_textBlobMap.find(m_blobLookupKey); it != m_textBlobMap.end()) {
            m_textBlobList.splice(m_textBlobList.begin(), m_textBlobList, it->second);
            return it->second->value;
        }

        TextBlobCacheValue built;
        built.blob = BuildLineTextBlob(typeface, fontSize, text, len, &built.width);

        if (m_textBlobList.size() >= kMaxTextBlobCacheSize) {
            auto last = std::prev(m_textBlobList.end());
            m_textBlobMap.erase(last->key);
            m_textBlobList.pop_back();
        }
        TextBlobCacheKey storedKey = m_blobLookupKey;
        m_textBlobList.push_front(TextBlobLruEntry{storedKey, std::move(built)});
        m_textBlobMap.emplace(std::move(storedKey), m_textBlobList.begin());
        return m_textBlobList.front().value;
    }

    bool RecreateSurface() {
        if (!m_hwnd) {
            return false;
        }

        if (m_width == 0 || m_height == 0) {
            RECT clientRect{};
            GetClientRect(m_hwnd, &clientRect);
            m_width = static_cast<UINT>(std::max<LONG>(1, clientRect.right - clientRect.left));
            m_height = static_cast<UINT>(std::max<LONG>(1, clientRect.bottom - clientRect.top));
        }

        if (m_width == 0 || m_height == 0) {
            ReleasePresentSurface();
            return false;
        }

        const UINT desiredCapW = (m_presentMode == PresentMode::Layered)
            ? m_width
            : ((m_width + 63u) & ~63u);
        const UINT desiredCapH = (m_presentMode == PresentMode::Layered)
            ? m_height
            : ((m_height + 63u) & ~63u);

        const bool needRealloc = !m_dibBits || !m_memDC ||
            (m_presentMode == PresentMode::Layered
                ? (m_dibCapWidth != desiredCapW || m_dibCapHeight != desiredCapH)
                : (m_width > m_dibCapWidth || m_height > m_dibCapHeight ||
                   m_dibCapWidth > desiredCapW + 256u || m_dibCapHeight > desiredCapH + 256u));

        if (needRealloc) {
            if (!CreatePresentSurface(desiredCapW, desiredCapH)) {
                return false;
            }
        }

        const size_t rowBytes = static_cast<size_t>(m_dibCapWidth) * 4;
        const SkImageInfo info = SkImageInfo::MakeN32Premul(static_cast<int>(m_width), static_cast<int>(m_height));
        const SkSurfaceProps surfaceProps(
            0,
            m_presentMode == PresentMode::Layered ? kUnknown_SkPixelGeometry : kRGB_H_SkPixelGeometry);
        m_surface = SkSurfaces::WrapPixels(info, m_dibBits, rowBytes, &surfaceProps);
        m_frameDirty = true;
        m_surfaceNeedsFullPaint = true;
        return static_cast<bool>(m_surface);
    }

    SkCanvas* Canvas() {
        return EnsureSurface() ? m_surface->getCanvas() : nullptr;
    }

    HWND m_hwnd = nullptr;
    PresentMode m_presentMode = PresentMode::Hwnd;
    UINT m_width = 0;
    UINT m_height = 0;
    struct ShadowCacheKey {
        int32_t wQ = 0;
        int32_t hQ = 0;
        int32_t rQ = 0;
        int32_t bQ = 0;
        SkColor color = 0;
        bool operator==(const ShadowCacheKey& o) const {
            return wQ == o.wQ && hQ == o.hQ && rQ == o.rQ && bQ == o.bQ && color == o.color;
        }
    };
    struct ShadowCacheKeyHash {
        size_t operator()(const ShadowCacheKey& k) const {
            size_t h = std::hash<int32_t>{}(k.wQ);
            h ^= std::hash<int32_t>{}(k.hQ) + 0x9e3779b9 + (h << 6) + (h >> 2);
            h ^= std::hash<int32_t>{}((k.rQ << 16) ^ k.bQ) + 0x9e3779b9 + (h << 6) + (h >> 2);
            h ^= std::hash<uint32_t>{}(k.color) + 0x9e3779b9 + (h << 6) + (h >> 2);
            return h;
        }
    };
    struct ShadowCacheEntry {
        sk_sp<SkImage> image;
        int pad = 0;
        int hits = 0;
    };

    UINT m_dibCapWidth = 0;
    UINT m_dibCapHeight = 0;
    bool m_frameDirty = false;
    bool m_surfaceNeedsFullPaint = true;
    std::unordered_map<std::string, SkPath> m_svgPathCache;
    std::unordered_map<ShadowCacheKey, ShadowCacheEntry, ShadowCacheKeyHash> m_shadowCache;
    HDC m_memDC = nullptr;
    HBITMAP m_dib = nullptr;
    HBITMAP m_oldBitmap = nullptr;
    void* m_dibBits = nullptr;
    sk_sp<SkSurface> m_surface;
    sk_sp<SkFontMgr> m_fontMgr;
    sk_sp<SkTypeface> m_defaultTypeface;
    // Declared AFTER m_fontMgr and m_defaultTypeface so cached blobs release before COM/FontMgr teardown.
    static constexpr size_t kMaxTextBlobCacheSize = 4096;
    std::list<TextBlobLruEntry> m_textBlobList;
    std::unordered_map<TextBlobCacheKey, std::list<TextBlobLruEntry>::iterator, TextBlobCacheKeyHash> m_textBlobMap;
    TextBlobCacheKey m_blobLookupKey;
    std::unordered_map<FontMetricsCacheKey, CachedFontMetrics, FontMetricsCacheKeyHash> m_fontMetricsCache;
    std::unordered_map<uint64_t, sk_sp<SkPathEffect>> m_dashEffectCache;
    SkSamplingOptions m_bitmapSampling{SkFilterMode::kLinear, SkMipmapMode::kLinear};
    struct SvgCacheEntry {
        std::string key;
        SvgHandle handle;
    };
    static constexpr size_t kMaxSvgCacheSize = 256;
    std::list<SvgCacheEntry> m_svgCacheList;
    std::unordered_map<std::string, std::list<SvgCacheEntry>::iterator> m_svgCacheMap;
};

} // namespace

std::unique_ptr<IRenderer> CreateSkiaRenderer() {
    return std::make_unique<SkiaRenderer>();
}

#else

std::unique_ptr<IRenderer> CreateSkiaRenderer() {
    return nullptr;
}

#endif
