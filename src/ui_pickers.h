#pragma once
// Component module - part of the ai-win-ui library.
// See ui.h for the full umbrella include.
#include "ui_inputs.h"

class ProgressBar : public UIElement {
public:
    explicit ProgressBar(std::wstring label = L"") : m_label(std::move(label)) {
        m_style = DefaultStyle(nullptr);
    }

    static ComponentStyle DefaultStyle(const Theme* theme = nullptr) {
        ComponentStyle s;
        BoxDecoration shell;
        shell.background = ComponentStyle::ThemeColor(theme, "surface-2", ColorFromHex(0x1A2431));
        shell.border.width = Thickness{1.0f, 1.0f, 1.0f, 1.0f};
        shell.border.color = ComponentStyle::ThemeColor(theme, "border-subtle", ColorFromHex(0x3A4B5D));
        shell.radius = CornerRadius::Uniform(ComponentStyle::ThemeNumber(theme, Theme::NumberCategory::Radius, "md", 8.0f));
        s.base.decoration = shell;
        s.base.foreground = ComponentStyle::ThemeColor(theme, "fg", ColorFromHex(0xEAF3FF));
        s.base.fontSize = ComponentStyle::ThemeNumber(theme, Theme::NumberCategory::FontSize, "sm", 12.0f);

        BoxDecoration track;
        track.background = ComponentStyle::ThemeColor(theme, "surface-3", ColorFromHex(0x2A3747));
        track.radius = CornerRadius::Uniform(5.0f);
        s.base.track = track;

        BoxDecoration fill;
        fill.background = ComponentStyle::ThemeColor(theme, "accent", ColorFromHex(0x4E7BFF));
        fill.radius = CornerRadius::Uniform(5.0f);
        s.base.fill = fill;

        BoxDecoration focused = shell;
        focused.border.width = Thickness{2.0f, 2.0f, 2.0f, 2.0f};
        focused.border.color = ComponentStyle::ThemeColor(theme, "border-focus", ColorFromHex(0xFFFFFF));
        s.overrides[static_cast<std::size_t>(StyleState::Focused)].decoration = focused;

        BoxDecoration hoverFill = fill;
        hoverFill.background = ComponentStyle::ThemeColor(theme, "accent", ColorFromHex(0x6A93FF));
        s.overrides[static_cast<std::size_t>(StyleState::Hover)].fill = hoverFill;

        BoxDecoration pressedFill = fill;
        pressedFill.background = ComponentStyle::ThemeColor(theme, "accent-soft", ColorFromHex(0x3A68D8));
        s.overrides[static_cast<std::size_t>(StyleState::Pressed)].fill = pressedFill;

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
        if (!m_hasFocus && !m_dragging && !m_hovered) {
            return false;
        }
        m_hasFocus = false;
        m_dragging = false;
        m_hovered = false;
        m_pressed = false;
        return true;
    }

