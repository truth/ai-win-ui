#pragma once
// Modern foundational UI widgets (Modern Components v2):
// Avatar, Badge, Divider, SegmentedControl, Switch, SearchBox, AlertBanner, CircularProgress, EmptyState

#include "ui_icons.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <functional>
#include <string>
#include <vector>

class Avatar : public UIElement {
public:
    Avatar() = default;

    std::wstring initials;
    std::string icon = "user";
    float size = 36.0f;
    bool circle = true;
    float cornerRadius = 8.0f;
    Color background = ColorFromHex(0xE0E7FF);
    Color color = ColorFromHex(0x2764FF);
    Color borderColor = Color{0.0f, 0.0f, 0.0f, 0.0f};
    float borderWidth = 0.0f;
    std::string status; // "online", "busy", "away", "offline"

protected:
    float MeasurePreferredWidth(float availableWidth) const override {
        const float w = ScaleValue(size);
        return availableWidth > 0.0f ? std::min(availableWidth, w) : w;
    }

    float MeasurePreferredHeight(float /*width*/) const override {
        return ScaleValue(size);
    }

public:
    void Render(IRenderer& renderer) override {
        if (!renderer.IntersectsDirtyRect(m_bounds)) {
            return;
        }
        const float w = std::max(0.0f, m_bounds.right - m_bounds.left);
        const float h = std::max(0.0f, m_bounds.bottom - m_bounds.top);
        const float d = std::min(w, h);
        const Rect box{
            m_bounds.left + (w - d) * 0.5f,
            m_bounds.top + (h - d) * 0.5f,
            m_bounds.left + (w + d) * 0.5f,
            m_bounds.top + (h + d) * 0.5f
        };
        const float r = circle ? d * 0.5f : ScaleValue(cornerRadius);
        renderer.FillRoundedRect(box, background, r);
        if (borderWidth > 0.0f && borderColor.a > 0.001f) {
            renderer.DrawRoundedRect(box, borderColor, ScaleValue(borderWidth), r);
        }

        if (!initials.empty()) {
            DrawStyledText(
                renderer,
                initials,
                box,
                d * 0.36f,
                color,
                TextHorizontalAlign::Center,
                TextVerticalAlign::Center,
                700);
        } else {
            bool filledDefault = false;
            const char* pathData = VectorIcon::LookupIconPath(icon, filledDefault);
            if (pathData) {
                const float iconDim = d * 0.50f;
                const Rect iconBox{
                    box.left + (d - iconDim) * 0.5f,
                    box.top + (d - iconDim) * 0.5f,
                    box.left + (d + iconDim) * 0.5f,
                    box.top + (d + iconDim) * 0.5f
                };
                renderer.DrawSvgPath(
                    pathData,
                    24.0f,
                    24.0f,
                    iconBox,
                    filledDefault ? color : Color{0.0f, 0.0f, 0.0f, 0.0f},
                    filledDefault ? Color{0.0f, 0.0f, 0.0f, 0.0f} : color,
                    ScaleValue(1.9f));
            }
        }

        if (!status.empty()) {
            Color dotColor = ColorFromHex(0x94A3B8);
            if (status == "online" || status == "active") {
                dotColor = ColorFromHex(0x10B981);
            } else if (status == "busy" || status == "dnd") {
                dotColor = ColorFromHex(0xEF4444);
            } else if (status == "away" || status == "warning") {
                dotColor = ColorFromHex(0xF59E0B);
            }
            const float dotR = std::max(ScaleValue(4.2f), d * 0.135f);
            const float ringR = dotR + ScaleValue(1.8f);
            const float cx = box.right - dotR * 0.85f;
            const float cy = box.bottom - dotR * 0.85f;
            renderer.FillRoundedRect(
                Rect{cx - ringR, cy - ringR, cx + ringR, cy + ringR},
                ColorFromHex(0xFFFFFF),
                ringR);
            renderer.FillRoundedRect(
                Rect{cx - dotR, cy - dotR, cx + dotR, cy + dotR},
                dotColor,
                dotR);
        }
    }
};

class Badge : public UIElement {
public:
    Badge() = default;
    explicit Badge(std::wstring labelText, std::string badgeVariant = "neutral")
        : text(std::move(labelText)), variant(std::move(badgeVariant)) {}

    std::wstring text;
    std::string variant = "neutral"; // "success", "warning", "danger", "info", "primary", "neutral"
    bool dot = false;
    std::string icon;
    float fontSize = 11.5f;
    float cornerRadius = 999.0f;
    Color customBackground = Color{0.0f, 0.0f, 0.0f, 0.0f};
    Color customColor = Color{0.0f, 0.0f, 0.0f, 0.0f};
    Color customBorderColor = Color{0.0f, 0.0f, 0.0f, 0.0f};

    void ResolvePalette(Color& outBg, Color& outFg, Color& outDot) const {
        if (variant == "success") {
            outBg = ColorFromHex(0xECFDF5);
            outFg = ColorFromHex(0x059669);
            outDot = ColorFromHex(0x10B981);
        } else if (variant == "warning") {
            outBg = ColorFromHex(0xFFFBEB);
            outFg = ColorFromHex(0xD97706);
            outDot = ColorFromHex(0xF59E0B);
        } else if (variant == "danger" || variant == "error") {
            outBg = ColorFromHex(0xFEF2F2);
            outFg = ColorFromHex(0xDC2626);
            outDot = ColorFromHex(0xEF4444);
        } else if (variant == "info") {
            outBg = ColorFromHex(0xEFF6FF);
            outFg = ColorFromHex(0x2563EB);
            outDot = ColorFromHex(0x3B82F6);
        } else if (variant == "primary") {
            outBg = ColorFromHex(0x2764FF);
            outFg = ColorFromHex(0xFFFFFF);
            outDot = ColorFromHex(0xFFFFFF);
        } else {
            outBg = ColorFromHex(0xF1F5F9);
            outFg = ColorFromHex(0x475569);
            outDot = ColorFromHex(0x64748B);
        }
        if (customBackground.a > 0.001f) outBg = customBackground;
        if (customColor.a > 0.001f) outFg = customColor;
    }

protected:
    float MeasurePreferredWidth(float availableWidth) const override {
        const Size ts = MeasureTextValue(text, fontSize, 400.0f, TextWrapMode::NoWrap);
        float w = ts.width + ScaleValue(20.0f);
        if (dot) w += ScaleValue(11.0f);
        if (!icon.empty()) w += ScaleValue(15.0f);
        return availableWidth > 0.0f ? std::min(availableWidth, w) : w;
    }

