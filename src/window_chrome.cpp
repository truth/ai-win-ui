#include "window_chrome.h"

#include <Windowsx.h>
#include <dwmapi.h>

#include <algorithm>
#include <cstring>

// std::max/min vs Windows min/max macros

#ifndef DWMWA_WINDOW_CORNER_PREFERENCE
#define DWMWA_WINDOW_CORNER_PREFERENCE 33
#endif
#ifndef DWMWCP_ROUND
#define DWMWCP_ROUND 2
#endif
#ifndef DWMWA_BORDER_COLOR
#define DWMWA_BORDER_COLOR 34
#endif
#ifndef DWMWA_COLOR_NONE
#define DWMWA_COLOR_NONE 0xFFFFFFFEu
#endif

namespace {

int ScaleDip(int dip, UINT dpi) {
    return MulDiv(std::max(0, dip), static_cast<int>(dpi > 0 ? dpi : 96), 96);
}

} // namespace

void WindowChrome::SetMode(WindowChromeMode mode) {
    m_mode = mode;
}

void WindowChrome::SetDpi(UINT dpi) {
    m_dpi = dpi > 0 ? dpi : 96;
}

void WindowChrome::SetFallbackCaptionHeightDip(int heightDip) {
    m_fallbackCaptionHeightDip = std::max(0, heightDip);
}

void WindowChrome::SetHitRegions(std::vector<WindowChromeHitRegion> regions) {
    m_hitRegions = std::move(regions);
}

void WindowChrome::ClearHitRegions() {
    m_hitRegions.clear();
}

void WindowChrome::SetContentBounds(const RECT& bounds, bool enabled) {
    m_contentBounds = bounds;
    m_contentBoundsEnabled = enabled;
}

void WindowChrome::ClearContentBounds() {
    m_contentBounds = {};
    m_contentBoundsEnabled = false;
}

bool WindowChrome::IsChromeInteractiveHit(POINT clientPt) const {
    for (const auto& region : m_hitRegions) {
        if ((region.clientOnly || region.caption) && PointInRect(region.rect, clientPt)) {
            return true;
        }
    }
    return false;
}

DWORD WindowChrome::WindowStyle() const {
    if (IsCustom()) {
        // Layered and custom share borderless popup chrome; layered adds WS_EX_LAYERED.
        // WS_THICKFRAME is intentionally OMITTED: it causes Windows to draw a visible 1-4px
        // resize frame border around the window even when WM_NCCALCSIZE returns 0.
        // Since we implement resize hit-testing ourselves via WM_NCHITTEST (returning HTLEFT,
        // HTRIGHT, etc.), DefWindowProc still handles the resize drag without WS_THICKFRAME.
        // DWM drop shadow is preserved via DwmExtendFrameIntoClientArea with MARGINS{-1,-1,-1,-1}.
        return WS_POPUP | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU | WS_CLIPCHILDREN;
    }
    return WS_OVERLAPPEDWINDOW | WS_VSCROLL | WS_CLIPCHILDREN;
}

DWORD WindowChrome::WindowExStyle() const {
    if (IsLayered()) {
        return WS_EX_LAYERED;
    }
    return 0;
}