    bool OnKeyDown(WPARAM keyCode, LPARAM /*lParam*/) override {
        if (keyCode == VK_LEFT || keyCode == VK_DOWN) {
            return SetValueInternal(m_value - m_step);
        }
        if (keyCode == VK_RIGHT || keyCode == VK_UP) {
            return SetValueInternal(m_value + m_step);
        }
        if (keyCode == VK_HOME) {
            return SetValueInternal(m_min);
        }
        if (keyCode == VK_END) {
            return SetValueInternal(m_max);
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
        return UpdateValueFromPoint(x);
    }

    bool OnMouseMove(float x, float y) override {
        bool changed = false;
        const bool nextHover = HitTest(x, y);
        if (nextHover != m_hovered) {
            m_hovered = nextHover;
            changed = true;
        }
        if (m_dragging) {
            return UpdateValueFromPoint(x) || changed;
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
        m_dragging = false;
        m_pressed = false;
        m_hovered = false;
        return true;
    }

protected:
    float MeasurePreferredWidth(float availableWidth) const override {
        return std::min(availableWidth, ScaleValue(260.0f));
    }

    float MeasurePreferredHeight(float width) const override {
        (void)width;
        return ScaleValue(42.0f);
    }

public:
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

        const float insetX = ScaleValue(12.0f);
        const float insetY = ScaleValue(8.0f);
        const float barHeightPx = std::max(ScaleValue(4.0f), ScaleValue(barHeight));
        const float trackTop = m_bounds.top + insetY + std::max(0.0f, (m_bounds.Height() - insetY * 2.0f - barHeightPx) * 0.5f);
        const Rect trackRect = Rect::Make(
            m_bounds.left + insetX,
            trackTop,
            m_bounds.right - insetX,
            trackTop + barHeightPx);

        BoxDecoration trackDeco = resolved.track.value_or(BoxDecoration{});
        if (!resolved.track.has_value()) {
            trackDeco.background = trackColor;
            trackDeco.radius = CornerRadius::Uniform(barHeight * 0.5f);
        }
        const float trackRadius = ScaleValue(
            trackDeco.radius.IsZero() ? barHeight * 0.5f : trackDeco.radius.MaxRadius());
        trackDeco.radius = CornerRadius::Uniform(trackRadius);
        DrawBoxDecoration(renderer, trackRect, trackDeco);

        const float progress = GetNormalizedValue();
        const float filledWidth = trackRect.Width() * progress;
        if (filledWidth > 0.5f) {
            BoxDecoration fillDeco = resolved.fill.value_or(BoxDecoration{});
            if (!resolved.fill.has_value()) {
                fillDeco.background = fillColor;
                fillDeco.radius = CornerRadius::Uniform(barHeight * 0.5f);
            }
            fillDeco.radius = CornerRadius::Uniform(ScaleValue(
                fillDeco.radius.IsZero() ? barHeight * 0.5f : fillDeco.radius.MaxRadius()));
            const Rect fillRect = Rect::Make(trackRect.left, trackRect.top, trackRect.left + filledWidth, trackRect.bottom);
            DrawBoxDecoration(renderer, fillRect, fillDeco);
        }

        std::wstring overlayText = m_label;
        if (m_showValueText) {
            const int percent = static_cast<int>(std::round(progress * 100.0f));
            const std::wstring percentText = std::to_wstring(percent) + L"%";
            overlayText = overlayText.empty() ? percentText : (overlayText + L" " + percentText);
        }
        if (!overlayText.empty()) {
            const TextRenderOptions textOptions{
                TextWrapMode::NoWrap,
                TextHorizontalAlign::Center,
                TextVerticalAlign::Center
            };
            const Color fg = resolved.foreground.value_or(textColor);
            const float size = resolved.fontSize.value_or(fontSize);
            renderer.DrawTextW(
                overlayText.c_str(),
                static_cast<UINT32>(overlayText.size()),
                m_bounds,
                fg,
                ScaleValue(size),
                textOptions);
        }
    }

    void SetLabel(std::wstring label) { m_label = std::move(label); }
    void SetRange(float minValue, float maxValue) {
        m_min = minValue;
        m_max = std::max(minValue + 0.001f, maxValue);
        SetValue(m_value);
    }
    void SetValue(float value) { m_value = std::clamp(value, m_min, m_max); }
    void SetStep(float step) { m_step = std::max(0.001f, step); }
    void SetShowValueText(bool showValueText) { m_showValueText = showValueText; }
    void SetFillColor(Color color) {
        fillColor = color;
        if (!m_style.base.fill.has_value()) {
            m_style.base.fill = BoxDecoration{};
            m_style.base.fill->radius = CornerRadius::Uniform(5.0f);
        }
        m_style.base.fill->background = color;
    }
    void SetTrackColor(Color color) {
        trackColor = color;
        if (!m_style.base.track.has_value()) {
            m_style.base.track = BoxDecoration{};
            m_style.base.track->radius = CornerRadius::Uniform(5.0f);
        }
        m_style.base.track->background = color;
    }

    Color background = ColorFromHex(0x1A2431);
    Color borderColor = ColorFromHex(0x3A4B5D);
    Color focusBorderColor = ColorFromHex(0xFFFFFF);
    Color trackColor = ColorFromHex(0x2A3747);
    Color fillColor = ColorFromHex(0x4E7BFF);
    Color textColor = ColorFromHex(0xEAF3FF);
    float cornerRadius = 8.0f;
    float fontSize = 12.0f;
    float barHeight = 10.0f;

private:
    void SyncQuickSetToStyle() {
        if (!m_style.base.decoration.has_value()) {
            m_style.base.decoration = BoxDecoration{};
        }
        m_style.base.decoration->background = background;
        m_style.base.decoration->border.color = borderColor;
        m_style.base.decoration->radius = CornerRadius::Uniform(cornerRadius);
        m_style.base.foreground = textColor;
        m_style.base.fontSize = fontSize;
    }

    bool SetValueInternal(float value) {
        const float clamped = std::clamp(value, m_min, m_max);
        if (std::abs(clamped - m_value) <= 0.0001f) {
            return false;
        }
        m_value = clamped;
        return true;
    }

    bool UpdateValueFromPoint(float x) {
        const float insetX = ScaleValue(12.0f);
        const float left = m_bounds.left + insetX;
        const float right = m_bounds.right - insetX;
        const float width = std::max(1.0f, right - left);
        const float ratio = std::clamp((x - left) / width, 0.0f, 1.0f);
        return SetValueInternal(m_min + ratio * (m_max - m_min));
    }

    float GetNormalizedValue() const {
        return std::clamp((m_value - m_min) / std::max(0.001f, m_max - m_min), 0.0f, 1.0f);
    }

    std::wstring m_label;
    float m_min = 0.0f;
    float m_max = 100.0f;
    float m_value = 42.0f;
    float m_step = 1.0f;
    bool m_showValueText = true;
    bool m_dragging = false;
};

// Spin-box numeric editor: value field + up/down buttons.
// Keyboard: Up/Down (+/-step), PageUp/PageDown (+/-10*step), Home/End.
class NumericUpDown : public UIElement {
public:
    explicit NumericUpDown(std::wstring label = L"") : m_label(std::move(label)) {
        m_style = DefaultStyle(nullptr);
    }

    static ComponentStyle DefaultStyle(const Theme* theme = nullptr) {
        ComponentStyle s;
        BoxDecoration shell;
        shell.background = ComponentStyle::ThemeColor(theme, "input-bg", ColorFromHex(0x1A2431));
        shell.border.width = Thickness{1, 1, 1, 1};
        shell.border.color = ComponentStyle::ThemeColor(theme, "border-subtle", ColorFromHex(0x3A4B5D));
        shell.radius = CornerRadius::Uniform(
            ComponentStyle::ThemeNumber(theme, Theme::NumberCategory::Radius, "md", 8.0f));
        s.base.decoration = shell;
        s.base.foreground = ComponentStyle::ThemeColor(theme, "fg", ColorFromHex(0xEAF3FF));
        s.base.fontSize = ComponentStyle::ThemeNumber(theme, Theme::NumberCategory::FontSize, "sm", 13.0f);

        BoxDecoration focused = shell;
        focused.border.width = Thickness{2, 2, 2, 2};
        focused.border.color = ComponentStyle::ThemeColor(theme, "border-focus", ColorFromHex(0xFFFFFF));
        s.overrides[static_cast<std::size_t>(StyleState::Focused)].decoration = focused;

        BoxDecoration hover;
        hover.background = ComponentStyle::ThemeColor(theme, "surface-3", ColorFromHex(0x2F4258));
        s.overrides[static_cast<std::size_t>(StyleState::Hover)].decoration = hover;

        BoxDecoration pressed;
        pressed.background = ComponentStyle::ThemeColor(theme, "surface-4", ColorFromHex(0x3A5470));
        s.overrides[static_cast<std::size_t>(StyleState::Pressed)].decoration = pressed;
        return s;
    }

    void ApplyThemeDefaults() override {
        if (m_styleFromLayout) {
            return;
        }
        const Theme* theme = (m_context && m_context->theme) ? m_context->theme : nullptr;
        m_style = DefaultStyle(theme);
        SyncLegacyColorsFromStyle();
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
        if (!m_hasFocus && m_hoveredButton == 0 && m_pressedButton == 0) {
            return false;
        }
        m_hasFocus = false;
        m_hoveredButton = 0;
        m_pressedButton = 0;
        return true;
    }