    float MeasurePreferredHeight(float /*width*/) const override {
        return ScaleValue(24.0f);
    }

public:
    void Render(IRenderer& renderer) override {
        if (!renderer.IntersectsDirtyRect(m_bounds)) {
            return;
        }
        Color bg{}, fg{}, dotCol{};
        ResolvePalette(bg, fg, dotCol);
        const float h = std::max(0.0f, m_bounds.bottom - m_bounds.top);
        const float r = std::min(h * 0.5f, ScaleValue(cornerRadius));
        renderer.FillRoundedRect(m_bounds, bg, r);
        if (customBorderColor.a > 0.001f) {
            renderer.DrawRoundedRect(m_bounds, customBorderColor, ScaleValue(1.0f), r);
        }

        float contentLeft = m_bounds.left + ScaleValue(10.0f);
        const float cy = (m_bounds.top + m_bounds.bottom) * 0.5f;
        if (dot) {
            const float dr = ScaleValue(3.0f);
            renderer.FillRoundedRect(
                Rect{contentLeft, cy - dr, contentLeft + dr * 2.0f, cy + dr},
                dotCol,
                dr);
            contentLeft += ScaleValue(11.0f);
        } else if (!icon.empty()) {
            bool filledDefault = false;
            const char* pathData = VectorIcon::LookupIconPath(icon, filledDefault);
            if (pathData) {
                const float isz = ScaleValue(12.0f);
                renderer.DrawSvgPath(
                    pathData,
                    24.0f,
                    24.0f,
                    Rect{contentLeft, cy - isz * 0.5f, contentLeft + isz, cy + isz * 0.5f},
                    filledDefault ? fg : Color{0.0f, 0.0f, 0.0f, 0.0f},
                    filledDefault ? Color{0.0f, 0.0f, 0.0f, 0.0f} : fg,
                    ScaleValue(2.0f));
                contentLeft += ScaleValue(15.0f);
            }
        }

        const bool centered = !dot && icon.empty();
        DrawStyledText(
            renderer,
            text,
            centered ? m_bounds : Rect{contentLeft, m_bounds.top, m_bounds.right - ScaleValue(8.0f), m_bounds.bottom},
            ScaleValue(fontSize),
            fg,
            centered ? TextHorizontalAlign::Center : TextHorizontalAlign::Start,
            TextVerticalAlign::Center,
            600);
    }
};

class Divider : public UIElement {
public:
    Divider() = default;

    bool vertical = false;
    bool dashed = false;
    float thickness = 1.0f;
    Color color = ColorFromHex(0xE6ECF5);
    std::wstring text;
    Color textColor = ColorFromHex(0x94A3B8);
    float fontSize = 11.0f;

protected:
    float MeasurePreferredWidth(float availableWidth) const override {
        if (vertical) {
            return std::max(1.0f, ScaleValue(thickness));
        }
        return availableWidth;
    }

    float MeasurePreferredHeight(float /*width*/) const override {
        if (vertical) {
            return ScaleValue(24.0f);
        }
        if (!text.empty()) {
            return ScaleValue(20.0f);
        }
        return std::max(1.0f, ScaleValue(thickness));
    }

public:
    void Render(IRenderer& renderer) override {
        if (!renderer.IntersectsDirtyRect(m_bounds)) {
            return;
        }
        const float stroke = std::max(1.0f, ScaleValue(thickness));
        if (vertical) {
            const float cx = (m_bounds.left + m_bounds.right) * 0.5f;
            if (dashed) {
                renderer.DrawDashedLine(PointF{cx, m_bounds.top}, PointF{cx, m_bounds.bottom}, color, stroke, ScaleValue(4.0f), ScaleValue(3.0f));
            } else {
                renderer.DrawLine(PointF{cx, m_bounds.top}, PointF{cx, m_bounds.bottom}, color, stroke);
            }
            return;
        }

        const float cy = (m_bounds.top + m_bounds.bottom) * 0.5f;
        if (text.empty()) {
            if (dashed) {
                renderer.DrawDashedLine(PointF{m_bounds.left, cy}, PointF{m_bounds.right, cy}, color, stroke, ScaleValue(4.0f), ScaleValue(3.0f));
            } else {
                renderer.DrawLine(PointF{m_bounds.left, cy}, PointF{m_bounds.right, cy}, color, stroke);
            }
            return;
        }

        const Size ts = MeasureTextValue(text, fontSize, 240.0f, TextWrapMode::NoWrap);
        const float cx = (m_bounds.left + m_bounds.right) * 0.5f;
        const float gap = ScaleValue(10.0f);
        const float leftEnd = std::max(m_bounds.left, cx - ts.width * 0.5f - gap);
        const float rightStart = std::min(m_bounds.right, cx + ts.width * 0.5f + gap);

        if (leftEnd > m_bounds.left) {
            if (dashed) {
                renderer.DrawDashedLine(PointF{m_bounds.left, cy}, PointF{leftEnd, cy}, color, stroke, ScaleValue(4.0f), ScaleValue(3.0f));
            } else {
                renderer.DrawLine(PointF{m_bounds.left, cy}, PointF{leftEnd, cy}, color, stroke);
            }
        }
        if (m_bounds.right > rightStart) {
            if (dashed) {
                renderer.DrawDashedLine(PointF{rightStart, cy}, PointF{m_bounds.right, cy}, color, stroke, ScaleValue(4.0f), ScaleValue(3.0f));
            } else {
                renderer.DrawLine(PointF{rightStart, cy}, PointF{m_bounds.right, cy}, color, stroke);
            }
        }
        DrawStyledText(
            renderer,
            text,
            Rect{leftEnd, m_bounds.top, rightStart, m_bounds.bottom},
            ScaleValue(fontSize),
            textColor,
            TextHorizontalAlign::Center,
            TextVerticalAlign::Center,
            600);
    }
};

class SegmentedControl : public UIElement {
public:
    SegmentedControl() {
        m_cursor = CursorType::Hand;
    }

    std::vector<std::wstring> items{L"Day", L"Week", L"Month"};
    int selectedIndex = 0;
    float fontSize = 12.0f;
    float cornerRadius = 8.0f;
    Color trackColor = ColorFromHex(0xF4F6FA);
    Color thumbColor = ColorFromHex(0xFFFFFF);
    Color activeTextColor = ColorFromHex(0x181D27);
    Color inactiveTextColor = ColorFromHex(0x697586);
    std::function<void()> onChange;

protected:
    float MeasurePreferredWidth(float availableWidth) const override {
        if (items.empty()) return ScaleValue(120.0f);
        float total = ScaleValue(6.0f);
        for (const auto& item : items) {
            const Size ts = MeasureTextValue(item, fontSize, 200.0f, TextWrapMode::NoWrap);
            total += std::max(ScaleValue(54.0f), ts.width + ScaleValue(24.0f));
        }
        return availableWidth > 0.0f ? std::min(availableWidth, total) : total;
    }

    float MeasurePreferredHeight(float /*width*/) const override {
        return ScaleValue(32.0f);
    }

public:
    bool OnMouseMove(float x, float y) override {
        const bool baseChanged = UIElement::OnMouseMove(x, y);
        if (items.empty()) return baseChanged;
        const float pad = ScaleValue(3.0f);
        const float innerW = std::max(1.0f, (m_bounds.right - m_bounds.left) - pad * 2.0f);
        const float segW = innerW / static_cast<float>(items.size());
        int idx = static_cast<int>((x - (m_bounds.left + pad)) / segW);
        idx = std::clamp(idx, 0, static_cast<int>(items.size()) - 1);
        if (m_hoverIndex != idx) {
            m_hoverIndex = idx;
            return true;
        }
        return baseChanged;
    }

    bool OnMouseLeave() override {
        const bool baseChanged = UIElement::OnMouseLeave();
        if (m_hoverIndex != -1) {
            m_hoverIndex = -1;
            return true;
        }
        return baseChanged;
    }

