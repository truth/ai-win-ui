#pragma once
// Component module - part of the ai-win-ui library.
// See ui.h for the full umbrella include.
#include "ui_media.h"

class Button : public UIElement {
public:
    // Default: filled action button. Caption*: window chrome controls with
    // geometric glyphs (min / max / close) and Fluent-like hover washes.
    enum class Variant {
        Default,
        CaptionMinimize,
        CaptionMaximize,
        CaptionClose,
    };

    explicit Button(std::wstring text) : m_text(std::move(text)) {
        m_style = DefaultStyle(nullptr);
    }

    void SetOnClick(std::function<void()> onClick) { m_onClick = std::move(onClick); }
    void SetText(std::wstring text) { m_text = std::move(text); }
    void SetVariant(Variant variant) {
        m_variant = variant;
        const Theme* theme = (m_context && m_context->theme) ? m_context->theme : nullptr;
        switch (variant) {
            case Variant::CaptionMinimize:
            case Variant::CaptionMaximize:
                m_style = CaptionStyle();
                m_styleFromLayout = true; // caption styles are fixed, not layout/theme
                cornerRadius = 0.0f;
                background = Color{0, 0, 0, 0};
                foreground = ColorFromHex(0xD7E2EE);
                break;
            case Variant::CaptionClose:
                m_style = CaptionCloseStyle();
                m_styleFromLayout = true;
                cornerRadius = 0.0f;
                background = Color{0, 0, 0, 0};
                foreground = ColorFromHex(0xD7E2EE);
                break;
            case Variant::Default:
            default:
                m_styleFromLayout = false;
                m_style = DefaultStyle(theme);
                break;
        }
    }
    Variant GetVariant() const { return m_variant; }

    // When true on CaptionMaximize, draw the dual-square restore glyph.
    void SetShowRestoreGlyph(bool show) { m_showRestoreGlyph = show; }
    bool ShowRestoreGlyph() const { return m_showRestoreGlyph; }

    float cornerRadius = 4.0f;
    float fontSize = 14.0f;
    Color background = ColorFromHex(0x2D2D30);
    Color foreground = ColorFromHex(0xFFFFFF);

    static ComponentStyle DefaultStyle(const Theme* theme = nullptr) {
        ComponentStyle s;
        BoxDecoration normalDeco;
        normalDeco.background = ComponentStyle::ThemeColor(theme, "btn-bg", ColorFromHex(0x2D2D30));
        normalDeco.border.width = Thickness{1.0f, 1.0f, 1.0f, 1.0f};
        normalDeco.border.color = ComponentStyle::ThemeColor(theme, "border", ColorFromHex(0x6A6A6A));
        normalDeco.radius = CornerRadius::Uniform(ComponentStyle::ThemeNumber(theme, Theme::NumberCategory::Radius, "sm", 4.0f));
        s.base.decoration = normalDeco;
        s.base.foreground = ComponentStyle::ThemeColor(theme, "fg-strong", ColorFromHex(0xFFFFFF));

        BoxDecoration hoverDeco = normalDeco;
        hoverDeco.background = ComponentStyle::ThemeColor(theme, "btn-bg-hover", ColorFromHex(0x3E3E42));
        s.overrides[static_cast<std::size_t>(StyleState::Hover)].decoration = hoverDeco;

        BoxDecoration pressedDeco = normalDeco;
        pressedDeco.background = ComponentStyle::ThemeColor(theme, "btn-bg-press", ColorFromHex(0x0E639C));
        s.overrides[static_cast<std::size_t>(StyleState::Pressed)].decoration = pressedDeco;

        BoxDecoration focusedDeco = normalDeco;
        focusedDeco.border.width = Thickness{2.0f, 2.0f, 2.0f, 2.0f};
        focusedDeco.border.color = ComponentStyle::ThemeColor(theme, "border-focus", ColorFromHex(0xFFFFFF));
        s.overrides[static_cast<std::size_t>(StyleState::Focused)].decoration = focusedDeco;

        BoxDecoration disabledDeco = normalDeco;
        disabledDeco.background = ComponentStyle::ThemeColor(theme, "surface-4", ColorFromHex(0x1F1F1F));
        disabledDeco.border.color = ComponentStyle::ThemeColor(theme, "border-subtle", ColorFromHex(0x3A3A3A));
        s.overrides[static_cast<std::size_t>(StyleState::Disabled)].decoration = disabledDeco;
        s.overrides[static_cast<std::size_t>(StyleState::Disabled)].opacity = 0.6f;

        return s;
    }

    void ApplyThemeDefaults() override {
        if (m_styleFromLayout || m_variant != Variant::Default) {
            return;
        }
        const Theme* theme = (m_context && m_context->theme) ? m_context->theme : nullptr;
        m_style = DefaultStyle(theme);
        if (m_style.base.decoration.has_value()) {
            background = m_style.base.decoration->background;
            if (!m_style.base.decoration->radius.IsZero()) {
                cornerRadius = m_style.base.decoration->radius.MaxRadius();
            }
        }
        if (m_style.base.foreground.has_value()) {
            foreground = *m_style.base.foreground;
        }
    }

    // Soft luminous wash - no border, flat against the title bar.
    static ComponentStyle CaptionStyle() {
        ComponentStyle s;
        BoxDecoration normal;
        normal.background = Color{0.0f, 0.0f, 0.0f, 0.0f};
        normal.border.width = Thickness{};
        normal.radius = CornerRadius::Uniform(0.0f);
        s.base.decoration = normal;
        s.base.foreground = ColorFromHex(0xD7E2EE);

        BoxDecoration hover = normal;
        hover.background = Color{1.0f, 1.0f, 1.0f, 0.08f};
        s.overrides[static_cast<std::size_t>(StyleState::Hover)].decoration = hover;
        s.overrides[static_cast<std::size_t>(StyleState::Hover)].foreground = ColorFromHex(0xF4F8FC);

        BoxDecoration pressed = normal;
        pressed.background = Color{1.0f, 1.0f, 1.0f, 0.14f};
        s.overrides[static_cast<std::size_t>(StyleState::Pressed)].decoration = pressed;
        s.overrides[static_cast<std::size_t>(StyleState::Pressed)].foreground = ColorFromHex(0xFFFFFF);

        BoxDecoration focused = normal;
        focused.background = Color{1.0f, 1.0f, 1.0f, 0.05f};
        s.overrides[static_cast<std::size_t>(StyleState::Focused)].decoration = focused;
        return s;
    }

    // Close: warm crimson hover (desktop OS convention), white glyph.
    static ComponentStyle CaptionCloseStyle() {
        ComponentStyle s = CaptionStyle();
        BoxDecoration hover;
        hover.background = ColorFromHex(0xE81123);
        hover.border.width = Thickness{};
        hover.radius = CornerRadius::Uniform(0.0f);
        s.overrides[static_cast<std::size_t>(StyleState::Hover)].decoration = hover;
        s.overrides[static_cast<std::size_t>(StyleState::Hover)].foreground = ColorFromHex(0xFFFFFF);

        BoxDecoration pressed;
        pressed.background = ColorFromHex(0xF1707A);
        pressed.border.width = Thickness{};
        pressed.radius = CornerRadius::Uniform(0.0f);
        s.overrides[static_cast<std::size_t>(StyleState::Pressed)].decoration = pressed;
        s.overrides[static_cast<std::size_t>(StyleState::Pressed)].foreground = ColorFromHex(0xFFFFFF);
        return s;
    }