    bool OnKeyDown(WPARAM keyCode, LPARAM /*lParam*/) override {
        switch (keyCode) {
            case VK_UP:
                return SetValueInternal(m_value + m_step);
            case VK_DOWN:
                return SetValueInternal(m_value - m_step);
            case VK_PRIOR:
                return SetValueInternal(m_value + m_step * 10.0f);
            case VK_NEXT:
                return SetValueInternal(m_value - m_step * 10.0f);
            case VK_HOME:
                return SetValueInternal(m_min);
            case VK_END:
                return SetValueInternal(m_max);
            default:
                return false;
        }
    }

    bool OnMouseMove(float x, float y) override {
        if (!HitTest(x, y)) {
            if (m_hoveredButton == 0) {
                return false;
            }
            m_hoveredButton = 0;
            return true;
        }
        const int nextHover = ButtonFromPoint(x, y);
        if (nextHover == m_hoveredButton) {
            return false;
        }
        m_hoveredButton = nextHover;
        return true;
    }

    bool OnMouseLeave() override {
        if (m_hoveredButton == 0 && m_pressedButton == 0) {
            return false;
        }
        m_hoveredButton = 0;
        m_pressedButton = 0;
        return true;
    }

    bool OnMouseDown(float x, float y) override {
        if (!HitTest(x, y)) {
            return false;
        }
        m_pressedButton = ButtonFromPoint(x, y);
        if (m_pressedButton == 1) {
            SetValueInternal(m_value + m_step);
            return true;
        }
        if (m_pressedButton == 2) {
            SetValueInternal(m_value - m_step);
            return true;
        }
        return true; // capture focus on value field
    }

    bool OnMouseUp(float x, float y) override {
        if (m_pressedButton == 0) {
            return false;
        }
        m_pressedButton = 0;
        m_hoveredButton = HitTest(x, y) ? ButtonFromPoint(x, y) : 0;
        return true;
    }

protected:
    float MeasurePreferredWidth(float availableWidth) const override {
        return std::min(availableWidth, ScaleValue(220.0f));
    }

    float MeasurePreferredHeight(float width) const override {
        (void)width;
        return ScaleValue(36.0f);
    }

public:
    void Render(IRenderer& renderer) override {
        const StyleSpec shell = m_style.Resolve(m_hasFocus ? StyleState::Focused : StyleState::Normal);
        Color shellBg = background;
        Color shellBorder = borderColor;
        float scaledCorner = ScaleValue(cornerRadius);
        Color shellFg = textColor;
        float shellFs = fontSize;
        if (shell.decoration.has_value()) {
            shellBg = shell.decoration->background;
            shellBorder = shell.decoration->border.color;
            if (!shell.decoration->radius.IsZero()) {
                scaledCorner = ScaleValue(shell.decoration->radius.MaxRadius());
            }
        }
        if (shell.foreground.has_value()) {
            shellFg = *shell.foreground;
        }
        if (shell.fontSize.has_value()) {
            shellFs = *shell.fontSize;
        }

        renderer.FillRoundedRect(m_bounds, shellBg, scaledCorner);
        renderer.DrawRoundedRect(m_bounds, shellBorder, 1.0f, scaledCorner);
        if (m_hasFocus) {
            renderer.DrawRoundedRect(m_bounds, focusBorderColor, 2.0f, scaledCorner);
        }

        const Rect buttons = ButtonsRect();
        // Button strip background
        renderer.FillRect(buttons, buttonStripBackground);
        renderer.DrawLine(
            PointF{buttons.left, m_bounds.top + 1.0f},
            PointF{buttons.left, m_bounds.bottom - 1.0f},
            shellBorder,
            1.0f);
        const float midY = (buttons.top + buttons.bottom) * 0.5f;
        renderer.DrawLine(
            PointF{buttons.left + 1.0f, midY},
            PointF{buttons.right - 1.0f, midY},
            borderColor,
            1.0f);

        DrawSpinButton(renderer, UpButtonRect(), /*up=*/true, m_hoveredButton == 1, m_pressedButton == 1);
        DrawSpinButton(renderer, DownButtonRect(), /*up=*/false, m_hoveredButton == 2, m_pressedButton == 2);

        // Label + value text in the field area
        const float padX = ScaleValue(10.0f);
        Rect field = m_bounds;
        field.right = buttons.left - ScaleValue(4.0f);
        field.left += padX;

        std::wstring display = FormatValue();
        if (!m_label.empty()) {
            display = m_label + L": " + display;
        }
        const TextRenderOptions textOptions{
            TextWrapMode::NoWrap,
            TextHorizontalAlign::Start,
            TextVerticalAlign::Center
        };
        renderer.DrawTextW(
            display.c_str(),
            static_cast<UINT32>(display.size()),
            field,
            shellFg,
            ScaleValue(shellFs),
            textOptions);
    }

    void SetLabel(std::wstring label) { m_label = std::move(label); }
    void SetRange(float minValue, float maxValue) {
        m_min = minValue;
        m_max = std::max(minValue, maxValue);
        SetValue(m_value);
    }
    void SetValue(float value) { m_value = SnapToStep(std::clamp(value, m_min, m_max)); }
    void SetStep(float step) { m_step = std::max(0.0001f, step); }
    void SetDecimalPlaces(int places) { m_decimalPlaces = std::clamp(places, 0, 6); }
    float Value() const { return m_value; }