    bool OnMouseDown(float x, float /*y*/) override {
        if (items.empty()) return false;
        const float pad = ScaleValue(3.0f);
        const float innerW = std::max(1.0f, (m_bounds.right - m_bounds.left) - pad * 2.0f);
        const float segW = innerW / static_cast<float>(items.size());
        int idx = static_cast<int>((x - (m_bounds.left + pad)) / segW);
        idx = std::clamp(idx, 0, static_cast<int>(items.size()) - 1);
        if (selectedIndex != idx) {
            if (!m_animInitialized) {
                m_animPos = static_cast<float>(selectedIndex);
                m_animInitialized = true;
            }
            selectedIndex = idx;
            InvalidateSelf(8.0f);
            if (onChange) onChange();
        }
        return true;
    }

    bool OnTimer(UINT_PTR timerId, uint32_t dtMs) override {
        bool changed = UIElement::OnTimer(timerId, dtMs);
        const float target = static_cast<float>(std::clamp(selectedIndex, 0, std::max(0, static_cast<int>(items.size()) - 1)));
        if (!m_animInitialized) {
            m_animPos = target;
            m_animInitialized = true;
            return changed;
        }
        const float diff = target - m_animPos;
        if (std::fabs(diff) > 0.005f) {
            const float step = std::min(1.0f, static_cast<float>(dtMs) / 90.0f);
            m_animPos += diff * step;
            if (std::fabs(target - m_animPos) <= 0.005f) {
                m_animPos = target;
            }
            InvalidateSelf(8.0f);
            return true;
        }
        return changed;
    }

    void Render(IRenderer& renderer) override {
        if (!renderer.IntersectsDirtyRect(m_bounds) || items.empty()) {
            return;
        }
        const float r = ScaleValue(cornerRadius);
        renderer.FillRoundedRect(m_bounds, trackColor, r);

        const float pad = ScaleValue(3.0f);
        const float innerW = std::max(1.0f, (m_bounds.right - m_bounds.left) - pad * 2.0f);
        const float segW = innerW / static_cast<float>(items.size());
        const int sel = std::clamp(selectedIndex, 0, static_cast<int>(items.size()) - 1);
        if (!m_animInitialized) {
            m_animPos = static_cast<float>(sel);
            m_animInitialized = true;
        }

        const float segR = std::max(ScaleValue(4.0f), r - pad * 0.5f);
        if (m_hoverIndex >= 0 && m_hoverIndex < static_cast<int>(items.size()) && m_hoverIndex != sel) {
            const float hLeft = m_bounds.left + pad + static_cast<float>(m_hoverIndex) * segW;
            const Rect hoverRect{hLeft, m_bounds.top + pad, hLeft + segW, m_bounds.bottom - pad};
            renderer.FillRoundedRect(hoverRect, Color{1.0f, 1.0f, 1.0f, 0.45f}, segR);
        }

        const float thumbLeft = m_bounds.left + pad + m_animPos * segW;
        const Rect thumbRect{thumbLeft, m_bounds.top + pad, thumbLeft + segW, m_bounds.bottom - pad};
        renderer.FillSoftShadow(thumbRect, segR, 0.0f, ScaleValue(1.5f), ScaleValue(6.0f), Color{0.06f, 0.09f, 0.16f, 0.08f});
        renderer.FillRoundedRect(thumbRect, thumbColor, segR);
        renderer.DrawRoundedRect(thumbRect, ColorFromHex(0xE6ECF5), ScaleValue(1.0f), segR);

        for (std::size_t i = 0; i < items.size(); ++i) {
            const float left = m_bounds.left + pad + static_cast<float>(i) * segW;
            const Rect segRect{left, m_bounds.top + pad, left + segW, m_bounds.bottom - pad};
            const bool isSel = (static_cast<int>(i) == sel);

            DrawStyledText(
                renderer,
                items[i],
                segRect,
                ScaleValue(fontSize),
                isSel ? activeTextColor : inactiveTextColor,
                TextHorizontalAlign::Center,
                TextVerticalAlign::Center,
                isSel ? 600 : 500);
        }
    }

private:
    int m_hoverIndex = -1;
    float m_animPos = 0.0f;
    bool m_animInitialized = false;
};

class Switch : public UIElement {
public:
    long GetUiaControlType() const override { return UIA::CheckBoxControlType; }

    Switch() {
        m_cursor = CursorType::Hand;
    }

    bool checked = false;
    std::wstring label;
    std::wstring onText;
    std::wstring offText;
    Color activeColor = ColorFromHex(0x2764FF);
    Color inactiveColor = ColorFromHex(0xCBD5E1);
    Color labelColor = ColorFromHex(0x181D27);
    float fontSize = 13.0f;
    std::function<void()> onChange;

    bool IsFocusable() const override { return !m_disabled; }

protected:
    float MeasurePreferredWidth(float availableWidth) const override {
        float w = ScaleValue(42.0f);
        const std::wstring& displayLabel = !label.empty() ? label : (checked ? onText : offText);
        if (!displayLabel.empty()) {
            const Size ts = MeasureTextValue(displayLabel, fontSize, 260.0f, TextWrapMode::NoWrap);
            w += ScaleValue(8.0f) + ts.width;
        }
        return availableWidth > 0.0f ? std::min(availableWidth, w) : w;
    }

    float MeasurePreferredHeight(float /*width*/) const override {
        return ScaleValue(24.0f);
    }

public:
    bool OnMouseDown(float /*x*/, float /*y*/) override {
        if (m_disabled) return false;
        if (!m_animInitialized) {
            m_animProgress = checked ? 1.0f : 0.0f;
            m_animInitialized = true;
        }
        checked = !checked;
        InvalidateSelf(8.0f);
        if (onChange) onChange();
        return true;
    }

    bool OnKeyDown(WPARAM keyCode, LPARAM /*lParam*/) override {
        if (m_disabled) return false;
        if (keyCode == VK_SPACE || keyCode == VK_RETURN) {
            if (!m_animInitialized) {
                m_animProgress = checked ? 1.0f : 0.0f;
                m_animInitialized = true;
            }
            checked = !checked;
            InvalidateSelf(8.0f);
            if (onChange) onChange();
            return true;
        }
        return false;
    }

    bool OnTimer(UINT_PTR timerId, uint32_t dtMs) override {
        bool changed = UIElement::OnTimer(timerId, dtMs);
        const float target = checked ? 1.0f : 0.0f;
        if (!m_animInitialized) {
            m_animProgress = target;
            m_animInitialized = true;
            return changed;
        }
        if (std::fabs(target - m_animProgress) > 0.004f) {
            const float step = static_cast<float>(dtMs) / 140.0f;
            if (m_animProgress < target) {
                m_animProgress = std::min(target, m_animProgress + step);
            } else {
                m_animProgress = std::max(target, m_animProgress - step);
            }
            InvalidateSelf(8.0f);
            return true;
        }
        return changed;
    }