    bool IsFocusable() const override { return true; }

    bool OnFocus() override {
        if (!m_hasFocus) {
            m_hasFocus = true;
            return true;
        }
        return false;
    }

    bool OnBlur() override {
        if (m_hasFocus) {
            m_hasFocus = false;
            return true;
        }
        return false;
    }

    bool OnKeyDown(WPARAM keyCode, LPARAM /*lParam*/) override {
        if (keyCode == VK_SPACE || keyCode == VK_RETURN) {
            if (!m_pressed) {
                m_pressed = true;
                return true;
            }
        }
        return false;
    }

    bool OnKeyUp(WPARAM keyCode, LPARAM /*lParam*/) override {
        if ((keyCode == VK_SPACE || keyCode == VK_RETURN) && m_pressed) {
            m_pressed = false;
            if (m_onClick) {
                m_onClick();
            }
            return true;
        }
        return false;
    }

protected:
    float MeasurePreferredWidth(float availableWidth) const override {
        const Size measured = MeasureTextValue(
            m_text,
            fontSize,
            availableWidth > 0.0f ? availableWidth : 4096.0f,
            TextWrapMode::NoWrap);
        return std::min(availableWidth, measured.width + ScaleValue(24.0f));
    }

    float MeasurePreferredHeight(float width) const override {
        const Size measured = MeasureTextValue(
            m_text,
            fontSize,
            width > 0.0f ? width : 4096.0f,
            TextWrapMode::NoWrap);
        return measured.height + ScaleValue(14.0f);
    }

public:
    void Render(IRenderer& renderer) override {
        SyncQuickSetToStyle();
        const StyleSpec resolved = m_style.Resolve(GetCurrentState());
        BoxDecoration deco = resolved.decoration.value_or(BoxDecoration{});
        if (IsCaptionVariant()) {
            // Caption controls stay sharp-edged and borderless.
            deco.radius = CornerRadius::Uniform(0.0f);
            deco.border.width = Thickness{};
        } else {
            deco.radius = CornerRadius::Uniform(ScaleValue(cornerRadius));
            deco.border.width = Thickness{
                ScaleValue(deco.border.width.left),
                ScaleValue(deco.border.width.top),
                ScaleValue(deco.border.width.right),
                ScaleValue(deco.border.width.bottom)
            };
        }
        if (resolved.opacity.has_value()) {
            deco.opacity = *resolved.opacity;
        }
        DrawBoxDecoration(renderer, m_bounds, deco);

        Color fg = resolved.foreground.value_or(foreground);
        if (IsCaptionVariant()) {
            if (m_context && !m_context->windowActive) {
                // Dim caption glyphs when the window is not active.
                fg.a *= 0.45f;
            }
            DrawCaptionGlyph(renderer, fg);
            return;
        }

        // Center label properly (no left padding bias).
        const TextRenderOptions textOptions{
            TextWrapMode::NoWrap,
            TextHorizontalAlign::Center,
            TextVerticalAlign::Center
        };
        const float fs = resolved.fontSize.value_or(fontSize);
        renderer.DrawTextW(
            m_text.c_str(),
            static_cast<UINT32>(m_text.size()),
            m_bounds,
            fg,
            ScaleValue(fs),
            textOptions
        );
    }

private:
    bool IsCaptionVariant() const {
        return m_variant == Variant::CaptionMinimize
            || m_variant == Variant::CaptionMaximize
            || m_variant == Variant::CaptionClose;
    }

    void DrawCaptionGlyph(IRenderer& renderer, const Color& color) const {
        const float cx = (m_bounds.left + m_bounds.right) * 0.5f;
        const float cy = (m_bounds.top + m_bounds.bottom) * 0.5f;
        // Icon optical size ~10dip; stroke slightly thicker than hairline for clarity.
        const float half = ScaleValue(5.0f);
        const float stroke = std::max(1.0f, ScaleValue(1.15f));

        switch (m_variant) {
            case Variant::CaptionMinimize: {
                renderer.DrawLine(
                    PointF{cx - half, cy},
                    PointF{cx + half, cy},
                    color,
                    stroke);
                break;
            }
            case Variant::CaptionMaximize: {
                if (m_showRestoreGlyph) {
                    // Overlapping squares (restore): back offset up-right, front lower-left.
                    const float offset = ScaleValue(2.2f);
                    const float s = half * 0.78f;
                    const Rect back = Rect::Make(
                        cx - s + offset, cy - s - offset,
                        cx + s + offset, cy + s - offset);
                    const Rect front = Rect::Make(
                        cx - s - offset, cy - s + offset,
                        cx + s - offset, cy + s + offset);
                    renderer.DrawRect(back, color, stroke);
                    // Clear the overlapping edge of the back square by filling front bg is
                    // unavailable; draw front on top for a clean dual-window mark.
                    renderer.DrawRect(front, color, stroke);
                } else {
                    const Rect box = Rect::Make(cx - half, cy - half, cx + half, cy + half);
                    renderer.DrawRect(box, color, stroke);
                }
                break;
            }
            case Variant::CaptionClose: {
                // Slightly larger X for balance against the square glyph.
                const float xHalf = ScaleValue(4.6f);
                renderer.DrawLine(
                    PointF{cx - xHalf, cy - xHalf},
                    PointF{cx + xHalf, cy + xHalf},
                    color,
                    stroke);
                renderer.DrawLine(
                    PointF{cx + xHalf, cy - xHalf},
                    PointF{cx - xHalf, cy + xHalf},
                    color,
                    stroke);
                break;
            }
            default:
                break;
        }
    }

    void SyncQuickSetToStyle() {
        // Caption variants own their state palette; don't clobber with quick-set colors.
        if (IsCaptionVariant()) {
            return;
        }
        if (!m_style.base.decoration.has_value()) {
            m_style.base.decoration = BoxDecoration{};
        }
        m_style.base.decoration->background = background;
        m_style.base.foreground = foreground;
        m_style.base.fontSize  = fontSize;
    }

public:

    bool OnMouseMove(float x, float y) override {
        const bool nextHovered = HitTest(x, y);
        const bool changed = m_hovered != nextHovered;
        m_hovered = nextHovered;
        return changed;
    }

    bool OnMouseDown(float x, float y) override {
        const bool nextPressed = HitTest(x, y);
        const bool changed = m_pressed != nextPressed || m_hovered != nextPressed;
        m_pressed = nextPressed;
        m_hovered = nextPressed;
        return changed;
    }

    bool OnMouseUp(float x, float y) override {
        const bool wasPressed = m_pressed;
        const bool nextHovered = HitTest(x, y);
        const bool changed = wasPressed || (m_hovered != nextHovered);
        m_pressed = false;
        m_hovered = nextHovered;
        if (wasPressed && nextHovered && m_onClick) {
            m_onClick();
            return true;
        }
        return changed;
    }