    Color background = ColorFromHex(0x1A2431);
    Color borderColor = ColorFromHex(0x3A4B5D);
    Color focusBorderColor = ColorFromHex(0xFFFFFF);
    Color buttonStripBackground = ColorFromHex(0x243142);
    Color buttonHoverBackground = ColorFromHex(0x2F4258);
    Color buttonPressedBackground = ColorFromHex(0x3A5470);
    Color glyphColor = ColorFromHex(0xEAF3FF);
    Color textColor = ColorFromHex(0xEAF3FF);
    float cornerRadius = 8.0f;
    float fontSize = 13.0f;

private:
    void SyncLegacyColorsFromStyle() {
        if (m_style.base.decoration.has_value()) {
            background = m_style.base.decoration->background;
            borderColor = m_style.base.decoration->border.color;
            if (!m_style.base.decoration->radius.IsZero()) {
                cornerRadius = m_style.base.decoration->radius.MaxRadius();
            }
        }
        if (m_style.base.foreground.has_value()) {
            textColor = *m_style.base.foreground;
            glyphColor = *m_style.base.foreground;
        }
        if (m_style.base.fontSize.has_value()) {
            fontSize = *m_style.base.fontSize;
        }
        const StyleSpec hover = m_style.Resolve(StyleState::Hover);
        const StyleSpec pressed = m_style.Resolve(StyleState::Pressed);
        if (hover.decoration.has_value()) {
            buttonHoverBackground = hover.decoration->background;
        }
        if (pressed.decoration.has_value()) {
            buttonPressedBackground = pressed.decoration->background;
        }
    }

    // 0 = none, 1 = up, 2 = down
    int ButtonFromPoint(float x, float y) const {
        if (UpButtonRect().Contains(x, y)) {
            return 1;
        }
        if (DownButtonRect().Contains(x, y)) {
            return 2;
        }
        return 0;
    }

    Rect ButtonsRect() const {
        // Wide enough to click reliably at 100-150% DPI.
        const float width = ScaleValue(34.0f);
        return Rect::Make(m_bounds.right - width, m_bounds.top, m_bounds.right, m_bounds.bottom);
    }

    Rect UpButtonRect() const {
        const Rect buttons = ButtonsRect();
        const float midY = (buttons.top + buttons.bottom) * 0.5f;
        return Rect::Make(buttons.left, buttons.top, buttons.right, midY);
    }

    Rect DownButtonRect() const {
        const Rect buttons = ButtonsRect();
        const float midY = (buttons.top + buttons.bottom) * 0.5f;
        return Rect::Make(buttons.left, midY, buttons.right, buttons.bottom);
    }

    void DrawSpinButton(IRenderer& renderer, const Rect& rect, bool up, bool hovered, bool pressed) const {
        if (pressed) {
            renderer.FillRect(rect, buttonPressedBackground);
        } else if (hovered) {
            renderer.FillRect(rect, buttonHoverBackground);
        }

        const float cx = (rect.left + rect.right) * 0.5f;
        const float cy = (rect.top + rect.bottom) * 0.5f;
        const float halfW = ScaleValue(5.0f);
        const float halfH = ScaleValue(3.0f);
        std::vector<PointF> chevron;
        if (up) {
            chevron = {
                PointF{cx - halfW, cy + halfH * 0.5f},
                PointF{cx, cy - halfH},
                PointF{cx + halfW, cy + halfH * 0.5f}
            };
        } else {
            chevron = {
                PointF{cx - halfW, cy - halfH * 0.5f},
                PointF{cx, cy + halfH},
                PointF{cx + halfW, cy - halfH * 0.5f}
            };
        }
        renderer.DrawPolyline(chevron, glyphColor, ScaleValue(1.8f));
    }

    bool SetValueInternal(float value) {
        const float next = SnapToStep(std::clamp(value, m_min, m_max));
        if (std::abs(next - m_value) <= 0.00001f) {
            return false;
        }
        m_value = next;
        return true;
    }

    float SnapToStep(float value) const {
        if (m_step <= 0.0f) {
            return value;
        }
        const float relative = value - m_min;
        const float steps = std::round(relative / m_step);
        return std::clamp(m_min + steps * m_step, m_min, m_max);
    }

    std::wstring FormatValue() const {
        wchar_t buffer[64] = {};
        if (m_decimalPlaces <= 0) {
            swprintf_s(buffer, L"%.0f", static_cast<double>(m_value));
        } else {
            swprintf_s(buffer, L"%.*f", m_decimalPlaces, static_cast<double>(m_value));
        }
        return buffer;
    }

    std::wstring m_label;
    float m_min = 0.0f;
    float m_max = 100.0f;
    float m_value = 0.0f;
    float m_step = 1.0f;
    int m_decimalPlaces = 0;
    int m_hoveredButton = 0;
    int m_pressedButton = 0;
};

// Segment-based date/time editor (up-down style, no calendar popup in v1).
// Fields: Y/M/D and/or H/M[/S] depending on mode. Left/Right switch segments;
// Up/Down or chevrons adjust the active segment.
class DateTimePicker : public UIElement {
public:
    enum class Mode {
        Date,
        Time,
        DateTime
    };

    enum class Field {
        Year = 0,
        Month,
        Day,
        Hour,
        Minute,
        Second,
        Count
    };

    explicit DateTimePicker(std::wstring label = L"") : m_label(std::move(label)) {
        m_style = DefaultStyle(nullptr);
        SYSTEMTIME now = {};
        GetLocalTime(&now);
        m_year = now.wYear;
        m_month = now.wMonth;
        m_day = now.wDay;
        m_hour = now.wHour;
        m_minute = now.wMinute;
        m_second = now.wSecond;
        ClampDay();
        EnsureActiveFieldValid();
    }

    static ComponentStyle DefaultStyle(const Theme* theme = nullptr) {
        ComponentStyle s;
        BoxDecoration shell;
        shell.background = ComponentStyle::ThemeColor(theme, "input-bg", ColorFromHex(0x1A2431));
        shell.border.width = Thickness{1, 1, 1, 1};
        shell.border.color = ComponentStyle::ThemeColor(theme, "border-subtle", ColorFromHex(0x3A4B5D));
        shell.radius = CornerRadius::Uniform(
            ComponentStyle::ThemeNumber(theme, Theme::NumberCategory::Radius, "md", 8.0f));
        s.base.decoration = shell;
        s.base.foreground = ComponentStyle::ThemeColor(theme, "fg", ColorFromHex(0xEAF3FF));
        s.base.fontSize = ComponentStyle::ThemeNumber(theme, Theme::NumberCategory::FontSize, "sm", 13.0f);

        BoxDecoration focused = shell;
        focused.border.width = Thickness{2, 2, 2, 2};
        focused.border.color = ComponentStyle::ThemeColor(theme, "border-focus", ColorFromHex(0xFFFFFF));
        s.overrides[static_cast<std::size_t>(StyleState::Focused)].decoration = focused;

        BoxDecoration hover;
        hover.background = ComponentStyle::ThemeColor(theme, "surface-3", ColorFromHex(0x2F4258));
        s.overrides[static_cast<std::size_t>(StyleState::Hover)].decoration = hover;

        BoxDecoration pressed;
        pressed.background = ComponentStyle::ThemeColor(theme, "surface-4", ColorFromHex(0x3A5470));
        s.overrides[static_cast<std::size_t>(StyleState::Pressed)].decoration = pressed;
        return s;
    }