    void Render(IRenderer& renderer) override {
        if (!renderer.IntersectsDirtyRect(m_bounds)) {
            return;
        }
        if (!m_animInitialized) {
            m_animProgress = checked ? 1.0f : 0.0f;
            m_animInitialized = true;
        }
        // Smoothstep easing on the linear progress
        const float t = std::clamp(m_animProgress, 0.0f, 1.0f);
        const float eased = t * t * (3.0f - 2.0f * t);

        const float trackW = ScaleValue(40.0f);
        const float trackH = ScaleValue(22.0f);
        const float cy = (m_bounds.top + m_bounds.bottom) * 0.5f;
        const Rect trackRect{
            m_bounds.left,
            cy - trackH * 0.5f,
            m_bounds.left + trackW,
            cy + trackH * 0.5f
        };
        const float r = trackH * 0.5f;
        const Color trackBg{
            inactiveColor.r + (activeColor.r - inactiveColor.r) * eased,
            inactiveColor.g + (activeColor.g - inactiveColor.g) * eased,
            inactiveColor.b + (activeColor.b - inactiveColor.b) * eased,
            inactiveColor.a + (activeColor.a - inactiveColor.a) * eased
        };
        renderer.FillRoundedRect(trackRect, trackBg, r);

        const float pad = ScaleValue(2.5f);
        const float knobD = trackH - pad * 2.0f;
        const float minLeft = trackRect.left + pad;
        const float maxLeft = trackRect.right - pad - knobD;
        const float knobLeft = minLeft + (maxLeft - minLeft) * eased;
        const Rect knobRect{
            knobLeft,
            trackRect.top + pad,
            knobLeft + knobD,
            trackRect.bottom - pad
        };
        const float knobR = knobD * 0.5f;
        renderer.FillSoftShadow(knobRect, knobR, 0.0f, ScaleValue(1.0f), ScaleValue(4.0f), Color{0.0f, 0.0f, 0.0f, 0.18f});
        renderer.FillRoundedRect(knobRect, ColorFromHex(0xFFFFFF), knobR);

        const std::wstring& displayLabel = !label.empty() ? label : (checked ? onText : offText);
        if (!displayLabel.empty()) {
            DrawStyledText(
                renderer,
                displayLabel,
                Rect{trackRect.right + ScaleValue(8.0f), m_bounds.top, m_bounds.right, m_bounds.bottom},
                ScaleValue(fontSize),
                labelColor,
                TextHorizontalAlign::Start,
                TextVerticalAlign::Center,
                500);
        }
    }

private:
    float m_animProgress = 0.0f;
    bool m_animInitialized = false;
};

class SearchBox : public UIElement {
public:
    long GetUiaControlType() const override { return UIA::EditControlType; }

    SearchBox() {
        m_cursor = CursorType::IBeam;
    }

    std::wstring text;
    std::wstring placeholder = L"Search anything...";
    std::string icon = "search";
    std::wstring shortcut;
    bool clearable = true;
    float cornerRadius = 10.0f;
    float fontSize = 13.0f;
    Color background = ColorFromHex(0xFFFFFF);
    Color borderColor = ColorFromHex(0xE6ECF5);
    Color focusBorderColor = ColorFromHex(0x2764FF);
    Color textColor = ColorFromHex(0x181D27);
    Color placeholderColor = ColorFromHex(0x8C97A8);
    Color iconColor = ColorFromHex(0x8C97A8);
    std::function<void()> onChange;

    bool IsFocusable() const override { return !m_disabled; }

protected:
    float MeasurePreferredWidth(float availableWidth) const override {
        return availableWidth > 0.0f ? availableWidth : ScaleValue(240.0f);
    }

    float MeasurePreferredHeight(float /*width*/) const override {
        return ScaleValue(38.0f);
    }

public:
    bool OnMouseDown(float x, float /*y*/) override {
        if (m_disabled) return false;
        if (clearable && !text.empty()) {
            const float clearLeft = m_bounds.right - ScaleValue(32.0f);
            if (x >= clearLeft) {
                text.clear();
                if (onChange) onChange();
                return true;
            }
        }
        return true;
    }

    bool OnChar(wchar_t ch) override {
        if (m_disabled) return false;
        if (ch == L'\b') {
            if (!text.empty()) {
                text.pop_back();
                if (onChange) onChange();
                return true;
            }
            return false;
        }
        if (ch >= 32) {
            text.push_back(ch);
            if (onChange) onChange();
            return true;
        }
        return false;
    }

    bool OnKeyDown(WPARAM keyCode, LPARAM /*lParam*/) override {
        if (m_disabled) return false;
        if (keyCode == VK_ESCAPE && !text.empty()) {
            text.clear();
            if (onChange) onChange();
            return true;
        }
        return false;
    }

    void Render(IRenderer& renderer) override {
        if (!renderer.IntersectsDirtyRect(m_bounds)) {
            return;
        }
        const float r = ScaleValue(cornerRadius);
        renderer.FillRoundedRect(m_bounds, background, r);
        const Color bCol = m_hasFocus ? focusBorderColor : borderColor;
        const float bW = m_hasFocus ? ScaleValue(1.5f) : ScaleValue(1.0f);
        renderer.DrawRoundedRect(m_bounds, bCol, bW, r);

        const float cy = (m_bounds.top + m_bounds.bottom) * 0.5f;
        const float iconSz = ScaleValue(15.0f);
        const float iconLeft = m_bounds.left + ScaleValue(13.0f);
        bool filledDefault = false;
        if (const char* pathData = VectorIcon::LookupIconPath(icon, filledDefault)) {
            renderer.DrawSvgPath(
                pathData,
                24.0f,
                24.0f,
                Rect{iconLeft, cy - iconSz * 0.5f, iconLeft + iconSz, cy + iconSz * 0.5f},
                filledDefault ? iconColor : Color{0.0f, 0.0f, 0.0f, 0.0f},
                filledDefault ? Color{0.0f, 0.0f, 0.0f, 0.0f} : iconColor,
                ScaleValue(2.0f));
        }

        float rightPad = ScaleValue(12.0f);
        if (clearable && !text.empty()) {
            const float xSz = ScaleValue(13.0f);
            const float xLeft = m_bounds.right - ScaleValue(24.0f);
            bool xFill = false;
            if (const char* xPath = VectorIcon::LookupIconPath("x", xFill)) {
                renderer.DrawSvgPath(
                    xPath,
                    24.0f,
                    24.0f,
                    Rect{xLeft, cy - xSz * 0.5f, xLeft + xSz, cy + xSz * 0.5f},
                    Color{0.0f, 0.0f, 0.0f, 0.0f},
                    placeholderColor,
                    ScaleValue(2.0f));
            }
            rightPad = ScaleValue(32.0f);
        } else if (!shortcut.empty()) {
            const Size scSize = MeasureTextValue(shortcut, 10.5f, 80.0f, TextWrapMode::NoWrap);
            const float badgeW = scSize.width + ScaleValue(12.0f);
            const float badgeH = ScaleValue(20.0f);
            const Rect badgeRect{
                m_bounds.right - ScaleValue(10.0f) - badgeW,
                cy - badgeH * 0.5f,
                m_bounds.right - ScaleValue(10.0f),
                cy + badgeH * 0.5f
            };
            const float br = ScaleValue(5.0f);
            renderer.FillRoundedRect(badgeRect, ColorFromHex(0xF1F5F9), br);
            renderer.DrawRoundedRect(badgeRect, ColorFromHex(0xE2E8F0), ScaleValue(1.0f), br);
            DrawStyledText(
                renderer,
                shortcut,
                badgeRect,
                ScaleValue(10.5f),
                ColorFromHex(0x64748B),
                TextHorizontalAlign::Center,
                TextVerticalAlign::Center,
                600);
            rightPad = badgeW + ScaleValue(18.0f);
        }

        const Rect textRect{
            iconLeft + iconSz + ScaleValue(9.0f),
            m_bounds.top,
            std::max(iconLeft + iconSz + ScaleValue(12.0f), m_bounds.right - rightPad),
            m_bounds.bottom
        };
        const bool showPlaceholder = text.empty();
        DrawStyledText(
            renderer,
            showPlaceholder ? placeholder : text,
            textRect,
            ScaleValue(fontSize),
            showPlaceholder ? placeholderColor : textColor,
            TextHorizontalAlign::Start,
            TextVerticalAlign::Center,
            showPlaceholder ? 400 : 500);
    }
};

