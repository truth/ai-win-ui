#pragma once

#include <Windows.h>

#include <vector>

enum class WindowChromeMode {
    System,
    Custom,
    Layered, // custom chrome + WS_EX_LAYERED / per-pixel alpha present
};

struct WindowChromeHitRegion {
    RECT rect{};
    bool caption = false;
    bool clientOnly = false;
};

// Win32 frame / non-client helper for optional custom title-bar chrome.
// System mode defers all NC behavior to DefWindowProc.
class WindowChrome {
public:
    void SetMode(WindowChromeMode mode);
    WindowChromeMode Mode() const { return m_mode; }
    bool IsCustom() const {
        return m_mode == WindowChromeMode::Custom || m_mode == WindowChromeMode::Layered;
    }
    bool IsLayered() const { return m_mode == WindowChromeMode::Layered; }

    void SetDpi(UINT dpi);
    UINT Dpi() const { return m_dpi; }

    void SetFallbackCaptionHeightDip(int heightDip);
    void SetHitRegions(std::vector<WindowChromeHitRegion> regions);
    void ClearHitRegions();

    // Optional visual content bounds (client coords). Layered windows use this
    // for resize-border hit-testing so grips follow the floating card, not the
    // transparent outer HWND padding.
    void SetContentBounds(const RECT& bounds, bool enabled);
    void ClearContentBounds();

    // True if pt lands on caption or clientOnly chrome regions (must stay clickable
    // even when painted with transparent fills, e.g. caption buttons).
    bool IsChromeInteractiveHit(POINT clientPt) const;

    DWORD WindowStyle() const;
    DWORD WindowExStyle() const;

    // Apply style bits for the current mode (safe after CreateWindow).
    void ApplyWindowStyle(HWND hwnd) const;

    bool InitializeDwm(HWND hwnd);

    LRESULT HandleNcCalcSize(HWND hwnd, WPARAM wParam, LPARAM lParam, bool& handled) const;
    LRESULT HandleNcHitTest(HWND hwnd, LPARAM lParam, bool& handled) const;
    LRESULT HandleGetMinMaxInfo(HWND hwnd, LPARAM lParam, bool& handled) const;

    int ResizeBorderPx() const;
    int FallbackCaptionHeightPx() const;

    static WindowChromeMode ParseMode(const wchar_t* value);

private:
    static bool PointInRect(const RECT& rc, POINT pt);
    LRESULT HitTestResizeBorder(const RECT& bounds, POINT pt, bool maximized) const;

    WindowChromeMode m_mode = WindowChromeMode::System;
    UINT m_dpi = 96;
    int m_fallbackCaptionHeightDip = 40;
    std::vector<WindowChromeHitRegion> m_hitRegions;
    RECT m_contentBounds{};
    bool m_contentBoundsEnabled = false;
};