    void ApplyThemeDefaults() override {
        if (m_styleFromLayout) {
            return;
        }
        const Theme* theme = (m_context && m_context->theme) ? m_context->theme : nullptr;
        m_style = DefaultStyle(theme);
        SyncLegacyColorsFromStyle();
    }

    bool IsFocusable() const override { return true; }

    bool OnFocus() override {
        if (!m_hasFocus) {
            m_hasFocus = true;
            EnsureActiveFieldValid();
            return true;
        }
        return false;
    }

    bool OnBlur() override {
        if (!m_hasFocus && m_hoveredButton == 0 && m_pressedButton == 0) {
            return false;
        }
        m_hasFocus = false;
        m_hoveredButton = 0;
        m_pressedButton = 0;
        return true;
    }

    bool OnKeyDown(WPARAM keyCode, LPARAM /*lParam*/) override {
        switch (keyCode) {
            case VK_LEFT:
                return MoveActiveField(-1);
            case VK_RIGHT:
                return MoveActiveField(1);
            case VK_UP:
                return AdjustActiveField(1);
            case VK_DOWN:
                return AdjustActiveField(-1);
            case VK_PRIOR:
                return AdjustActiveField(10);
            case VK_NEXT:
                return AdjustActiveField(-10);
            case VK_HOME:
                return SetActiveFieldToMin();
            case VK_END:
                return SetActiveFieldToMax();
            default:
                return false;
        }
    }

    bool OnMouseMove(float x, float y) override {
        if (!HitTest(x, y)) {
            if (m_hoveredButton == 0 && m_hoveredField < 0) {
                return false;
            }
            m_hoveredButton = 0;
            m_hoveredField = -1;
            return true;
        }

        bool changed = false;
        const int nextButton = ButtonFromPoint(x, y);
        if (nextButton != m_hoveredButton) {
            m_hoveredButton = nextButton;
            changed = true;
        }

        const int nextField = nextButton == 0 ? FieldFromPoint(x, y) : -1;
        if (nextField != m_hoveredField) {
            m_hoveredField = nextField;
            changed = true;
        }
        return changed;
    }

    bool OnMouseLeave() override {
        if (m_hoveredButton == 0 && m_pressedButton == 0 && m_hoveredField < 0) {
            return false;
        }
        m_hoveredButton = 0;
        m_pressedButton = 0;
        m_hoveredField = -1;
        return true;
    }

    bool OnMouseDown(float x, float y) override {
        if (!HitTest(x, y)) {
            return false;
        }

        m_pressedButton = ButtonFromPoint(x, y);
        if (m_pressedButton == 1) {
            AdjustActiveField(1);
            return true;
        }
        if (m_pressedButton == 2) {
            AdjustActiveField(-1);
            return true;
        }

        const int field = FieldFromPoint(x, y);
        if (field >= 0) {
            m_activeField = field;
            return true;
        }
        return true;
    }

    bool OnMouseUp(float x, float y) override {
        if (m_pressedButton == 0) {
            return false;
        }
        m_pressedButton = 0;
        m_hoveredButton = HitTest(x, y) ? ButtonFromPoint(x, y) : 0;
        return true;
    }

protected:
    float MeasurePreferredWidth(float availableWidth) const override {
        float ideal = 220.0f;
        if (m_mode == Mode::DateTime) {
            ideal = m_showSeconds ? 300.0f : 260.0f;
        } else if (m_mode == Mode::Time) {
            ideal = m_showSeconds ? 180.0f : 150.0f;
        }
        if (!m_label.empty()) {
            ideal += 80.0f;
        }
        return std::min(availableWidth, ScaleValue(ideal));
    }