class AlertBanner : public UIElement {
public:
    AlertBanner() = default;

    std::string variant = "info"; // "info", "success", "warning", "error"
    std::wstring title;
    std::wstring message;
    bool closable = false;
    bool dismissed = false;
    float cornerRadius = 10.0f;

    void ResolveAlertColors(Color& outBg, Color& outBorder, Color& outAccent, Color& outTitle, Color& outMsg, std::string& outIcon) const {
        if (variant == "success") {
            outBg = ColorFromHex(0xECFDF5);
            outBorder = ColorFromHex(0xA7F3D0);
            outAccent = ColorFromHex(0x059669);
            outTitle = ColorFromHex(0x065F46);
            outMsg = ColorFromHex(0x047857);
            outIcon = "check-circle";
        } else if (variant == "warning") {
            outBg = ColorFromHex(0xFFFBEB);
            outBorder = ColorFromHex(0xFDE68A);
            outAccent = ColorFromHex(0xD97706);
            outTitle = ColorFromHex(0x92400E);
            outMsg = ColorFromHex(0xB45309);
            outIcon = "alert-triangle";
        } else if (variant == "error" || variant == "danger") {
            outBg = ColorFromHex(0xFEF2F2);
            outBorder = ColorFromHex(0xFECACA);
            outAccent = ColorFromHex(0xDC2626);
            outTitle = ColorFromHex(0x991B1B);
            outMsg = ColorFromHex(0xB91C1C);
            outIcon = "alert-circle";
        } else {
            outBg = ColorFromHex(0xEFF6FF);
            outBorder = ColorFromHex(0xBFDBFE);
            outAccent = ColorFromHex(0x2563EB);
            outTitle = ColorFromHex(0x1E40AF);
            outMsg = ColorFromHex(0x1D4ED8);
            outIcon = "info";
        }
    }

protected:
    float MeasurePreferredWidth(float availableWidth) const override {
        return availableWidth > 0.0f ? availableWidth : ScaleValue(320.0f);
    }

    float MeasurePreferredHeight(float /*width*/) const override {
        if (dismissed) return 0.0f;
        if (!title.empty() && !message.empty()) {
            return ScaleValue(56.0f);
        }
        return ScaleValue(42.0f);
    }

public:
    bool OnMouseDown(float x, float /*y*/) override {
        if (dismissed) return false;
        if (closable && x >= m_bounds.right - ScaleValue(34.0f)) {
            dismissed = true;
            if (m_context && m_context->requestLayout) {
                m_context->requestLayout();
            }
            return true;
        }
        return false;
    }

    void Render(IRenderer& renderer) override {
        if (dismissed || !renderer.IntersectsDirtyRect(m_bounds)) {
            return;
        }
        Color bg{}, borderCol{}, accent{}, titleCol{}, msgCol{};
        std::string iconName;
        ResolveAlertColors(bg, borderCol, accent, titleCol, msgCol, iconName);

        const float r = ScaleValue(cornerRadius);
        renderer.FillRoundedRect(m_bounds, bg, r);
        renderer.DrawRoundedRect(m_bounds, borderCol, ScaleValue(1.0f), r);

        const float cy = (m_bounds.top + m_bounds.bottom) * 0.5f;
        const float iconSz = ScaleValue(17.0f);
        const float iconLeft = m_bounds.left + ScaleValue(14.0f);
        bool filled = false;
        if (const char* pathData = VectorIcon::LookupIconPath(iconName, filled)) {
            renderer.DrawSvgPath(
                pathData,
                24.0f,
                24.0f,
                Rect{iconLeft, cy - iconSz * 0.5f, iconLeft + iconSz, cy + iconSz * 0.5f},
                Color{0.0f, 0.0f, 0.0f, 0.0f},
                accent,
                ScaleValue(2.0f));
        }

        const float textLeft = iconLeft + iconSz + ScaleValue(11.0f);
        const float textRight = closable ? (m_bounds.right - ScaleValue(34.0f)) : (m_bounds.right - ScaleValue(14.0f));
        if (!title.empty() && !message.empty()) {
            DrawStyledText(
                renderer,
                title,
                Rect{textLeft, m_bounds.top + ScaleValue(8.0f), textRight, cy + ScaleValue(1.0f)},
                ScaleValue(12.5f),
                titleCol,
                TextHorizontalAlign::Start,
                TextVerticalAlign::Center,
                700);

            DrawStyledText(
                renderer,
                message,
                Rect{textLeft, cy - ScaleValue(1.0f), textRight, m_bounds.bottom - ScaleValue(7.0f)},
                ScaleValue(11.5f),
                msgCol,
                TextHorizontalAlign::Start,
                TextVerticalAlign::Center,
                500);
        } else {
            const std::wstring& singleText = !title.empty() ? title : message;
            DrawStyledText(
                renderer,
                singleText,
                Rect{textLeft, m_bounds.top, textRight, m_bounds.bottom},
                ScaleValue(12.5f),
                titleCol,
                TextHorizontalAlign::Start,
                TextVerticalAlign::Center,
                600);
        }

        if (closable) {
            const float xSz = ScaleValue(13.0f);
            const float xLeft = m_bounds.right - ScaleValue(25.0f);
            bool xFill = false;
            if (const char* xPath = VectorIcon::LookupIconPath("x", xFill)) {
                renderer.DrawSvgPath(
                    xPath,
                    24.0f,
                    24.0f,
                    Rect{xLeft, cy - xSz * 0.5f, xLeft + xSz, cy + xSz * 0.5f},
                    Color{0.0f, 0.0f, 0.0f, 0.0f},
                    accent,
                    ScaleValue(2.0f));
            }
        }
    }
};

class CircularProgress : public UIElement {
public:
    CircularProgress() = default;

    float value = 75.0f; // 0..100
    float size = 64.0f;
    float thickness = 6.0f;
    Color color = ColorFromHex(0x2764FF);
    Color trackColor = ColorFromHex(0xEEF2F7);
    Color textColor = ColorFromHex(0x181D27);
    bool showText = true;
    std::wstring label;
    float fontSize = 13.0f;

protected:
    float MeasurePreferredWidth(float availableWidth) const override {
        const float w = ScaleValue(size);
        return availableWidth > 0.0f ? std::min(availableWidth, w) : w;
    }