    bool OnMouseLeave() override {
        if (!m_hovered) {
            return false;
        }
        m_hovered = false;
        return true;
    }

private:
    std::wstring m_text;
    std::function<void()> m_onClick;
    Variant m_variant = Variant::Default;
    bool m_showRestoreGlyph = false;
};

class TextInput : public UIElement {
public:
    explicit TextInput(std::wstring text = L"") : m_text(std::move(text)), m_caretPosition(m_text.size()) {
        m_style = DefaultStyle(nullptr);
    }

    static ComponentStyle DefaultStyle(const Theme* theme = nullptr) {
        ComponentStyle s;
        BoxDecoration normalDeco;
        normalDeco.background = ComponentStyle::ThemeColor(theme, "input-bg", ColorFromHex(0x1F1F1F));
        normalDeco.border.width = Thickness{1.0f, 1.0f, 1.0f, 1.0f};
        normalDeco.border.color = ComponentStyle::ThemeColor(theme, "border", ColorFromHex(0x6A6A6A));
        normalDeco.radius = CornerRadius::Uniform(ComponentStyle::ThemeNumber(theme, Theme::NumberCategory::Radius, "sm", 4.0f));
        s.base.decoration = normalDeco;
        s.base.foreground = ComponentStyle::ThemeColor(theme, "fg", ColorFromHex(0xEDEDED));

        BoxDecoration focusedDeco = normalDeco;
        focusedDeco.border.width = Thickness{2.0f, 2.0f, 2.0f, 2.0f};
        focusedDeco.border.color = ComponentStyle::ThemeColor(theme, "border-focus", ColorFromHex(0xFFFFFF));
        s.overrides[static_cast<std::size_t>(StyleState::Focused)].decoration = focusedDeco;

        BoxDecoration disabledDeco = normalDeco;
        disabledDeco.background = ComponentStyle::ThemeColor(theme, "surface-0", ColorFromHex(0x171717));
        disabledDeco.border.color = ComponentStyle::ThemeColor(theme, "border-subtle", ColorFromHex(0x3A3A3A));
        s.overrides[static_cast<std::size_t>(StyleState::Disabled)].decoration = disabledDeco;
        s.overrides[static_cast<std::size_t>(StyleState::Disabled)].opacity = 0.6f;

        return s;
    }

    void ApplyThemeDefaults() override {
        if (m_styleFromLayout) {
            return;
        }
        const Theme* theme = (m_context && m_context->theme) ? m_context->theme : nullptr;
        m_style = DefaultStyle(theme);
    }

    bool IsFocusable() const override { return true; }

    bool OnFocus() override {
        if (!m_hasFocus) {
            m_hasFocus = true;
            return true;
        }
        return false;
    }

    bool OnBlur() override {
        if (m_hasFocus) {
            m_hasFocus = false;
            return true;
        }
        return false;
    }

    bool OnKeyDown(WPARAM keyCode, LPARAM /*lParam*/) override {
        const bool shiftDown = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
        const bool ctrlDown = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
        switch (keyCode) {
            case VK_BACK:
                if (HasSelection()) {
                    DeleteSelection();
                    ResetCaretBlink();
                    return true;
                }
                if (m_caretPosition > 0) {
                    m_text.erase(m_caretPosition - 1, 1);
                    --m_caretPosition;
                    ClearSelection();
                    ResetCaretBlink();
                    return true;
                }
                break;
            case VK_DELETE:
                if (HasSelection()) {
                    DeleteSelection();
                    ResetCaretBlink();
                    return true;
                }
                if (m_caretPosition < m_text.size()) {
                    m_text.erase(m_caretPosition, 1);
                    ResetCaretBlink();
                    return true;
                }
                break;
            case VK_LEFT:
                if (m_caretPosition > 0) {
                    if (shiftDown && !HasSelection()) {
                        m_selectionStart = m_caretPosition;
                    }
                    --m_caretPosition;
                    if (shiftDown) {
                        m_selectionEnd = m_caretPosition;
                    } else {
                        ClearSelection();
                    }
                    ResetCaretBlink();
                    return true;
                }
                break;
            case VK_RIGHT:
                if (m_caretPosition < m_text.size()) {
                    if (shiftDown && !HasSelection()) {
                        m_selectionStart = m_caretPosition;
                    }
                    ++m_caretPosition;
                    if (shiftDown) {
                        m_selectionEnd = m_caretPosition;
                    } else {
                        ClearSelection();
                    }
                    ResetCaretBlink();
                    return true;
                }
                break;
            case VK_HOME:
                m_caretPosition = 0;
                if (shiftDown) {
                    m_selectionEnd = m_caretPosition;
                } else {
                    ClearSelection();
                }
                ResetCaretBlink();
                return true;
            case VK_END:
                m_caretPosition = m_text.size();
                if (shiftDown) {
                    m_selectionEnd = m_caretPosition;
                } else {
                    ClearSelection();
                }
                ResetCaretBlink();
                return true;
            case 'A':
                if (ctrlDown) {
                    m_selectionStart = 0;
                    m_selectionEnd = m_text.size();
                    m_caretPosition = m_selectionEnd;
                    ResetCaretBlink();
                    return true;
                }
                break;
            case 'C':
                if (ctrlDown && HasSelection()) {
                    const auto [start, end] = GetSelectionRange();
                    const std::wstring selectedText = m_text.substr(start, end - start);
                    if (OpenClipboard(nullptr)) {
                        EmptyClipboard();
                        const size_t bytes = (selectedText.size() + 1) * sizeof(wchar_t);
                        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, bytes);
                        if (hMem) {
                            void* dest = GlobalLock(hMem);
                            if (dest) {
                                memcpy(dest, selectedText.c_str(), bytes);
                                GlobalUnlock(hMem);
                                SetClipboardData(CF_UNICODETEXT, hMem);
                            } else {
                                GlobalFree(hMem);
                            }
                        }
                        CloseClipboard();
                    }
                    return true;
                }
                break;
            case 'V':
                if (ctrlDown) {
                    if (OpenClipboard(nullptr)) {
                        HANDLE hData = GetClipboardData(CF_UNICODETEXT);
                        if (hData) {
                            wchar_t* clipboardText = static_cast<wchar_t*>(GlobalLock(hData));
                            if (clipboardText) {
                                if (HasSelection()) {
                                    DeleteSelection();
                                }
                                m_text.insert(m_caretPosition, clipboardText);
                                m_caretPosition += wcslen(clipboardText);
                                GlobalUnlock(hData);
                                ClearSelection();
                                ResetCaretBlink();
                                CloseClipboard();
                                return true;
                            }
                        }
                        CloseClipboard();
                    }
                }
                break;
            case 'X':
                if (ctrlDown && HasSelection()) {
                    const auto [start, end] = GetSelectionRange();
                    const std::wstring selectedText = m_text.substr(start, end - start);
                    if (OpenClipboard(nullptr)) {
                        EmptyClipboard();
                        const size_t bytes = (selectedText.size() + 1) * sizeof(wchar_t);
                        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, bytes);
                        if (hMem) {
                            void* dest = GlobalLock(hMem);
                            if (dest) {
                                memcpy(dest, selectedText.c_str(), bytes);
                                GlobalUnlock(hMem);
                                SetClipboardData(CF_UNICODETEXT, hMem);
                            } else {
                                GlobalFree(hMem);
                            }
                        }
                        CloseClipboard();
                    }
                    DeleteSelection();
                    ResetCaretBlink();
                    return true;
                }
                break;
        }
        return false;
    }

    bool OnChar(wchar_t ch) override {
        if (ch == L'\b') {
            return true;
        }
        if (ch >= L' ' && ch != 0x7F) {
            if (HasSelection()) {
                DeleteSelection();
            }
            m_text.insert(m_caretPosition, 1, ch);
            ++m_caretPosition;
            ResetCaretBlink();
            return true;
        }
        return false;
    }

    bool OnMouseDown(float x, float y) override {
        const float horizontalInset = ScaleValue(8.0f);
        const float verticalInset = ScaleValue(6.0f);
        const Rect textRect = Rect::Make(m_bounds.left + horizontalInset, m_bounds.top + verticalInset, m_bounds.right - horizontalInset, m_bounds.bottom - verticalInset);
        if (!HitTest(x, y)) {
            return false;
        }

        const float localX = x - textRect.left;
        const size_t newCaret = ComputeCaretIndex(localX, textRect);
        const bool shiftDown = (GetKeyState(VK_SHIFT) & 0x8000) != 0;

        if (shiftDown) {
            if (!HasSelection()) {
                m_selectionStart = m_caretPosition;
            }
            m_caretPosition = newCaret;
            m_selectionEnd = m_caretPosition;
        } else {
            m_caretPosition = newCaret;
            ClearSelection();
        }

        m_dragAnchor = m_caretPosition;
        m_isDragging = true;
        ResetCaretBlink();
        return true;
    }

    bool OnMouseMove(float x, float y) override {
        if (!m_isDragging) {
            return false;
        }

        const float horizontalInset = ScaleValue(8.0f);
        const float verticalInset = ScaleValue(6.0f);
        const Rect textRect = Rect::Make(m_bounds.left + horizontalInset, m_bounds.top + verticalInset, m_bounds.right - horizontalInset, m_bounds.bottom - verticalInset);
        const float localX = x - textRect.left;
        const size_t newCaret = ComputeCaretIndex(localX, textRect);
        if (newCaret == m_caretPosition) {
            return false;
        }

        m_caretPosition = newCaret;
        if (!HasSelection()) {
            m_selectionStart = m_dragAnchor;
        }
        m_selectionEnd = m_caretPosition;
        ResetCaretBlink();
        return true;
    }

    bool OnMouseUp(float x, float y) override {
        if (!m_isDragging) {
            return false;
        }
        m_isDragging = false;
        return true;
    }

    bool OnMouseLeave() override {
        if (!m_isDragging) {
            return false;
        }
        m_isDragging = false;
        return true;
    }

    // Place caret at the glyph boundary nearest to localX (same NoWrap metrics as draw).
    size_t ComputeCaretIndex(float localX, const Rect& textRect) const {
        if (m_text.empty()) {
            return 0;
        }
        const float maxW = textRect.Width();
        size_t best = 0;
        float bestDist = std::abs(localX - 0.0f);
        for (size_t i = 1; i <= m_text.size(); ++i) {
            const float prefixWidth = MeasureNoWrapTextWidth(m_text.substr(0, i), maxW);
            const float dist = std::abs(localX - prefixWidth);
            if (dist < bestDist) {
                bestDist = dist;
                best = i;
            }
        }
        return best;
    }

    bool OnTimer(UINT_PTR /*timerId*/) override {
        if (!m_hasFocus) {
            return false;
        }
        const DWORD now = GetTickCount();
        if (now - m_lastBlink >= 500) {
            m_showCaret = !m_showCaret;
            m_lastBlink = now;
            return true;
        }
        return false;
    }

    void SetFontSize(float size) { fontSize = size; }
    void SetBackgroundColor(const Color& color) { background = color; }
    void SetTextColor(const Color& color) { textColor = color; }
    void SetCornerRadius(float radius) { cornerRadius = radius; }

    bool GetTextInputCaretRect(Rect& out) const override {
        if (!m_hasFocus) {
            return false;
        }
        out = CaretRect();
        return true;
    }