    float MeasurePreferredHeight(float width) const override {
        (void)width;
        return ScaleValue(36.0f);
    }

public:
    void Render(IRenderer& renderer) override {
        const StyleSpec shell = m_style.Resolve(m_hasFocus ? StyleState::Focused : StyleState::Normal);
        Color shellBg = background;
        Color shellBorder = borderColor;
        float scaledCorner = ScaleValue(cornerRadius);
        if (shell.decoration.has_value()) {
            shellBg = shell.decoration->background;
            shellBorder = shell.decoration->border.color;
            if (!shell.decoration->radius.IsZero()) {
                scaledCorner = ScaleValue(shell.decoration->radius.MaxRadius());
            }
        }
        if (shell.foreground.has_value()) {
            textColor = *shell.foreground;
        }
        if (shell.fontSize.has_value()) {
            fontSize = *shell.fontSize;
        }

        renderer.FillRoundedRect(m_bounds, shellBg, scaledCorner);
        renderer.DrawRoundedRect(m_bounds, shellBorder, 1.0f, scaledCorner);
        if (m_hasFocus) {
            renderer.DrawRoundedRect(m_bounds, focusBorderColor, 2.0f, scaledCorner);
        }

        const Rect buttons = ButtonsRect();
        renderer.FillRect(buttons, buttonStripBackground);
        renderer.DrawLine(
            PointF{buttons.left, m_bounds.top + 1.0f},
            PointF{buttons.left, m_bounds.bottom - 1.0f},
            shellBorder,
            1.0f);
        const float midY = (buttons.top + buttons.bottom) * 0.5f;
        renderer.DrawLine(
            PointF{buttons.left + 1.0f, midY},
            PointF{buttons.right - 1.0f, midY},
            shellBorder,
            1.0f);

        DrawSpinButton(renderer, UpButtonRect(), true, m_hoveredButton == 1, m_pressedButton == 1);
        DrawSpinButton(renderer, DownButtonRect(), false, m_hoveredButton == 2, m_pressedButton == 2);

        const float padX = ScaleValue(10.0f);
        float cursorX = m_bounds.left + padX;
        const float fieldTop = m_bounds.top;
        const float fieldBottom = m_bounds.bottom;
        const float fieldRightLimit = buttons.left - ScaleValue(4.0f);

        if (!m_label.empty()) {
            const std::wstring labelText = m_label + L": ";
            const float labelWidth = EstimateTextWidth(labelText);
            const Rect labelRect = Rect::Make(cursorX, fieldTop, std::min(cursorX + labelWidth, fieldRightLimit), fieldBottom);
            DrawFieldText(renderer, labelText, labelRect, textColor, false);
            cursorX = labelRect.right;
        }

        const auto fields = ActiveFields();
        for (size_t i = 0; i < fields.size(); ++i) {
            if (i > 0) {
                const wchar_t* sep = SeparatorBefore(fields[i]);
                const float sepWidth = EstimateTextWidth(sep);
                const Rect sepRect = Rect::Make(cursorX, fieldTop, std::min(cursorX + sepWidth, fieldRightLimit), fieldBottom);
                DrawFieldText(renderer, sep, sepRect, mutedTextColor, false);
                cursorX = sepRect.right;
            }

            const int fieldIndex = static_cast<int>(fields[i]);
            const std::wstring token = FormatField(fields[i]);
            const float tokenWidth = EstimateTextWidth(token);
            const Rect tokenRect = Rect::Make(cursorX, fieldTop, std::min(cursorX + tokenWidth + ScaleValue(4.0f), fieldRightLimit), fieldBottom);
            const bool active = m_hasFocus && m_activeField == fieldIndex;
            const bool hovered = m_hoveredField == fieldIndex;
            if (active) {
                renderer.FillRoundedRect(tokenRect, activeFieldBackground, ScaleValue(3.0f));
            } else if (hovered) {
                renderer.FillRoundedRect(tokenRect, hoverFieldBackground, ScaleValue(3.0f));
            }
            DrawFieldText(renderer, token, tokenRect, active ? activeFieldTextColor : textColor, true);
            cursorX = tokenRect.right;
            if (cursorX >= fieldRightLimit) {
                break;
            }
        }
    }

    void SetLabel(std::wstring label) { m_label = std::move(label); }
    void SetMode(Mode mode) {
        m_mode = mode;
        EnsureActiveFieldValid();
    }
    void SetShowSeconds(bool showSeconds) {
        m_showSeconds = showSeconds;
        EnsureActiveFieldValid();
    }
    void SetDate(int year, int month, int day) {
        m_year = std::clamp(year, 1970, 9999);
        m_month = std::clamp(month, 1, 12);
        m_day = day;
        ClampDay();
    }
    void SetTime(int hour, int minute, int second = 0) {
        m_hour = std::clamp(hour, 0, 23);
        m_minute = std::clamp(minute, 0, 59);
        m_second = std::clamp(second, 0, 59);
    }
    bool SetFromString(const std::wstring& text) {
        // Accepts: yyyy-MM-dd | HH:mm[:ss] | yyyy-MM-dd HH:mm[:ss]
        int y = m_year, mo = m_month, d = m_day, h = m_hour, mi = m_minute, s = m_second;
        const wchar_t* p = text.c_str();
        while (*p == L' ' || *p == L'\t') {
            ++p;
        }
        bool hasDate = false;
        bool hasTime = false;
        if (std::iswdigit(p[0]) && std::iswdigit(p[1]) && std::iswdigit(p[2]) && std::iswdigit(p[3]) && p[4] == L'-') {
            int parsedY = 0, parsedM = 0, parsedD = 0;
            if (swscanf_s(p, L"%d-%d-%d", &parsedY, &parsedM, &parsedD) >= 3) {
                y = parsedY;
                mo = parsedM;
                d = parsedD;
                hasDate = true;
                while (*p && *p != L' ' && *p != L'T') {
                    ++p;
                }
                while (*p == L' ' || *p == L'T') {
                    ++p;
                }
            }
        }
        if (std::iswdigit(p[0])) {
            int parsedH = 0, parsedMi = 0, parsedS = 0;
            const int count = swscanf_s(p, L"%d:%d:%d", &parsedH, &parsedMi, &parsedS);
            if (count >= 2) {
                h = parsedH;
                mi = parsedMi;
                if (count >= 3) {
                    s = parsedS;
                    m_showSeconds = true;
                }
                hasTime = true;
            }
        }
        if (!hasDate && !hasTime) {
            return false;
        }
        if (hasDate) {
            SetDate(y, mo, d);
        }
        if (hasTime) {
            SetTime(h, mi, s);
        }
        // Mode is controlled by SetMode / layout props; value only fills fields.
        EnsureActiveFieldValid();
        return true;
    }

    std::wstring ValueText() const {
        wchar_t buffer[64] = {};
        switch (m_mode) {
            case Mode::Date:
                swprintf_s(buffer, L"%04d-%02d-%02d", m_year, m_month, m_day);
                break;
            case Mode::Time:
                if (m_showSeconds) {
                    swprintf_s(buffer, L"%02d:%02d:%02d", m_hour, m_minute, m_second);
                } else {
                    swprintf_s(buffer, L"%02d:%02d", m_hour, m_minute);
                }
                break;
            case Mode::DateTime:
                if (m_showSeconds) {
                    swprintf_s(buffer, L"%04d-%02d-%02d %02d:%02d:%02d", m_year, m_month, m_day, m_hour, m_minute, m_second);
                } else {
                    swprintf_s(buffer, L"%04d-%02d-%02d %02d:%02d", m_year, m_month, m_day, m_hour, m_minute);
                }
                break;
        }
        return buffer;
    }