void WindowChrome::ApplyWindowStyle(HWND hwnd) const {
    if (!hwnd) {
        return;
    }
    SetWindowLongPtrW(hwnd, GWL_STYLE, static_cast<LONG_PTR>(WindowStyle()));
    SetWindowLongPtrW(hwnd, GWL_EXSTYLE, static_cast<LONG_PTR>(WindowExStyle()));
    SetWindowPos(
        hwnd,
        nullptr,
        0,
        0,
        0,
        0,
        SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}

bool WindowChrome::InitializeDwm(HWND hwnd) {
    // Layered windows own transparency via UpdateLayeredWindow; skip DWM frame extend.
    if (!hwnd || !IsCustom() || IsLayered()) {
        return false;
    }

    // Extend frame into the entire client area to remove all non-client borders while keeping the drop shadow.
    MARGINS margins{-1, -1, -1, -1};
    const HRESULT hrExtend = DwmExtendFrameIntoClientArea(hwnd, &margins);
    if (FAILED(hrExtend)) {
        return false;
    }

    // Win11 rounded corners when the attribute is available.
    const int preference = DWMWCP_ROUND;
    DwmSetWindowAttribute(hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &preference, sizeof(preference));

    // Remove the 1px DWM border line that Windows 11 draws around all windows.
    // DWMWA_COLOR_NONE (0xFFFFFFFE) suppresses the border entirely.
    const COLORREF noBorder = static_cast<COLORREF>(DWMWA_COLOR_NONE);
    DwmSetWindowAttribute(hwnd, DWMWA_BORDER_COLOR, &noBorder, sizeof(noBorder));

    return true;
}

int WindowChrome::ResizeBorderPx() const {
    const int frame = GetSystemMetricsForDpi(SM_CXFRAME, m_dpi);
    const int padded = GetSystemMetricsForDpi(SM_CXPADDEDBORDER, m_dpi);
    return std::max(6, frame + padded);
}

int WindowChrome::FallbackCaptionHeightPx() const {
    return ScaleDip(m_fallbackCaptionHeightDip, m_dpi);
}

bool WindowChrome::PointInRect(const RECT& rc, POINT pt) {
    return pt.x >= rc.left && pt.x < rc.right && pt.y >= rc.top && pt.y < rc.bottom;
}

LRESULT WindowChrome::HitTestResizeBorder(const RECT& bounds, POINT pt, bool maximized) const {
    if (maximized) {
        return HTCLIENT;
    }

    // Point must be on/near the content (or window) rectangle.
    const int border = ResizeBorderPx();
    const RECT hitBounds{
        bounds.left - border,
        bounds.top - border,
        bounds.right + border,
        bounds.bottom + border};
    if (!PointInRect(hitBounds, pt)) {
        return HTCLIENT;
    }

    const bool left = pt.x < bounds.left + border;
    const bool right = pt.x >= bounds.right - border;
    const bool top = pt.y < bounds.top + border;
    const bool bottom = pt.y >= bounds.bottom - border;

    if (top && left) {
        return HTTOPLEFT;
    }
    if (top && right) {
        return HTTOPRIGHT;
    }
    if (bottom && left) {
        return HTBOTTOMLEFT;
    }
    if (bottom && right) {
        return HTBOTTOMRIGHT;
    }
    if (left) {
        return HTLEFT;
    }
    if (right) {
        return HTRIGHT;
    }
    if (top) {
        return HTTOP;
    }
    if (bottom) {
        return HTBOTTOM;
    }
    return HTCLIENT;
}

LRESULT WindowChrome::HandleNcCalcSize(HWND hwnd, WPARAM wParam, LPARAM lParam, bool& handled) const {
    handled = false;
    if (!IsCustom()) {
        return 0;
    }

    if (wParam == TRUE) {
        auto* params = reinterpret_cast<NCCALCSIZE_PARAMS*>(lParam);
        if (params) {
            if (IsZoomed(hwnd)) {
                HMONITOR monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
                MONITORINFO info{};
                info.cbSize = sizeof(info);
                if (GetMonitorInfoW(monitor, &info)) {
                    params->rgrc[0] = info.rcWork;
                }
            }
            // Non-maximized: leave rgrc[0] as the proposed window rect so the
            // entire window becomes client area (no system caption/border strip).
        }
        handled = true;
        return 0;
    }

    handled = true;
    return 0;
}

LRESULT WindowChrome::HandleNcHitTest(HWND hwnd, LPARAM lParam, bool& handled) const {
    handled = false;
    if (!IsCustom()) {
        return HTCLIENT;
    }

    POINT screenPt{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
    POINT pt = screenPt;
    ScreenToClient(hwnd, &pt);

    RECT client{};
    GetClientRect(hwnd, &client);
    const bool maximized = IsZoomed(hwnd) != FALSE;

    // Prefer visual content bounds for resize grips (layered floating card).
    // Fall back to full client for non-layered / missing content.
    RECT resizeBounds = client;
    if (m_contentBoundsEnabled && !maximized) {
        resizeBounds = m_contentBounds;
        // Keep grips usable: clamp to client so they never leave the HWND.
        resizeBounds.left = std::max(client.left, resizeBounds.left);
        resizeBounds.top = std::max(client.top, resizeBounds.top);
        resizeBounds.right = std::min(client.right, resizeBounds.right);
        resizeBounds.bottom = std::min(client.bottom, resizeBounds.bottom);
    }

    const LRESULT borderHit = HitTestResizeBorder(resizeBounds, pt, maximized);
    if (borderHit != HTCLIENT) {
        handled = true;
        return borderHit;
    }

    // Interactive chrome regions (buttons / caption) take priority over alpha.
    for (const auto& region : m_hitRegions) {
        if (region.clientOnly && PointInRect(region.rect, pt)) {
            handled = true;
            return HTCLIENT;
        }
    }

    for (const auto& region : m_hitRegions) {
        if (region.caption && !region.clientOnly && PointInRect(region.rect, pt)) {
            handled = true;
            return HTCAPTION;
        }
    }

    // Fallback top strip when UI regions are not ready yet.
    // Prefer content-top for layered cards so the strip aligns with the title bar.
    const LONG captionTop = m_contentBoundsEnabled ? resizeBounds.top : client.top;
    if (!maximized && pt.y >= captionTop && pt.y < captionTop + FallbackCaptionHeightPx() &&
        pt.x >= resizeBounds.left && pt.x < resizeBounds.right) {
        handled = true;
        return HTCAPTION;
    }

    handled = true;
    return HTCLIENT;
}

LRESULT WindowChrome::HandleGetMinMaxInfo(HWND hwnd, LPARAM lParam, bool& handled) const {
    handled = false;
    if (!IsCustom()) {
        return 0;
    }

    auto* info = reinterpret_cast<MINMAXINFO*>(lParam);
    if (!info) {
        return 0;
    }

    const int minW = ScaleDip(320, m_dpi);
    const int minH = ScaleDip(240, m_dpi);
    info->ptMinTrackSize.x = minW;
    info->ptMinTrackSize.y = minH;

    // Keep maximized size on the monitor work area.
    HMONITOR monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi{};
    mi.cbSize = sizeof(mi);
    if (GetMonitorInfoW(monitor, &mi)) {
        info->ptMaxPosition.x = mi.rcWork.left - mi.rcMonitor.left;
        info->ptMaxPosition.y = mi.rcWork.top - mi.rcMonitor.top;
        info->ptMaxSize.x = mi.rcWork.right - mi.rcWork.left;
        info->ptMaxSize.y = mi.rcWork.bottom - mi.rcWork.top;
    }

    handled = true;
    return 0;
}

WindowChromeMode WindowChrome::ParseMode(const wchar_t* value) {
    if (!value || !*value) {
        return WindowChromeMode::System;
    }

    auto equalsIgnoreCase = [](const wchar_t* a, const wchar_t* b) {
        size_t i = 0;
        for (; a[i] && b[i]; ++i) {
            wchar_t ca = a[i];
            wchar_t cb = b[i];
            if (ca >= L'A' && ca <= L'Z') {
                ca = static_cast<wchar_t>(ca - L'A' + L'a');
            }
            if (cb >= L'A' && cb <= L'Z') {
                cb = static_cast<wchar_t>(cb - L'A' + L'a');
            }
            if (ca != cb) {
                return false;
            }
        }
        return a[i] == L'\0' && b[i] == L'\0';
    };

    if (equalsIgnoreCase(value, L"custom")) {
        return WindowChromeMode::Custom;
    }
    if (equalsIgnoreCase(value, L"layered") || equalsIgnoreCase(value, L"irregular")) {
        return WindowChromeMode::Layered;
    }
    return WindowChromeMode::System;
}