protected:
    float MeasurePreferredWidth(float availableWidth) const override {
        const std::wstring measureText = m_text.empty() ? L" " : m_text;
        const float inset = ScaleValue(16.0f);
        const Size measured = MeasureTextValue(
            measureText,
            fontSize,
            std::max(1.0f, availableWidth - inset),
            TextWrapMode::NoWrap);
        return std::min(availableWidth, measured.width + inset);
    }

    float MeasurePreferredHeight(float width) const override {
        const std::wstring measureText = m_text.empty() ? L" " : m_text;
        const float inset = ScaleValue(16.0f);
        const Size measured = MeasureTextValue(
            measureText,
            fontSize,
            std::max(1.0f, width - inset),
            TextWrapMode::NoWrap);
        return measured.height + ScaleValue(12.0f);
    }

public:
    void Render(IRenderer& renderer) override {
        SyncQuickSetToStyle();
        const StyleSpec resolved = m_style.Resolve(GetCurrentState());
        BoxDecoration deco = resolved.decoration.value_or(BoxDecoration{});
        deco.radius = CornerRadius::Uniform(ScaleValue(cornerRadius));
        deco.border.width = Thickness{
            ScaleValue(deco.border.width.left),
            ScaleValue(deco.border.width.top),
            ScaleValue(deco.border.width.right),
            ScaleValue(deco.border.width.bottom)
        };
        if (resolved.opacity.has_value()) {
            deco.opacity = *resolved.opacity;
        }
        DrawBoxDecoration(renderer, m_bounds, deco);

        const Rect textRect = ContentRect();
        const Rect lineRect = TextLineRect(textRect);
        if (HasSelection()) {
            const auto [selStart, selEnd] = GetSelectionRange();
            std::wstring prefix = m_text.substr(0, selStart);
            std::wstring selection = m_text.substr(selStart, selEnd - selStart);
            const float prefixWidth = MeasureNoWrapTextWidth(prefix, textRect.Width());
            const Size selectionMetrics = MeasureTextValue(
                selection.empty() ? L" " : selection,
                fontSize,
                std::max(1.0f, textRect.Width()),
                TextWrapMode::NoWrap);
            Rect selectionRect = Rect::Make(
                lineRect.left + prefixWidth,
                lineRect.top,
                lineRect.left + prefixWidth + selectionMetrics.width,
                lineRect.bottom
            );
            renderer.FillRect(selectionRect, ColorFromHex(0x3A86FF));
        }
        const TextRenderOptions textOptions{
            TextWrapMode::NoWrap,
            TextHorizontalAlign::Start,
            TextVerticalAlign::Center
        };
        const Color fg = resolved.foreground.value_or(textColor);
        renderer.DrawTextW(
            m_text.c_str(),
            static_cast<UINT32>(m_text.size()),
            lineRect,
            fg,
            ScaleValue(fontSize),
            textOptions);

        if (m_hasFocus) {
            const Rect caretRect = CaretRect();
            if (m_showCaret) {
                renderer.DrawRect(caretRect, fg, 1.0f);
            }
        }
    }