    Color background = ColorFromHex(0x1A2431);
    Color borderColor = ColorFromHex(0x3A4B5D);
    Color focusBorderColor = ColorFromHex(0xFFFFFF);
    Color buttonStripBackground = ColorFromHex(0x243142);
    Color buttonHoverBackground = ColorFromHex(0x2F4258);
    Color buttonPressedBackground = ColorFromHex(0x3A5470);
    Color activeFieldBackground = ColorFromHex(0x2D6CDF);
    Color hoverFieldBackground = ColorFromHex(0x2A3A4D);
    Color activeFieldTextColor = ColorFromHex(0xFFFFFF);
    Color glyphColor = ColorFromHex(0xEAF3FF);
    Color textColor = ColorFromHex(0xEAF3FF);
    Color mutedTextColor = ColorFromHex(0x8FA3B8);
    float cornerRadius = 8.0f;
    float fontSize = 13.0f;

private:
    void SyncLegacyColorsFromStyle() {
        if (m_style.base.decoration.has_value()) {
            background = m_style.base.decoration->background;
            borderColor = m_style.base.decoration->border.color;
            if (!m_style.base.decoration->radius.IsZero()) {
                cornerRadius = m_style.base.decoration->radius.MaxRadius();
            }
        }
        if (m_style.base.foreground.has_value()) {
            textColor = *m_style.base.foreground;
            glyphColor = *m_style.base.foreground;
        }
        if (m_style.base.fontSize.has_value()) {
            fontSize = *m_style.base.fontSize;
        }
        const StyleSpec hover = m_style.Resolve(StyleState::Hover);
        const StyleSpec pressed = m_style.Resolve(StyleState::Pressed);
        if (hover.decoration.has_value()) {
            buttonHoverBackground = hover.decoration->background;
            hoverFieldBackground = hover.decoration->background;
        }
        if (pressed.decoration.has_value()) {
            buttonPressedBackground = pressed.decoration->background;
        }
    }

    std::vector<Field> ActiveFields() const {
        std::vector<Field> fields;
        if (m_mode == Mode::Date || m_mode == Mode::DateTime) {
            fields.push_back(Field::Year);
            fields.push_back(Field::Month);
            fields.push_back(Field::Day);
        }
        if (m_mode == Mode::Time || m_mode == Mode::DateTime) {
            fields.push_back(Field::Hour);
            fields.push_back(Field::Minute);
            if (m_showSeconds) {
                fields.push_back(Field::Second);
            }
        }
        return fields;
    }

    void EnsureActiveFieldValid() {
        const auto fields = ActiveFields();
        if (fields.empty()) {
            m_activeField = static_cast<int>(Field::Year);
            return;
        }
        for (Field f : fields) {
            if (static_cast<int>(f) == m_activeField) {
                return;
            }
        }
        m_activeField = static_cast<int>(fields.front());
    }

    bool MoveActiveField(int delta) {
        const auto fields = ActiveFields();
        if (fields.empty()) {
            return false;
        }
        int index = 0;
        for (size_t i = 0; i < fields.size(); ++i) {
            if (static_cast<int>(fields[i]) == m_activeField) {
                index = static_cast<int>(i);
                break;
            }
        }
        const int next = (index + delta + static_cast<int>(fields.size())) % static_cast<int>(fields.size());
        if (static_cast<int>(fields[static_cast<size_t>(next)]) == m_activeField) {
            return false;
        }
        m_activeField = static_cast<int>(fields[static_cast<size_t>(next)]);
        return true;
    }

    bool AdjustActiveField(int delta) {
        switch (static_cast<Field>(m_activeField)) {
            case Field::Year:
                m_year = std::clamp(m_year + delta, 1970, 9999);
                ClampDay();
                return true;
            case Field::Month: {
                int month = m_month + delta;
                while (month < 1) {
                    month += 12;
                    m_year = std::clamp(m_year - 1, 1970, 9999);
                }
                while (month > 12) {
                    month -= 12;
                    m_year = std::clamp(m_year + 1, 1970, 9999);
                }
                m_month = month;
                ClampDay();
                return true;
            }
            case Field::Day: {
                const int dim = DaysInMonth(m_year, m_month);
                int day = m_day + delta;
                while (day < 1) {
                    int month = m_month - 1;
                    int year = m_year;
                    if (month < 1) {
                        month = 12;
                        year = std::clamp(year - 1, 1970, 9999);
                    }
                    m_year = year;
                    m_month = month;
                    day += DaysInMonth(m_year, m_month);
                }
                while (day > DaysInMonth(m_year, m_month)) {
                    day -= DaysInMonth(m_year, m_month);
                    int month = m_month + 1;
                    int year = m_year;
                    if (month > 12) {
                        month = 1;
                        year = std::clamp(year + 1, 1970, 9999);
                    }
                    m_year = year;
                    m_month = month;
                }
                m_day = day;
                return true;
            }
            case Field::Hour:
                m_hour = (m_hour + delta) % 24;
                if (m_hour < 0) {
                    m_hour += 24;
                }
                return true;
            case Field::Minute:
                m_minute = (m_minute + delta) % 60;
                if (m_minute < 0) {
                    m_minute += 60;
                }
                return true;
            case Field::Second:
                m_second = (m_second + delta) % 60;
                if (m_second < 0) {
                    m_second += 60;
                }
                return true;
            default:
                return false;
        }
    }

    bool SetActiveFieldToMin() {
        switch (static_cast<Field>(m_activeField)) {
            case Field::Year: m_year = 1970; ClampDay(); return true;
            case Field::Month: m_month = 1; ClampDay(); return true;
            case Field::Day: m_day = 1; return true;
            case Field::Hour: m_hour = 0; return true;
            case Field::Minute: m_minute = 0; return true;
            case Field::Second: m_second = 0; return true;
            default: return false;
        }
    }

    bool SetActiveFieldToMax() {
        switch (static_cast<Field>(m_activeField)) {
            case Field::Year: m_year = 9999; ClampDay(); return true;
            case Field::Month: m_month = 12; ClampDay(); return true;
            case Field::Day: m_day = DaysInMonth(m_year, m_month); return true;
            case Field::Hour: m_hour = 23; return true;
            case Field::Minute: m_minute = 59; return true;
            case Field::Second: m_second = 59; return true;
            default: return false;
        }
    }

    void ClampDay() {
        m_day = std::clamp(m_day, 1, DaysInMonth(m_year, m_month));
    }

    static bool IsLeapYear(int year) {
        return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
    }