    float MeasurePreferredHeight(float /*width*/) const override {
        return ScaleValue(size);
    }

public:
    void Render(IRenderer& renderer) override {
        if (!renderer.IntersectsDirtyRect(m_bounds)) {
            return;
        }
        const float w = std::max(0.0f, m_bounds.right - m_bounds.left);
        const float h = std::max(0.0f, m_bounds.bottom - m_bounds.top);
        const float d = std::min(w, h);
        const Rect box{
            m_bounds.left + (w - d) * 0.5f,
            m_bounds.top + (h - d) * 0.5f,
            m_bounds.left + (w + d) * 0.5f,
            m_bounds.top + (h + d) * 0.5f
        };
        const float stroke = ScaleValue(thickness);

        // Draw background track in 24x24 viewBox space (radius = 10, center = 12,12)
        renderer.DrawSvgPath(
            "M12 2a10 10 0 1 0 0 20 10 10 0 1 0 0-20z",
            24.0f,
            24.0f,
            box,
            Color{0.0f, 0.0f, 0.0f, 0.0f},
            trackColor,
            stroke);

        const float pct = std::clamp(value, 0.0f, 100.0f) * 0.01f;
        if (pct > 0.005f) {
            if (pct >= 0.999f) {
                renderer.DrawSvgPath(
                    "M12 2a10 10 0 1 0 0 20 10 10 0 1 0 0-20z",
                    24.0f,
                    24.0f,
                    box,
                    Color{0.0f, 0.0f, 0.0f, 0.0f},
                    color,
                    stroke);
            } else {
                const float angle = (-0.5f + pct * 2.0f) * 3.14159265f;
                const float ex = 12.0f + 10.0f * std::cos(angle);
                const float ey = 12.0f + 10.0f * std::sin(angle);
                const int largeArc = (pct > 0.5f) ? 1 : 0;
                char buf[128];
                std::snprintf(buf, sizeof(buf), "M 12 2 A 10 10 0 %d 1 %.3f %.3f", largeArc, ex, ey);
                renderer.DrawSvgPath(
                    buf,
                    24.0f,
                    24.0f,
                    box,
                    Color{0.0f, 0.0f, 0.0f, 0.0f},
                    color,
                    stroke);
            }
        }

        if (showText) {
            std::wstring display = label;
            if (display.empty()) {
                wchar_t wbuf[32];
                std::swprintf(wbuf, 32, L"%d%%", static_cast<int>(std::round(std::clamp(value, 0.0f, 100.0f))));
                display = wbuf;
            }
            DrawStyledText(
                renderer,
                display,
                box,
                ScaleValue(fontSize),
                textColor,
                TextHorizontalAlign::Center,
                TextVerticalAlign::Center,
                700);
        }
    }
};

class EmptyState : public UIElement {
public:
    EmptyState() = default;

    std::string icon = "inbox";
    std::wstring title = L"No data available";
    std::wstring description = L"There are no records to display right now.";
    std::wstring actionText;
    Color accentColor = ColorFromHex(0x2764FF);
    Color titleColor = ColorFromHex(0x181D27);
    Color descriptionColor = ColorFromHex(0x697586);
    std::function<void()> onAction;

protected:
    float MeasurePreferredWidth(float availableWidth) const override {
        return availableWidth > 0.0f ? availableWidth : ScaleValue(280.0f);
    }

    float MeasurePreferredHeight(float /*width*/) const override {
        return actionText.empty() ? ScaleValue(132.0f) : ScaleValue(172.0f);
    }

public:
    bool OnMouseDown(float x, float y) override {
        if (!actionText.empty() && onAction) {
            const float btnW = ScaleValue(132.0f);
            const float btnH = ScaleValue(32.0f);
            const float cx = (m_bounds.left + m_bounds.right) * 0.5f;
            const float btnTop = m_bounds.bottom - ScaleValue(42.0f);
            if (x >= cx - btnW * 0.5f && x <= cx + btnW * 0.5f && y >= btnTop && y <= btnTop + btnH) {
                onAction();
                return true;
            }
        }
        return false;
    }

    void Render(IRenderer& renderer) override {
        if (!renderer.IntersectsDirtyRect(m_bounds)) {
            return;
        }
        const float cx = (m_bounds.left + m_bounds.right) * 0.5f;
        float topY = m_bounds.top + ScaleValue(14.0f);

        const float circleD = ScaleValue(44.0f);
        const Rect circleBox{cx - circleD * 0.5f, topY, cx + circleD * 0.5f, topY + circleD};
        renderer.FillRoundedRect(circleBox, ColorFromHex(0xEEF4FF), circleD * 0.5f);

        bool filled = false;
        if (const char* pathData = VectorIcon::LookupIconPath(icon, filled)) {
            const float isz = ScaleValue(20.0f);
            renderer.DrawSvgPath(
                pathData,
                24.0f,
                24.0f,
                Rect{cx - isz * 0.5f, topY + (circleD - isz) * 0.5f, cx + isz * 0.5f, topY + (circleD + isz) * 0.5f},
                Color{0.0f, 0.0f, 0.0f, 0.0f},
                accentColor,
                ScaleValue(2.0f));
        }
        topY += circleD + ScaleValue(10.0f);

        if (!title.empty()) {
            DrawStyledText(
                renderer,
                title,
                Rect{m_bounds.left + ScaleValue(12.0f), topY, m_bounds.right - ScaleValue(12.0f), topY + ScaleValue(22.0f)},
                ScaleValue(14.0f),
                titleColor,
                TextHorizontalAlign::Center,
                TextVerticalAlign::Center,
                700);
            topY += ScaleValue(22.0f);
        }

        if (!description.empty()) {
            DrawStyledText(
                renderer,
                description,
                Rect{m_bounds.left + ScaleValue(16.0f), topY, m_bounds.right - ScaleValue(16.0f), topY + ScaleValue(20.0f)},
                ScaleValue(12.0f),
                descriptionColor,
                TextHorizontalAlign::Center,
                TextVerticalAlign::Center,
                400);
            topY += ScaleValue(26.0f);
        }

        if (!actionText.empty()) {
            const Size ts = MeasureTextValue(actionText, 12.0f, 200.0f, TextWrapMode::NoWrap);
            const float btnW = std::max(ScaleValue(110.0f), ts.width + ScaleValue(28.0f));
            const float btnH = ScaleValue(30.0f);
            const Rect btnRect{cx - btnW * 0.5f, topY, cx + btnW * 0.5f, topY + btnH};
            const float br = ScaleValue(8.0f);
            renderer.FillRoundedRect(btnRect, accentColor, br);
            DrawStyledText(
                renderer,
                actionText,
                btnRect,
                ScaleValue(12.0f),
                ColorFromHex(0xFFFFFF),
                TextHorizontalAlign::Center,
                TextVerticalAlign::Center,
                600);
        }
    }
};

// ============================================================================
// Breadcrumb and BreadcrumbItem (Modern Components v3)
// ============================================================================

class BreadcrumbItem : public UIElement {
public:
    BreadcrumbItem() = default;
    explicit BreadcrumbItem(std::wstring t) : text(std::move(t)) {}

    std::wstring text;
    std::string icon;
    std::string href;
    bool active = false;
    float fontSize = 13.0f;
    Color color = ColorFromHex(0x64748B);
    Color activeColor = ColorFromHex(0x0F172A);
    Color hoverColor = ColorFromHex(0x2764FF);
    std::function<void()> onClick;

    bool isHovered = false;

protected:
    float MeasurePreferredWidth(float /*availableWidth*/) const override {
        float w = 0.0f;
        if (!icon.empty()) {
            w += ScaleValue(16.0f) + ScaleValue(5.0f);
        }
        if (!text.empty()) {
            const Size ts = const_cast<BreadcrumbItem*>(this)->MeasureTextValue(
                text, fontSize, 4096.0f, TextWrapMode::NoWrap);
            w += ts.width;
        }
        return w;
    }