private:
    void SyncQuickSetToStyle() {
        if (!m_style.base.decoration.has_value()) {
            m_style.base.decoration = BoxDecoration{};
        }
        m_style.base.decoration->background = background;
        m_style.base.foreground = textColor;
        m_style.base.fontSize  = fontSize;
    }

public:

private:
    std::wstring m_text;
    size_t m_caretPosition = 0;
    size_t m_selectionStart = 0;
    size_t m_selectionEnd = 0;
    bool m_showCaret = true;
    DWORD m_lastBlink = 0;
    float fontSize = 14.0f;
    Color background = ColorFromHex(0x1F1F1F);
    Color borderColor = ColorFromHex(0x6A6A6A);
    Color focusBorderColor = ColorFromHex(0xFFFFFF);
    Color textColor = ColorFromHex(0xEDEDED);
    float cornerRadius = 4.0f;

    bool HasSelection() const {
        return m_selectionStart != m_selectionEnd;
    }

    std::pair<size_t, size_t> GetSelectionRange() const {
        return m_selectionStart <= m_selectionEnd
            ? std::make_pair(m_selectionStart, m_selectionEnd)
            : std::make_pair(m_selectionEnd, m_selectionStart);
    }

    void ClearSelection() {
        m_selectionStart = m_selectionEnd = m_caretPosition;
    }

    void DeleteSelection() {
        if (!HasSelection()) {
            return;
        }
        const auto [start, end] = GetSelectionRange();
        m_text.erase(start, end - start);
        m_caretPosition = start;
        ClearSelection();
    }

    void ResetCaretBlink() {
        m_showCaret = true;
        m_lastBlink = GetTickCount();
    }

    Rect ContentRect() const {
        const float horizontalInset = ScaleValue(8.0f);
        const float verticalInset = ScaleValue(6.0f);
        return Rect::Make(
            m_bounds.left + horizontalInset,
            m_bounds.top + verticalInset,
            std::max(m_bounds.left + horizontalInset, m_bounds.right - horizontalInset),
            std::max(m_bounds.top + verticalInset, m_bounds.bottom - verticalInset));
    }

    Rect TextLineRect(const Rect& contentRect) const {
        const Size metrics = MeasureTextValue(
            m_text.empty() ? L" " : m_text,
            fontSize,
            std::max(1.0f, contentRect.Width()),
            TextWrapMode::NoWrap);
        const float lineHeight = std::min(contentRect.Height(), std::max(1.0f, metrics.height));
        const float top = contentRect.top + std::max(0.0f, (contentRect.Height() - lineHeight) * 0.5f);
        return Rect::Make(contentRect.left, top, contentRect.right, top + lineHeight);
    }

    float MeasureNoWrapTextWidth(const std::wstring& text, float maxWidth) const {
        if (text.empty()) {
            return 0.0f;
        }
        return MeasureTextValue(
            text,
            fontSize,
            std::max(1.0f, maxWidth),
            TextWrapMode::NoWrap).width;
    }

    Rect CaretRect() const {
        const Rect textRect = ContentRect();
        const Rect lineRect = TextLineRect(textRect);
        const std::wstring prefix = m_text.substr(0, m_caretPosition);
        const float caretX = lineRect.left + MeasureNoWrapTextWidth(prefix, textRect.Width());
        return Rect::Make(caretX, lineRect.top, caretX + ScaleValue(1.0f), lineRect.bottom);
    }

    bool m_isDragging = false;
    size_t m_dragAnchor = 0;
};

class Checkbox : public UIElement {
public:
    explicit Checkbox(std::wstring text = L"") : m_text(std::move(text)) {
        m_style = DefaultStyle(nullptr);
    }

    static ComponentStyle DefaultStyle(const Theme* theme = nullptr) {
        ComponentStyle s;
        BoxDecoration normalDeco;
        normalDeco.background = ComponentStyle::ThemeColor(theme, "btn-bg", ColorFromHex(0x2D2D30));
        normalDeco.border.width = Thickness{1.0f, 1.0f, 1.0f, 1.0f};
        normalDeco.border.color = ComponentStyle::ThemeColor(theme, "border", ColorFromHex(0x6A6A6A));
        s.base.decoration = normalDeco;
        s.base.foreground = ComponentStyle::ThemeColor(theme, "fg", ColorFromHex(0xEDEDED));

        BoxDecoration focusedDeco = normalDeco;
        focusedDeco.border.width = Thickness{2.0f, 2.0f, 2.0f, 2.0f};
        focusedDeco.border.color = ComponentStyle::ThemeColor(theme, "border-focus", ColorFromHex(0xFFFFFF));
        s.overrides[static_cast<std::size_t>(StyleState::Focused)].decoration = focusedDeco;

        BoxDecoration disabledDeco = normalDeco;
        disabledDeco.background = ComponentStyle::ThemeColor(theme, "surface-4", ColorFromHex(0x1F1F1F));
        disabledDeco.border.color = ComponentStyle::ThemeColor(theme, "border-subtle", ColorFromHex(0x3A3A3A));
        s.overrides[static_cast<std::size_t>(StyleState::Disabled)].decoration = disabledDeco;
        s.overrides[static_cast<std::size_t>(StyleState::Disabled)].opacity = 0.6f;
        return s;
    }

    void ApplyThemeDefaults() override {
        if (m_styleFromLayout) {
            return;
        }
        const Theme* theme = (m_context && m_context->theme) ? m_context->theme : nullptr;
        m_style = DefaultStyle(theme);
        if (m_style.base.foreground.has_value()) {
            textColor = *m_style.base.foreground;
        }
    }

    bool IsFocusable() const override { return true; }

    bool OnFocus() override {
        if (!m_hasFocus) {
            m_hasFocus = true;
            return true;
        }
        return false;
    }

    bool OnBlur() override {
        if (m_hasFocus) {
            m_hasFocus = false;
            return true;
        }
        return false;
    }

    bool OnKeyDown(WPARAM keyCode, LPARAM /*lParam*/) override {
        if (keyCode == VK_SPACE || keyCode == VK_RETURN) {
            m_checked = !m_checked;
            return true;
        }
        return false;
    }

    float MeasurePreferredWidth(float availableWidth) const override {
        const float reserve = ScaleValue(30.0f);
        const Size textSize = MeasureTextValue(
            m_text.empty() ? L" " : m_text,
            fontSize,
            std::max(1.0f, availableWidth - reserve),
            TextWrapMode::NoWrap);
        return std::min(availableWidth, textSize.width + reserve);
    }

    float MeasurePreferredHeight(float width) const override {
        const float reserve = ScaleValue(30.0f);
        const Size textSize = MeasureTextValue(
            m_text.empty() ? L" " : m_text,
            fontSize,
            std::max(1.0f, width - reserve),
            TextWrapMode::NoWrap);
        return textSize.height + ScaleValue(10.0f);
    }

