#include "dirty_region.h"
#include "renderer.h"
#include "skia_font_shared.h"
#include "skia_text_layout.h"

#include <cmath>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

namespace {

bool Check(bool cond, const char* msg) {
    if (!cond) {
        std::printf("[FAIL] %s\n", msg);
    }
    return cond;
}

class DummyRenderer final : public IRenderer {
public:
    RendererBackend Backend() const override { return RendererBackend::Skia; }
    bool Initialize(HWND) override { return true; }
    void Resize(UINT, UINT) override {}
    void BeginFrame(const Color&) override {}
    void EndFrame() override {}
    void FillRect(const Rect&, const Color&) override {}
    void FillRoundedRect(const Rect&, const Color&, float) override {}
    void FillLinearGradient(const Rect&, float, const Color&, const Color&, float) override {}
    void FillSoftShadow(const Rect&, float, float, float, float, const Color&) override {}
    void DrawRect(const Rect&, const Color&, float) override {}
    void DrawRoundedRect(const Rect&, const Color&, float, float) override {}
    void DrawLine(const PointF&, const PointF&, const Color&, float) override {}
    void DrawPolyline(const std::vector<PointF>&, const Color&, float) override {}
    void FillPolygon(const std::vector<PointF>&, const Color&) override {}
    void PushRoundedClip(const Rect&, float) override {}
    void PopLayer() override {}
    void DrawTextW(const wchar_t*, UINT32, const Rect&, const Color&, float, const TextRenderOptions&) override {}
    Size GetBitmapSize(BitmapHandle) override { return {}; }
    BitmapHandle CreateBitmapFromBytes(const uint8_t*, size_t) override { return nullptr; }
    void DrawBitmap(BitmapHandle, const Rect&) override {}
    SvgHandle CreateSvgFromBytes(const uint8_t*, size_t, const char*) override { return nullptr; }
    Size GetSvgSize(SvgHandle) override { return {}; }
    void DrawSvg(SvgHandle, const Rect&, bool, const Color&) override {}
};

bool TestDirtyRegionDisjointCornersAndCulling() {
    ai_win_ui::DirtyRegion dr;
    // Top-right market quotes panel + bottom-left status bar
    dr.AddRect(Rect::Make(1000.0f, 30.0f, 1270.0f, 260.0f));
    dr.AddRect(Rect::Make(10.0f, 790.0f, 360.0f, 818.0f));
    dr.NormalizeForViewport(1280.0f, 820.0f);

    if (!Check(!dr.IsEmpty(), "disjoint corners: not empty")) return false;
    if (!Check(!dr.IsFull(), "disjoint corners: not full")) return false;
    if (!Check(dr.Rects().size() == 2, "disjoint corners: 2 separate rects kept")) return false;
    // Large static chart in center must be culled even though it sits inside the union bbox
    if (!Check(!dr.Intersects(Rect::Make(60.0f, 100.0f, 898.0f, 658.0f)), "disjoint corners: center chart culled")) return false;
    if (!Check(dr.Intersects(Rect::Make(1010.0f, 40.0f, 1100.0f, 80.0f)), "disjoint corners: top-right intersects")) return false;
    if (!Check(dr.Intersects(Rect::Make(20.0f, 795.0f, 100.0f, 815.0f)), "disjoint corners: bottom-left intersects")) return false;
    if (!Check(dr.CoverageRatio(1280.0f, 820.0f) < 0.15f, "disjoint corners: coverage < 15%")) return false;

    std::printf("[PASS] TestDirtyRegionDisjointCornersAndCulling\n");
    return true;
}

bool TestDirtyRegionGridCoalescingAndCap() {
    ai_win_ui::DirtyRegion grid;
    // 10x10 compact grid cells (padded by 2px like UIElement::InvalidateSelf(2.0f))
    for (int r = 0; r < 10; ++r) {
        for (int c = 0; c < 10; ++c) {
            const float x = 20.0f + static_cast<float>(c) * 64.0f;
            const float y = 80.0f + static_cast<float>(r) * 26.0f;
            grid.AddRect(Rect::Make(x - 2.0f, y - 2.0f, x + 62.0f, y + 24.0f));
        }
    }
    grid.NormalizeForViewport(1000.0f, 700.0f);
    if (!Check(!grid.IsEmpty() && !grid.IsFull(), "grid: valid partial")) return false;
    if (!Check(grid.Rects().size() <= ai_win_ui::DirtyRegion::kMaxRects, "grid: rect count <= kMaxRects")) return false;
    if (!Check(grid.Rects().size() == 1, "grid: 100 adjacent cells coalesce cleanly")) return false;
    if (!Check(grid.Intersects(Rect::Make(22.0f, 82.0f, 78.0f, 100.0f)), "grid: first cell intersects")) return false;
    if (!Check(!grid.Intersects(Rect::Make(900.0f, 600.0f, 980.0f, 680.0f)), "grid: unrelated corner culled")) return false;

    // Scattered rects exceeding kMaxRects must stay capped at kMaxRects
    ai_win_ui::DirtyRegion scattered;
    for (int i = 0; i < 20; ++i) {
        const float x = static_cast<float>((i % 5) * 400 + 10);
        const float y = static_cast<float>((i / 5) * 400 + 10);
        scattered.AddRect(Rect::Make(x, y, x + 20.0f, y + 20.0f));
    }
    scattered.NormalizeForViewport(2000.0f, 2000.0f);
    if (!Check(scattered.Rects().size() <= ai_win_ui::DirtyRegion::kMaxRects, "scattered: capped at kMaxRects")) return false;

    // >= 85% coverage collapses to full repaint
    ai_win_ui::DirtyRegion nearlyFull;
    nearlyFull.AddRect(Rect::Make(10.0f, 10.0f, 950.0f, 660.0f)); // ~87% of 1000x700
    nearlyFull.NormalizeForViewport(1000.0f, 700.0f);
    if (!Check(nearlyFull.IsFull(), "coverage >= 85%: collapses to IsFull()")) return false;

    std::printf("[PASS] TestDirtyRegionGridCoalescingAndCap\n");
    return true;
}

bool TestRendererDirtyRectsInterface() {
    DummyRenderer r;
    if (!Check(!r.HasDirtyRect(), "initial HasDirtyRect false")) return false;
    if (!Check(r.IntersectsDirtyRect(Rect::Make(50.0f, 50.0f, 80.0f, 80.0f)), "unrestricted intersects all")) return false;

    r.SetDirtyRects({
        Rect::Make(1000.0f, 30.0f, 1270.0f, 260.0f),
        Rect::Make(10.0f, 790.0f, 360.0f, 818.0f),
    });
    if (!Check(r.HasDirtyRect() && r.GetDirtyRects().size() == 2, "SetDirtyRects stores 2 rects")) return false;
    if (!Check(!r.IntersectsDirtyRect(Rect::Make(60.0f, 100.0f, 898.0f, 658.0f)), "renderer culls center gap")) return false;
    if (!Check(r.IntersectsDirtyRect(Rect::Make(1010.0f, 40.0f, 1100.0f, 80.0f)), "renderer accepts corner 1")) return false;
    if (!Check(r.IntersectsDirtyRect(Rect::Make(20.0f, 795.0f, 100.0f, 815.0f)), "renderer accepts corner 2")) return false;

    r.ClearDirtyRect();
    if (!Check(!r.HasDirtyRect() && r.GetDirtyRects().empty(), "ClearDirtyRect resets rects")) return false;

    std::printf("[PASS] TestRendererDirtyRectsInterface\n");
    return true;
}

#if defined(AI_WIN_UI_HAS_VCPKG_SKIA) || defined(AI_WIN_UI_HAS_LOCAL_SKIA)
float ReferenceMeasureWidthUtf8(SkFontMgr* fontMgr, SkTypeface* defaultTypeface, float fontSize, const std::wstring& text) {
    if (text.empty()) return 0.0f;
    const uint32_t len = static_cast<uint32_t>(text.size());
    if (skia_font::TypefaceSupportsText(defaultTypeface, text.c_str(), len)) {
        SkFont font = skia_font::CreateSkiaFont(fontSize, defaultTypeface);
        skia_font::StackUtf8Converter<> utf8(text.c_str(), len);
        return utf8.empty() ? 0.0f : font.measureText(utf8.c_str(), utf8.size(), SkTextEncoding::kUTF8);
    }
    float w = 0.0f;
    skia_font::IterateTextRuns(fontMgr, defaultTypeface, text.c_str(), len, [&](const wchar_t* rt, uint32_t rl, SkTypeface* tf) {
        SkFont font = skia_font::CreateSkiaFont(fontSize, tf);
        skia_font::StackUtf8Converter<> utf8(rt, rl);
        if (!utf8.empty()) {
            w += font.measureText(utf8.c_str(), utf8.size(), SkTextEncoding::kUTF8);
        }
    });
    return w;
}

bool TestSkiaTextUtf16ParityAndLruCache() {
    sk_sp<SkFontMgr> fontMgr = skia_font::CreateDefaultFontManager();
    sk_sp<SkTypeface> defaultFace = skia_font::CreateDefaultTypeface(fontMgr.get());
    if (!Check(fontMgr != nullptr && defaultFace != nullptr, "Skia FontMgr & default typeface initialized")) return false;

    const std::vector<std::wstring> samples = {
        L"AAPL 218.42 +1.25%",
        L"\u4E0A\u8BC1\u6307\u6570 \u9886\u5148: 3367.42  \u6700\u65B0: 3371.18 (+0.85%)",
        L"\u5E73\u5B89\u94F6\u884C 000001.SZ +3.45% \U0001F4C8"
    };
    const float sizes[] = {11.0f, 12.0f, 14.0f};
    for (float fs : sizes) {
        for (const auto& s : samples) {
            const float fastW = skia_font::MeasureTextWidthWithFallback(
                fontMgr.get(), defaultFace.get(), fs, s.c_str(), static_cast<uint32_t>(s.size()));
            const float refW = ReferenceMeasureWidthUtf8(fontMgr.get(), defaultFace.get(), fs, s);
            if (!Check(std::abs(fastW - refW) < 0.05f, "UTF-16 width matches UTF-8 reference within 0.05px")) {
                std::printf("  diff fast=%.4f ref=%.4f size=%.1f\n", fastW, refW, fs);
                return false;
            }
        }
    }

    // AcquireSkiaTextLayout pointer reuse + ellipsis constraint
    const std::wstring header = L"\u4E0A\u8BC1\u6307\u6570\u5B9E\u65F6\u884C\u60C5\u770B\u677F 1234.56";
    auto l1 = AcquireSkiaTextLayout(header.c_str(), static_cast<uint32_t>(header.size()), 12.0f, 400.0f, TextWrapMode::NoWrap);
    auto l2 = AcquireSkiaTextLayout(header.c_str(), static_cast<uint32_t>(header.size()), 12.0f, 400.0f, TextWrapMode::NoWrap);
    if (!Check(l1 != nullptr && l1.get() == l2.get(), "AcquireSkiaTextLayout zero-copy cache hit returns identical pointer")) return false;

    auto ell = AcquireSkiaTextLayout(header.c_str(), static_cast<uint32_t>(header.size()), 12.0f, 60.0f, TextWrapMode::NoWrap, false, false, true);
    if (!Check(ell != nullptr && ell->maxLineWidth <= 60.5f, "AcquireSkiaTextLayout ellipsis respects width budget")) return false;

    // LRU behavior check: keep a hot entry alive past 4096 capacity while evicting cold entries
    const std::wstring hotStr = L"HOT_LRU_TICK_99.99";
    const std::wstring coldStr = L"COLD_LRU_ENTRY_0";
    auto cold0 = AcquireSkiaTextLayout(coldStr.c_str(), static_cast<uint32_t>(coldStr.size()), 12.0f, 200.0f, TextWrapMode::NoWrap);
    auto hot0 = AcquireSkiaTextLayout(hotStr.c_str(), static_cast<uint32_t>(hotStr.size()), 12.0f, 200.0f, TextWrapMode::NoWrap);
    for (int i = 1; i <= 2500; ++i) {
        const std::wstring s = L"FILLER_A_" + std::to_wstring(i);
        AcquireSkiaTextLayout(s.c_str(), static_cast<uint32_t>(s.size()), 12.0f, 200.0f, TextWrapMode::NoWrap);
    }
    // Touch hot entry so it splices to front before second half overflows 4096
    auto hotTouch = AcquireSkiaTextLayout(hotStr.c_str(), static_cast<uint32_t>(hotStr.size()), 12.0f, 200.0f, TextWrapMode::NoWrap);
    if (!Check(hotTouch.get() == hot0.get(), "LRU hot entry still cached before overflow")) return false;

    for (int i = 2501; i <= 4200; ++i) {
        const std::wstring s = L"FILLER_B_" + std::to_wstring(i);
        AcquireSkiaTextLayout(s.c_str(), static_cast<uint32_t>(s.size()), 12.0f, 200.0f, TextWrapMode::NoWrap);
    }
    auto hotAfter = AcquireSkiaTextLayout(hotStr.c_str(), static_cast<uint32_t>(hotStr.size()), 12.0f, 200.0f, TextWrapMode::NoWrap);
    auto coldAfter = AcquireSkiaTextLayout(coldStr.c_str(), static_cast<uint32_t>(coldStr.size()), 12.0f, 200.0f, TextWrapMode::NoWrap);
    if (!Check(hotAfter.get() == hot0.get(), "LRU keeps promoted hot entry after >4096 insertions")) return false;
    if (!Check(coldAfter.get() != cold0.get(), "LRU evicts oldest unreferenced cold entry")) return false;

    std::printf("[PASS] TestSkiaTextUtf16ParityAndLruCache\n");
    return true;
}
#endif

} // namespace

int main() {
    if (!TestDirtyRegionDisjointCornersAndCulling()) return 1;
    if (!TestDirtyRegionGridCoalescingAndCap()) return 1;
    if (!TestRendererDirtyRectsInterface()) return 1;
#if defined(AI_WIN_UI_HAS_VCPKG_SKIA) || defined(AI_WIN_UI_HAS_LOCAL_SKIA)
    if (!TestSkiaTextUtf16ParityAndLruCache()) return 1;
#endif
    std::printf("ALL SKIA UNIT TESTS PASSED\n");
    return 0;
}