    float MeasurePreferredHeight(float /*width*/) const override {
        return ScaleValue(24.0f);
    }

public:
    bool OnMouseMove(float /*x*/, float /*y*/) override {
        if (active) return false;
        if (!isHovered) {
            isHovered = true;
            InvalidateSelf(2.0f);
        }
        return false;
    }

    bool OnMouseLeave() override {
        if (isHovered) {
            isHovered = false;
            InvalidateSelf(2.0f);
            return true;
        }
        return false;
    }

    bool OnMouseDown(float /*x*/, float /*y*/) override {
        if (!active && onClick) {
            onClick();
            return true;
        }
        return false;
    }

    void Render(IRenderer& renderer) override {
        if (!renderer.IntersectsDirtyRect(m_bounds)) {
            return;
        }
        float curX = m_bounds.left;
        if (!icon.empty()) {
            bool filled = false;
            if (const char* path = VectorIcon::LookupIconPath(icon, filled)) {
                const float isz = ScaleValue(14.0f);
                const float iconTop = m_bounds.top + (m_bounds.bottom - m_bounds.top - isz) * 0.5f;
                const Rect iconRect{curX, iconTop, curX + isz, iconTop + isz};
                const Color ic = active ? activeColor : (isHovered ? hoverColor : color);
                renderer.DrawSvgPath(path, 24.0f, 24.0f, iconRect, Color{0.0f, 0.0f, 0.0f, 0.0f}, ic, ScaleValue(1.8f));
                curX += isz + ScaleValue(5.0f);
            }
        }
        if (!text.empty()) {
            const Rect textRect{curX, m_bounds.top, m_bounds.right, m_bounds.bottom};
            const Color tc = active ? activeColor : (isHovered ? hoverColor : color);
            const int weight = active ? 600 : 400;
            DrawStyledText(
                renderer,
                text,
                textRect,
                ScaleValue(fontSize),
                tc,
                TextHorizontalAlign::Start,
                TextVerticalAlign::Center,
                weight);
        }
    }
};

class Breadcrumb : public UIElement {
public:
    Breadcrumb() = default;

    std::wstring separator = L"/";
    float spacing = 8.0f;
    float fontSize = 13.0f;
    Color separatorColor = ColorFromHex(0x94A3B8);

    bool IsLayoutLeaf() const override { return true; }

protected:
    float MeasurePreferredWidth(float availableWidth) const override {
        float totalW = 0.0f;
        const float gap = ScaleValue(spacing);
        const Size sepSize = const_cast<Breadcrumb*>(this)->MeasureTextValue(
            separator, fontSize, 200.0f, TextWrapMode::NoWrap);
        bool first = true;
        for (const auto& child : m_children) {
            if (!child) continue;
            child->Measure(availableWidth, 40.0f);
            if (!first) {
                totalW += gap + sepSize.width + gap;
            }
            totalW += child->DesiredSize().width;
            first = false;
        }
        return totalW;
    }

    float MeasurePreferredHeight(float /*width*/) const override {
        float maxH = ScaleValue(24.0f);
        for (const auto& child : m_children) {
            if (child) {
                maxH = std::max(maxH, child->DesiredSize().height);
            }
        }
        return maxH;
    }

public:
    void Measure(float availableWidth, float /*availableHeight*/) override {
        const float width = GetPreferredWidth(availableWidth);
        m_desiredSize.width = width;
        if (HasFixedHeight()) {
            m_desiredSize.height = GetPreferredHeight(width);
        } else {
            m_desiredSize.height = ClampHeight(MeasurePreferredHeight(width));
        }
    }

    void Arrange(const Rect& finalRect) override {
        UIElement::Arrange(finalRect);
        float curX = finalRect.left;
        const float gap = ScaleValue(spacing);
        const Size sepSize = MeasureTextValue(separator, fontSize, 200.0f, TextWrapMode::NoWrap);

        // Auto mark last item as active if none is explicitly active
        bool anyExplicitActive = false;
        for (const auto& child : m_children) {
            if (auto* bi = dynamic_cast<BreadcrumbItem*>(child.get())) {
                if (bi->active) {
                    anyExplicitActive = true;
                    break;
                }
            }
        }
        if (!anyExplicitActive && !m_children.empty()) {
            if (auto* lastBi = dynamic_cast<BreadcrumbItem*>(m_children.back().get())) {
                lastBi->active = true;
            }
        }

        bool first = true;
        for (auto& child : m_children) {
            if (!child) continue;
            if (!first) {
                curX += gap + sepSize.width + gap;
            }
            const float cw = child->DesiredSize().width;
            const Rect childRect{curX, finalRect.top, curX + cw, finalRect.bottom};
            child->Arrange(childRect);
            curX += cw;
            first = false;
        }
    }

    void Render(IRenderer& renderer) override {
        if (!renderer.IntersectsDirtyRect(m_bounds)) {
            return;
        }
        const float gap = ScaleValue(spacing);
        const Size sepSize = MeasureTextValue(separator, fontSize, 200.0f, TextWrapMode::NoWrap);

        for (size_t i = 0; i < m_children.size(); ++i) {
            const auto& child = m_children[i];
            if (!child) continue;
            child->Render(renderer);

            if (i + 1 < m_children.size()) {
                const Rect childBounds = child->Bounds();
                const Rect sepRect{
                    childBounds.right + gap,
                    m_bounds.top,
                    childBounds.right + gap + sepSize.width,
                    m_bounds.bottom
                };
                DrawStyledText(
                    renderer,
                    separator,
                    sepRect,
                    ScaleValue(fontSize),
                    separatorColor,
                    TextHorizontalAlign::Center,
                    TextVerticalAlign::Center,
                    400);
            }
        }
    }
};

// ============================================================================
// Pagination (Modern Components v3)
// ============================================================================

class Pagination : public UIElement {
public:
    Pagination() = default;

    int current = 1;
    int total = 0;
    int pageSize = 10;
    bool showTotal = true;
    float itemSize = 32.0f;
    float spacing = 6.0f;
    float fontSize = 12.5f;
    float cornerRadius = 6.0f;
    Color activeColor = ColorFromHex(0x2764FF);
    Color activeTextColor = ColorFromHex(0xFFFFFF);
    Color hoverColor = ColorFromHex(0xEFF6FF);
    Color textColor = ColorFromHex(0x334155);
    Color disabledColor = ColorFromHex(0xCBD5E1);
    Color borderColor = ColorFromHex(0xE2E8F0);
    std::function<void(int page, int pageSize)> onPageChange;

    struct ButtonInfo {
        int page; // -1: prev, -2: next, -3: left ellipsis, -4: right ellipsis, >= 1: page number
        Rect rect;
        bool isCurrent = false;
        bool isDisabled = false;
        bool isHovered = false;
        std::wstring label;
    };
    mutable std::vector<ButtonInfo> m_buttons;

    int GetTotalPages() const {
        if (pageSize <= 0) return 1;
        return std::max(1, (total + pageSize - 1) / pageSize);
    }