    void Render(IRenderer& renderer) override {
        SyncQuickSetToStyle();
        const StyleSpec resolved = m_style.Resolve(GetCurrentState());
        BoxDecoration deco = resolved.decoration.value_or(BoxDecoration{});
        deco.border.width = Thickness{
            ScaleValue(deco.border.width.left),
            ScaleValue(deco.border.width.top),
            ScaleValue(deco.border.width.right),
            ScaleValue(deco.border.width.bottom)
        };
        if (resolved.opacity.has_value()) {
            deco.opacity = *resolved.opacity;
        }

        const Rect box = Rect::Make(
            m_bounds.left,
            m_bounds.top + ScaleValue(2.0f),
            m_bounds.left + ScaleValue(18.0f),
            m_bounds.top + ScaleValue(20.0f));
        DrawBoxDecoration(renderer, box, deco);
        if (m_checked) {
            const float inset = ScaleValue(4.0f);
            const Rect mark = Rect::Make(box.left + inset, box.top + inset, box.right - inset, box.bottom - inset);
            renderer.FillRect(mark, ColorFromHex(0x2D6CDF));
        }

        Rect textRect = m_bounds;
        textRect.left += ScaleValue(26.0f);
        const TextRenderOptions textOptions{
            TextWrapMode::NoWrap,
            TextHorizontalAlign::Start,
            TextVerticalAlign::Center
        };
        const Color fg = resolved.foreground.value_or(textColor);
        renderer.DrawTextW(
            m_text.c_str(),
            static_cast<UINT32>(m_text.size()),
            textRect,
            fg,
            ScaleValue(fontSize),
            textOptions);
    }

private:
    void SyncQuickSetToStyle() {
        if (!m_style.base.decoration.has_value()) {
            m_style.base.decoration = BoxDecoration{};
        }
        m_style.base.foreground = textColor;
    }

public:

    bool OnMouseDown(float x, float y) override {
        if (!HitTest(x, y)) {
            return false;
        }
        m_pressed = true;
        return true;
    }

    bool OnMouseUp(float x, float y) override {
        const bool hit = HitTest(x, y);
        if (m_pressed && hit) {
            m_checked = !m_checked;
        }
        m_pressed = false;
        return hit;
    }

    bool OnMouseLeave() override {
        if (!m_pressed) {
            return false;
        }
        m_pressed = false;
        return true;
    }

    void SetText(std::wstring text) { m_text = std::move(text); }
    void SetChecked(bool checked) { m_checked = checked; }
    bool IsChecked() const { return m_checked; }

private:
    std::wstring m_text;
    bool m_checked = false;
    Color textColor = ColorFromHex(0xEDEDED);
    float fontSize = 14.0f;
};

class RadioButton : public UIElement {
public:
    explicit RadioButton(std::wstring text = L"", std::wstring group = L"default")
        : m_text(std::move(text)), m_group(std::move(group)) {
        s_groups[m_group].push_back(this);
        m_style = DefaultStyle(nullptr);
    }

    static ComponentStyle DefaultStyle(const Theme* theme = nullptr) {
        ComponentStyle s;
        BoxDecoration normalDeco;
        normalDeco.background = ColorFromHex(0x000000, 0.0f);  // transparent
        normalDeco.border.width = Thickness{1.0f, 1.0f, 1.0f, 1.0f};
        normalDeco.border.color = ComponentStyle::ThemeColor(theme, "border", ColorFromHex(0x6A6A6A));
        normalDeco.radius = CornerRadius::Uniform(9.0f);
        s.base.decoration = normalDeco;
        s.base.foreground = ComponentStyle::ThemeColor(theme, "fg", ColorFromHex(0xEDEDED));

        BoxDecoration focusedDeco = normalDeco;
        focusedDeco.border.width = Thickness{2.0f, 2.0f, 2.0f, 2.0f};
        focusedDeco.border.color = ComponentStyle::ThemeColor(theme, "border-focus", ColorFromHex(0xFFFFFF));
        s.overrides[static_cast<std::size_t>(StyleState::Focused)].decoration = focusedDeco;

        BoxDecoration disabledDeco = normalDeco;
        disabledDeco.border.color = ComponentStyle::ThemeColor(theme, "border-subtle", ColorFromHex(0x3A3A3A));
        s.overrides[static_cast<std::size_t>(StyleState::Disabled)].decoration = disabledDeco;
        s.overrides[static_cast<std::size_t>(StyleState::Disabled)].opacity = 0.6f;
        return s;
    }

    void ApplyThemeDefaults() override {
        if (m_styleFromLayout) {
            return;
        }
        const Theme* theme = (m_context && m_context->theme) ? m_context->theme : nullptr;
        m_style = DefaultStyle(theme);
        if (m_style.base.foreground.has_value()) {
            textColor = *m_style.base.foreground;
        }
    }

    ~RadioButton() {
        auto& group = s_groups[m_group];
        group.erase(std::remove(group.begin(), group.end(), this), group.end());
    }

    bool IsFocusable() const override { return true; }

    bool OnFocus() override {
        if (!m_hasFocus) {
            m_hasFocus = true;
            return true;
        }
        return false;
    }

    bool OnBlur() override {
        if (m_hasFocus) {
            m_hasFocus = false;
            return true;
        }
        return false;
    }

    bool OnKeyDown(WPARAM keyCode, LPARAM /*lParam*/) override {
        if (keyCode == VK_SPACE || keyCode == VK_RETURN) {
            SetChecked(true);
            return true;
        }
        return false;
    }

    float MeasurePreferredWidth(float availableWidth) const override {
        const float reserve = ScaleValue(28.0f);
        const Size textSize = MeasureTextValue(
            m_text.empty() ? L" " : m_text,
            fontSize,
            std::max(1.0f, availableWidth - reserve),
            TextWrapMode::NoWrap);
        return std::min(availableWidth, textSize.width + reserve);
    }

    float MeasurePreferredHeight(float width) const override {
        const float reserve = ScaleValue(28.0f);
        const Size textSize = MeasureTextValue(
            m_text.empty() ? L" " : m_text,
            fontSize,
            std::max(1.0f, width - reserve),
            TextWrapMode::NoWrap);
        return textSize.height + ScaleValue(10.0f);
    }

    void Render(IRenderer& renderer) override {
        SyncQuickSetToStyle();
        const StyleSpec resolved = m_style.Resolve(GetCurrentState());
        BoxDecoration deco = resolved.decoration.value_or(BoxDecoration{});
        deco.radius = CornerRadius::Uniform(ScaleValue(deco.radius.MaxRadius()));
        deco.border.width = Thickness{
            ScaleValue(deco.border.width.left),
            ScaleValue(deco.border.width.top),
            ScaleValue(deco.border.width.right),
            ScaleValue(deco.border.width.bottom)
        };
        if (resolved.opacity.has_value()) {
            deco.opacity = *resolved.opacity;
        }

        const Rect circle = Rect::Make(
            m_bounds.left,
            m_bounds.top + ScaleValue(2.0f),
            m_bounds.left + ScaleValue(18.0f),
            m_bounds.top + ScaleValue(20.0f));
        DrawBoxDecoration(renderer, circle, deco);
        if (m_checked) {
            const float inset = ScaleValue(5.0f);
            const Rect dot = Rect::Make(circle.left + inset, circle.top + inset, circle.right - inset, circle.bottom - inset);
            renderer.FillRoundedRect(dot, ColorFromHex(0x2D6CDF), ScaleValue(5.0f));
        }

        Rect textRect = m_bounds;
        textRect.left += ScaleValue(26.0f);
        const TextRenderOptions textOptions{
            TextWrapMode::NoWrap,
            TextHorizontalAlign::Start,
            TextVerticalAlign::Center
        };
        const Color fg = resolved.foreground.value_or(textColor);
        renderer.DrawTextW(
            m_text.c_str(),
            static_cast<UINT32>(m_text.size()),
            textRect,
            fg,
            ScaleValue(fontSize),
            textOptions);
    }

private:
    void SyncQuickSetToStyle() {
        if (!m_style.base.decoration.has_value()) {
            m_style.base.decoration = BoxDecoration{};
        }
        m_style.base.foreground = textColor;
    }

public:

    bool OnMouseDown(float x, float y) override {
        const bool hit = HitTest(x, y);
        if (hit) {
            m_pressed = true;
        }
        return m_pressed;
    }

    bool OnMouseUp(float x, float y) override {
        const bool hit = HitTest(x, y);
        if (m_pressed && hit) {
            SetChecked(true);
        }
        m_pressed = false;
        return hit;
    }

    bool OnMouseLeave() override {
        if (!m_pressed) {
            return false;
        }
        m_pressed = false;
        return true;
    }

    void SetText(std::wstring text) { m_text = std::move(text); }
    void SetGroup(std::wstring group) { m_group = std::move(group); }
    void SetChecked(bool checked) {
        if (checked && !m_checked) {
            for (RadioButton* button : s_groups[m_group]) {
                if (button != this) {
                    button->m_checked = false;
                }
            }
        }
        m_checked = checked;
    }
    bool IsChecked() const { return m_checked; }

private:
    std::wstring m_text;
    std::wstring m_group;
    bool m_checked = false;
    Color textColor = ColorFromHex(0xEDEDED);
    float fontSize = 14.0f;
    inline static std::map<std::wstring, std::vector<RadioButton*>> s_groups;
};

class Slider : public UIElement {
public:
    explicit Slider(std::wstring label = L"") : m_label(std::move(label)) {
        m_style = DefaultStyle(nullptr);
    }

    static ComponentStyle DefaultStyle(const Theme* theme = nullptr) {
        ComponentStyle s;
        BoxDecoration shell;
        shell.background = ComponentStyle::ThemeColor(theme, "input-bg", ColorFromHex(0x1F1F1F));
        shell.border.width = Thickness{1.0f, 1.0f, 1.0f, 1.0f};
        shell.border.color = ComponentStyle::ThemeColor(theme, "border", ColorFromHex(0x6A6A6A));
        shell.radius = CornerRadius::Uniform(ComponentStyle::ThemeNumber(theme, Theme::NumberCategory::Radius, "md", 8.0f));
        s.base.decoration = shell;
        s.base.foreground = ComponentStyle::ThemeColor(theme, "fg", ColorFromHex(0xEDEDED));
        s.base.fontSize = ComponentStyle::ThemeNumber(theme, Theme::NumberCategory::FontSize, "sm", 13.0f);

        BoxDecoration track;
        track.background = ComponentStyle::ThemeColor(theme, "surface-4", ColorFromHex(0x333333));
        track.radius = CornerRadius::Uniform(4.0f);
        s.base.track = track;

        BoxDecoration fill;
        fill.background = ComponentStyle::ThemeColor(theme, "accent", ColorFromHex(0x2D6CDF));
        fill.radius = CornerRadius::Uniform(4.0f);
        s.base.fill = fill;

        BoxDecoration thumb;
        thumb.background = ComponentStyle::ThemeColor(theme, "accent", ColorFromHex(0x2D6CDF));
        thumb.border.width = Thickness{1.0f, 1.0f, 1.0f, 1.0f};
        thumb.border.color = ComponentStyle::ThemeColor(theme, "border", ColorFromHex(0x6A6A6A));
        thumb.radius = CornerRadius::Uniform(8.0f);
        s.base.thumb = thumb;

        BoxDecoration focusedShell = shell;
        focusedShell.border.width = Thickness{2.0f, 2.0f, 2.0f, 2.0f};
        focusedShell.border.color = ComponentStyle::ThemeColor(theme, "border-focus", ColorFromHex(0xFFFFFF));
        s.overrides[static_cast<std::size_t>(StyleState::Focused)].decoration = focusedShell;

        BoxDecoration hoverThumb = thumb;
        hoverThumb.background = ComponentStyle::ThemeColor(theme, "accent", ColorFromHex(0x4A8AFF));
        s.overrides[static_cast<std::size_t>(StyleState::Hover)].thumb = hoverThumb;

        BoxDecoration pressedThumb = thumb;
        pressedThumb.background = ComponentStyle::ThemeColor(theme, "accent-soft", ColorFromHex(0x1A5AD0));
        s.overrides[static_cast<std::size_t>(StyleState::Pressed)].thumb = pressedThumb;

        return s;
    }

    void ApplyThemeDefaults() override {
        if (m_styleFromLayout) {
            return;
        }
        const Theme* theme = (m_context && m_context->theme) ? m_context->theme : nullptr;
        m_style = DefaultStyle(theme);
    }

    bool IsFocusable() const override { return true; }

    bool OnFocus() override {
        if (!m_hasFocus) {
            m_hasFocus = true;
            return true;
        }
        return false;
    }

    bool OnBlur() override {
        if (!m_hasFocus && !m_hovered && !m_dragging) {
            return false;
        }
        m_hasFocus = false;
        m_hovered = false;
        m_pressed = false;
        m_dragging = false;
        return true;
    }

    bool OnKeyDown(WPARAM keyCode, LPARAM /*lParam*/) override {
        if (keyCode == VK_LEFT || keyCode == VK_DOWN) {
            SetValue(m_value - m_step);
            return true;
        }
        if (keyCode == VK_RIGHT || keyCode == VK_UP) {
            SetValue(m_value + m_step);
            return true;
        }
        return false;
    }

    bool OnMouseDown(float x, float y) override {
        if (!HitTest(x, y)) {
            return false;
        }
        m_dragging = true;
        m_pressed = true;
        m_hovered = true;
        UpdateValueFromPoint(x);
        return true;
    }

    bool OnMouseMove(float x, float y) override {
        bool changed = false;
        const bool nextHover = HitTest(x, y);
        if (nextHover != m_hovered) {
            m_hovered = nextHover;
            changed = true;
        }
        if (m_dragging) {
            UpdateValueFromPoint(x);
            return true;
        }
        return changed;
    }

    bool OnMouseUp(float x, float y) override {
        if (!m_dragging && !m_pressed) {
            return false;
        }
        m_dragging = false;
        m_pressed = false;
        m_hovered = HitTest(x, y);
        return true;
    }

    bool OnMouseLeave() override {
        if (!m_dragging && !m_hovered && !m_pressed) {
            return false;
        }
        if (m_dragging) {
            m_dragging = false;
        }
        m_pressed = false;
        m_hovered = false;
        return true;
    }

    float MeasurePreferredWidth(float availableWidth) const override {
        return std::min(availableWidth, ScaleValue(220.0f));
    }

    float MeasurePreferredHeight(float width) const override {
        (void)width;
        return ScaleValue(42.0f);
    }