    static int DaysInMonth(int year, int month) {
        static const int kDays[] = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
        if (month == 2 && IsLeapYear(year)) {
            return 29;
        }
        if (month < 1 || month > 12) {
            return 30;
        }
        return kDays[month];
    }

    std::wstring FormatField(Field field) const {
        wchar_t buffer[16] = {};
        switch (field) {
            case Field::Year: swprintf_s(buffer, L"%04d", m_year); break;
            case Field::Month: swprintf_s(buffer, L"%02d", m_month); break;
            case Field::Day: swprintf_s(buffer, L"%02d", m_day); break;
            case Field::Hour: swprintf_s(buffer, L"%02d", m_hour); break;
            case Field::Minute: swprintf_s(buffer, L"%02d", m_minute); break;
            case Field::Second: swprintf_s(buffer, L"%02d", m_second); break;
            default: break;
        }
        return buffer;
    }

    const wchar_t* SeparatorBefore(Field field) const {
        switch (field) {
            case Field::Month:
            case Field::Day:
                return L"-";
            case Field::Hour:
                return (m_mode == Mode::DateTime) ? L" " : L"";
            case Field::Minute:
            case Field::Second:
                return L":";
            default:
                return L"";
        }
    }

    float EstimateTextWidth(const std::wstring& text) const {
        // Approximate monospace-ish width for layout of segments.
        return ScaleValue(static_cast<float>(text.size()) * fontSize * 0.62f);
    }

    float EstimateTextWidth(const wchar_t* text) const {
        return EstimateTextWidth(std::wstring(text ? text : L""));
    }

    void DrawFieldText(IRenderer& renderer, const std::wstring& text, const Rect& rect, const Color& color, bool center) const {
        const TextRenderOptions options{
            TextWrapMode::NoWrap,
            center ? TextHorizontalAlign::Center : TextHorizontalAlign::Start,
            TextVerticalAlign::Center
        };
        renderer.DrawTextW(text.c_str(), static_cast<UINT32>(text.size()), rect, color, ScaleValue(fontSize), options);
    }

    void DrawFieldText(IRenderer& renderer, const wchar_t* text, const Rect& rect, const Color& color, bool center) const {
        DrawFieldText(renderer, std::wstring(text ? text : L""), rect, color, center);
    }

    Rect ButtonsRect() const {
        const float width = ScaleValue(34.0f);
        return Rect::Make(m_bounds.right - width, m_bounds.top, m_bounds.right, m_bounds.bottom);
    }

    Rect UpButtonRect() const {
        const Rect buttons = ButtonsRect();
        const float midY = (buttons.top + buttons.bottom) * 0.5f;
        return Rect::Make(buttons.left, buttons.top, buttons.right, midY);
    }

    Rect DownButtonRect() const {
        const Rect buttons = ButtonsRect();
        const float midY = (buttons.top + buttons.bottom) * 0.5f;
        return Rect::Make(buttons.left, midY, buttons.right, buttons.bottom);
    }

    int ButtonFromPoint(float x, float y) const {
        if (UpButtonRect().Contains(x, y)) {
            return 1;
        }
        if (DownButtonRect().Contains(x, y)) {
            return 2;
        }
        return 0;
    }

    int FieldFromPoint(float x, float y) const {
        if (!m_bounds.Contains(x, y) || ButtonFromPoint(x, y) != 0) {
            return -1;
        }

        const float padX = ScaleValue(10.0f);
        float cursorX = m_bounds.left + padX;
        const float fieldRightLimit = ButtonsRect().left - ScaleValue(4.0f);

        if (!m_label.empty()) {
            cursorX += EstimateTextWidth(m_label + L": ");
        }

        const auto fields = ActiveFields();
        for (size_t i = 0; i < fields.size(); ++i) {
            if (i > 0) {
                cursorX += EstimateTextWidth(SeparatorBefore(fields[i]));
            }
            const std::wstring token = FormatField(fields[i]);
            const float tokenWidth = EstimateTextWidth(token) + ScaleValue(4.0f);
            const Rect tokenRect = Rect::Make(cursorX, m_bounds.top, std::min(cursorX + tokenWidth, fieldRightLimit), m_bounds.bottom);
            if (tokenRect.Contains(x, y)) {
                return static_cast<int>(fields[i]);
            }
            cursorX = tokenRect.right;
            if (cursorX >= fieldRightLimit) {
                break;
            }
        }
        return -1;
    }

    void DrawSpinButton(IRenderer& renderer, const Rect& rect, bool up, bool hovered, bool pressed) const {
        if (pressed) {
            renderer.FillRect(rect, buttonPressedBackground);
        } else if (hovered) {
            renderer.FillRect(rect, buttonHoverBackground);
        }

        const float cx = (rect.left + rect.right) * 0.5f;
        const float cy = (rect.top + rect.bottom) * 0.5f;
        const float halfW = ScaleValue(4.0f);
        const float halfH = ScaleValue(2.5f);
        std::vector<PointF> chevron;
        if (up) {
            chevron = {
                PointF{cx - halfW, cy + halfH * 0.5f},
                PointF{cx, cy - halfH},
                PointF{cx + halfW, cy + halfH * 0.5f}
            };
        } else {
            chevron = {
                PointF{cx - halfW, cy - halfH * 0.5f},
                PointF{cx, cy + halfH},
                PointF{cx + halfW, cy - halfH * 0.5f}
            };
        }
        renderer.DrawPolyline(chevron, glyphColor, ScaleValue(1.6f));
    }

    std::wstring m_label;
    Mode m_mode = Mode::Date;
    bool m_showSeconds = false;
    int m_year = 2026;
    int m_month = 1;
    int m_day = 1;
    int m_hour = 0;
    int m_minute = 0;
    int m_second = 0;
    int m_activeField = static_cast<int>(Field::Year);
    int m_hoveredField = -1;
    int m_hoveredButton = 0;
    int m_pressedButton = 0;
};

// Multi-line text editor with a bold/italic formatting subset (v1).
// Hard line breaks via Enter. Ctrl+B / Ctrl+I toggle style on selection or typing.
// Optional load-time markup: **bold** and *italic*.