    void SetCurrent(int page) {
        const int tp = GetTotalPages();
        const int newPage = std::clamp(page, 1, tp);
        if (newPage != current) {
            current = newPage;
            InvalidateSelf(4.0f);
            if (onPageChange) {
                onPageChange(current, pageSize);
            }
        }
    }

protected:
    std::wstring GetTotalString() const {
        return L"共 " + std::to_wstring(total) + L" 条";
    }

    std::vector<int> ComputePageNumbers() const {
        std::vector<int> pages;
        const int tp = GetTotalPages();
        if (tp <= 7) {
            for (int i = 1; i <= tp; ++i) pages.push_back(i);
        } else {
            if (current <= 4) {
                for (int i = 1; i <= 5; ++i) pages.push_back(i);
                pages.push_back(-4); // right ellipsis
                pages.push_back(tp);
            } else if (current >= tp - 3) {
                pages.push_back(1);
                pages.push_back(-3); // left ellipsis
                for (int i = tp - 4; i <= tp; ++i) pages.push_back(i);
            } else {
                pages.push_back(1);
                pages.push_back(-3);
                pages.push_back(current - 1);
                pages.push_back(current);
                pages.push_back(current + 1);
                pages.push_back(-4);
                pages.push_back(tp);
            }
        }
        return pages;
    }

    float MeasurePreferredWidth(float /*availableWidth*/) const override {
        float w = 0.0f;
        const float isz = ScaleValue(itemSize);
        const float gap = ScaleValue(spacing);

        if (showTotal) {
            const Size ts = const_cast<Pagination*>(this)->MeasureTextValue(
                GetTotalString(), fontSize, 400.0f, TextWrapMode::NoWrap);
            w += ts.width + ScaleValue(12.0f);
        }

        // Prev + Next
        w += isz * 2.0f;
        const auto pages = ComputePageNumbers();
        w += static_cast<float>(pages.size()) * isz;
        w += static_cast<float>(pages.size() + 1) * gap;
        return w;
    }

    float MeasurePreferredHeight(float /*width*/) const override {
        return ScaleValue(itemSize);
    }

public:
    void LayoutButtons() const {
        m_buttons.clear();
        const float isz = ScaleValue(itemSize);
        const float gap = ScaleValue(spacing);
        float curX = m_bounds.left;
        const float top = m_bounds.top + (m_bounds.bottom - m_bounds.top - isz) * 0.5f;

        if (showTotal) {
            const Size ts = const_cast<Pagination*>(this)->MeasureTextValue(
                GetTotalString(), fontSize, 400.0f, TextWrapMode::NoWrap);
            curX += ts.width + ScaleValue(12.0f);
        }

        // Prev button
        const int tp = GetTotalPages();
        ButtonInfo prevBtn;
        prevBtn.page = -1;
        prevBtn.rect = Rect{curX, top, curX + isz, top + isz};
        prevBtn.isDisabled = (current <= 1);
        prevBtn.label = L"‹";
        m_buttons.push_back(prevBtn);
        curX += isz + gap;

        // Number buttons
        const auto pages = ComputePageNumbers();
        for (int p : pages) {
            ButtonInfo b;
            b.page = p;
            b.rect = Rect{curX, top, curX + isz, top + isz};
            if (p == -3 || p == -4) {
                b.label = L"•••";
            } else {
                b.label = std::to_wstring(p);
                b.isCurrent = (p == current);
            }
            m_buttons.push_back(b);
            curX += isz + gap;
        }

        // Next button
        ButtonInfo nextBtn;
        nextBtn.page = -2;
        nextBtn.rect = Rect{curX, top, curX + isz, top + isz};
        nextBtn.isDisabled = (current >= tp);
        nextBtn.label = L"›";
        m_buttons.push_back(nextBtn);
    }

    bool OnMouseMove(float x, float y) override {
        bool changed = false;
        for (auto& btn : m_buttons) {
            const bool inside = (x >= btn.rect.left && x <= btn.rect.right &&
                                 y >= btn.rect.top && y <= btn.rect.bottom);
            if (inside != btn.isHovered) {
                btn.isHovered = inside;
                changed = true;
            }
        }
        if (changed) {
            InvalidateSelf(2.0f);
        }
        return false;
    }

    bool OnMouseLeave() override {
        bool changed = false;
        for (auto& btn : m_buttons) {
            if (btn.isHovered) {
                btn.isHovered = false;
                changed = true;
            }
        }
        if (changed) {
            InvalidateSelf(2.0f);
        }
        return changed;
    }

    bool OnMouseDown(float x, float y) override {
        LayoutButtons();
        for (const auto& btn : m_buttons) {
            if (x >= btn.rect.left && x <= btn.rect.right &&
                y >= btn.rect.top && y <= btn.rect.bottom) {
                if (btn.isDisabled) return false;
                if (btn.page == -1) {
                    SetCurrent(current - 1);
                    return true;
                }
                if (btn.page == -2) {
                    SetCurrent(current + 1);
                    return true;
                }
                if (btn.page == -3) {
                    SetCurrent(std::max(1, current - 5));
                    return true;
                }
                if (btn.page == -4) {
                    SetCurrent(std::min(GetTotalPages(), current + 5));
                    return true;
                }
                if (btn.page >= 1) {
                    SetCurrent(btn.page);
                    return true;
                }
            }
        }
        return false;
    }

    void Render(IRenderer& renderer) override {
        if (!renderer.IntersectsDirtyRect(m_bounds)) {
            return;
        }
        LayoutButtons();

        float curX = m_bounds.left;
        if (showTotal) {
            const std::wstring totalStr = GetTotalString();
            const Size ts = MeasureTextValue(totalStr, fontSize, 400.0f, TextWrapMode::NoWrap);
            const Rect totalRect{curX, m_bounds.top, curX + ts.width, m_bounds.bottom};
            DrawStyledText(
                renderer,
                totalStr,
                totalRect,
                ScaleValue(fontSize),
                textColor,
                TextHorizontalAlign::Start,
                TextVerticalAlign::Center,
                400);
        }

        const float cr = ScaleValue(cornerRadius);
        const float bw = ScaleValue(1.0f);

        for (const auto& btn : m_buttons) {
            if (btn.isCurrent) {
                renderer.FillRoundedRect(btn.rect, activeColor, cr);
                DrawStyledText(
                    renderer,
                    btn.label,
                    btn.rect,
                    ScaleValue(fontSize),
                    activeTextColor,
                    TextHorizontalAlign::Center,
                    TextVerticalAlign::Center,
                    600);
            } else if (btn.isDisabled) {
                renderer.DrawRoundedRect(btn.rect, borderColor, bw, cr);
                DrawStyledText(
                    renderer,
                    btn.label,
                    btn.rect,
                    ScaleValue(fontSize),
                    disabledColor,
                    TextHorizontalAlign::Center,
                    TextVerticalAlign::Center,
                    400);
            } else {
                if (btn.isHovered) {
                    renderer.FillRoundedRect(btn.rect, hoverColor, cr);
                }
                const Color bc = btn.isHovered ? activeColor : borderColor;
                renderer.DrawRoundedRect(btn.rect, bc, bw, cr);
                const Color tc = btn.isHovered ? activeColor : textColor;
                DrawStyledText(
                    renderer,
                    btn.label,
                    btn.rect,
                    ScaleValue(fontSize),
                    tc,
                    TextHorizontalAlign::Center,
                    TextVerticalAlign::Center,
                    500);
            }
        }
    }
};