    void Render(IRenderer& renderer) override {
        SyncQuickSetToStyle();
        StyleState state = GetCurrentState();
        if (m_dragging) {
            state = StyleState::Pressed;
        }
        const StyleSpec resolved = m_style.Resolve(state);

        BoxDecoration shell = resolved.decoration.value_or(BoxDecoration{});
        shell.radius = CornerRadius::Uniform(ScaleValue(
            shell.radius.IsZero() ? cornerRadius : shell.radius.MaxRadius()));
        shell.border.width = Thickness{
            ScaleValue(shell.border.width.left),
            ScaleValue(shell.border.width.top),
            ScaleValue(shell.border.width.right),
            ScaleValue(shell.border.width.bottom)
        };
        if (resolved.opacity.has_value()) {
            shell.opacity = *resolved.opacity;
        }
        DrawBoxDecoration(renderer, m_bounds, shell);

        const float trackLeft = m_bounds.left + ScaleValue(14.0f);
        const float trackRight = m_bounds.right - ScaleValue(14.0f);
        const float trackTop = m_bounds.top + (m_bounds.bottom - m_bounds.top) * 0.5f - ScaleValue(4.0f);
        const float trackBottom = trackTop + ScaleValue(8.0f);
        const Rect trackRect = Rect::Make(trackLeft, trackTop, trackRight, trackBottom);

        BoxDecoration trackDeco = resolved.track.value_or(BoxDecoration{});
        if (!resolved.track.has_value()) {
            trackDeco.background = ColorFromHex(0x333333);
            trackDeco.radius = CornerRadius::Uniform(4.0f);
        }
        trackDeco.radius = CornerRadius::Uniform(ScaleValue(
            trackDeco.radius.IsZero() ? 4.0f : trackDeco.radius.MaxRadius()));
        DrawBoxDecoration(renderer, trackRect, trackDeco);

        const float range = std::max(0.001f, m_max - m_min);
        const float ratio = std::clamp((m_value - m_min) / range, 0.0f, 1.0f);
        const float position = trackLeft + (trackRight - trackLeft) * ratio;

        // Filled portion of the track up to the thumb.
        if (ratio > 0.001f) {
            BoxDecoration fillDeco = resolved.fill.value_or(BoxDecoration{});
            if (!resolved.fill.has_value()) {
                fillDeco.background = highlightColor;
                fillDeco.radius = CornerRadius::Uniform(4.0f);
            }
            fillDeco.radius = CornerRadius::Uniform(ScaleValue(
                fillDeco.radius.IsZero() ? 4.0f : fillDeco.radius.MaxRadius()));
            const Rect fillRect = Rect::Make(trackLeft, trackTop, position, trackBottom);
            DrawBoxDecoration(renderer, fillRect, fillDeco);
        }

        const Rect thumbRect = Rect::Make(
            position - ScaleValue(8.0f),
            trackTop - ScaleValue(6.0f),
            position + ScaleValue(8.0f),
            trackBottom + ScaleValue(6.0f));
        BoxDecoration thumbDeco = resolved.thumb.value_or(BoxDecoration{});
        if (!resolved.thumb.has_value()) {
            thumbDeco.background = highlightColor;
            thumbDeco.border.width = Thickness{1.0f, 1.0f, 1.0f, 1.0f};
            thumbDeco.border.color = ColorFromHex(0x6A6A6A);
            thumbDeco.radius = CornerRadius::Uniform(8.0f);
        }
        thumbDeco.radius = CornerRadius::Uniform(ScaleValue(
            thumbDeco.radius.IsZero() ? 8.0f : thumbDeco.radius.MaxRadius()));
        thumbDeco.border.width = Thickness{
            ScaleValue(thumbDeco.border.width.left),
            ScaleValue(thumbDeco.border.width.top),
            ScaleValue(thumbDeco.border.width.right),
            ScaleValue(thumbDeco.border.width.bottom)
        };
        DrawBoxDecoration(renderer, thumbRect, thumbDeco);

        if (!m_label.empty()) {
            Rect labelRect = m_bounds;
            labelRect.right = trackLeft - ScaleValue(8.0f);
            const TextRenderOptions textOptions{
                TextWrapMode::NoWrap,
                TextHorizontalAlign::Start,
                TextVerticalAlign::Center
            };
            const Color fg = resolved.foreground.value_or(textColor);
            const float size = resolved.fontSize.value_or(fontSize);
            renderer.DrawTextW(
                m_label.c_str(),
                static_cast<UINT32>(m_label.size()),
                labelRect,
                fg,
                ScaleValue(size),
                textOptions);
        }
    }

    void SetText(std::wstring text) { m_label = std::move(text); }
    void SetRange(float minValue, float maxValue) { m_min = minValue; m_max = maxValue; SetValue(m_value); }
    void SetValue(float value) { m_value = std::clamp(value, m_min, m_max); }
    void SetStep(float step) { m_step = std::max(0.001f, step); }

    // Legacy quick-set fields (still honored; synced into style before paint).
    float fontSize = 13.0f;
    Color background = ColorFromHex(0x1F1F1F);
    Color borderColor = ColorFromHex(0x6A6A6A);
    Color focusBorderColor = ColorFromHex(0xFFFFFF);
    Color textColor = ColorFromHex(0xEDEDED);
    Color highlightColor = ColorFromHex(0x2D6CDF);
    float cornerRadius = 8.0f;

private:
    void SyncQuickSetToStyle() {
        // Shell + text only. track/thumb/fill come from DefaultStyle / style block;
        // highlightColor is applied once via ApplyHighlightToStyle() when attributes change.
        if (!m_style.base.decoration.has_value()) {
            m_style.base.decoration = BoxDecoration{};
        }
        m_style.base.decoration->background = background;
        m_style.base.decoration->border.color = borderColor;
        m_style.base.decoration->radius = CornerRadius::Uniform(cornerRadius);
        m_style.base.foreground = textColor;
        m_style.base.fontSize = fontSize;
    }

    void ApplyHighlightToStyle() {
        if (!m_style.base.fill.has_value()) {
            m_style.base.fill = BoxDecoration{};
            m_style.base.fill->radius = CornerRadius::Uniform(4.0f);
        }
        m_style.base.fill->background = highlightColor;
        if (!m_style.base.thumb.has_value()) {
            m_style.base.thumb = BoxDecoration{};
            m_style.base.thumb->radius = CornerRadius::Uniform(8.0f);
            m_style.base.thumb->border.width = Thickness{1.0f, 1.0f, 1.0f, 1.0f};
            m_style.base.thumb->border.color = ColorFromHex(0x6A6A6A);
        }
        m_style.base.thumb->background = highlightColor;
    }

public:
    void SetHighlightColor(Color color) {
        highlightColor = color;
        ApplyHighlightToStyle();
    }

private:
    void UpdateValueFromPoint(float x) {
        const float trackLeft = m_bounds.left + ScaleValue(14.0f);
        const float trackRight = m_bounds.right - ScaleValue(14.0f);
        const float ratio = std::clamp((x - trackLeft) / (trackRight - trackLeft), 0.0f, 1.0f);
        SetValue(m_min + ratio * (m_max - m_min));
    }

    std::wstring m_label;
    float m_min = 0.0f;
    float m_max = 1.0f;
    float m_value = 0.5f;
    float m_step = 0.05f;
    bool m_dragging = false;
};

