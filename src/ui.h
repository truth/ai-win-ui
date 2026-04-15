#pragma once

#include "renderer.h"
#include <wrl/client.h>

#include <algorithm>
#include <functional>
#include <memory>
#include <map>
#include <string>
#include <vector>

struct Thickness {
    float left = 0;
    float top = 0;
    float right = 0;
    float bottom = 0;
};

static D2D1_SIZE_F MeasureTextLayout(const std::wstring& text, float fontSize, float maxWidth) {
    if (text.empty() || fontSize <= 0.0f) {
        return D2D1::SizeF(0.0f, 0.0f);
    }

    static Microsoft::WRL::ComPtr<IDWriteFactory> s_dwriteFactory;
    if (!s_dwriteFactory) {
        if (FAILED(DWriteCreateFactory(
                DWRITE_FACTORY_TYPE_SHARED,
                __uuidof(IDWriteFactory),
                reinterpret_cast<IUnknown**>(s_dwriteFactory.ReleaseAndGetAddressOf())))) {
            return D2D1::SizeF(0.0f, 0.0f);
        }
    }

    Microsoft::WRL::ComPtr<IDWriteTextFormat> textFormat;
    if (FAILED(s_dwriteFactory->CreateTextFormat(
            L"Segoe UI",
            nullptr,
            DWRITE_FONT_WEIGHT_NORMAL,
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            fontSize,
            L"",
            textFormat.ReleaseAndGetAddressOf()))) {
        return D2D1::SizeF(0.0f, 0.0f);
    }

    textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
    textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);

    const float layoutWidth = std::max(1.0f, maxWidth);
    Microsoft::WRL::ComPtr<IDWriteTextLayout> textLayout;
    if (FAILED(s_dwriteFactory->CreateTextLayout(
            text.c_str(),
            static_cast<UINT32>(text.size()),
            textFormat.Get(),
            layoutWidth,
            4096.0f,
            textLayout.ReleaseAndGetAddressOf()))) {
        return D2D1::SizeF(0.0f, 0.0f);
    }

    DWRITE_TEXT_METRICS metrics{};
    if (FAILED(textLayout->GetMetrics(&metrics))) {
        return D2D1::SizeF(0.0f, 0.0f);
    }

    return D2D1::SizeF(metrics.widthIncludingTrailingWhitespace, metrics.height);
}

class UIElement {
public:
    virtual ~UIElement() = default;

    void SetBounds(const D2D1_RECT_F& rect) { m_bounds = rect; }
    D2D1_RECT_F Bounds() const { return m_bounds; }
    void SetMargin(const Thickness& margin) { m_margin = margin; }
    const Thickness& Margin() const { return m_margin; }
    void SetFixedWidth(float width) { m_fixedWidth = width; }
    void SetFixedHeight(float height) { m_fixedHeight = height; }
    bool HasFixedWidth() const { return m_fixedWidth >= 0.0f; }
    bool HasFixedHeight() const { return m_fixedHeight >= 0.0f; }

    float GetPreferredWidth(float availableWidth) const {
        const float clampedWidth = std::max(0.0f, availableWidth);
        const float preferredWidth = HasFixedWidth() ? m_fixedWidth : MeasurePreferredWidth(clampedWidth);
        return std::clamp(preferredWidth, 0.0f, clampedWidth);
    }

    float GetPreferredHeight(float width) const {
        const float clampedWidth = std::max(0.0f, width);
        if (HasFixedHeight()) {
            return std::max(0.0f, m_fixedHeight);
        }
        return std::max(0.0f, MeasurePreferredHeight(clampedWidth));
    }

    virtual void Measure(float availableWidth, float availableHeight) {
        (void)availableHeight;
        m_desiredSize.width = GetPreferredWidth(availableWidth);
        m_desiredSize.height = GetPreferredHeight(m_desiredSize.width);
    }
    virtual void Arrange(const D2D1_RECT_F& finalRect) { m_bounds = finalRect; }
    virtual void Render(Renderer& renderer) {
        for (auto& child : m_children) {
            child->Render(renderer);
        }
    }

    virtual bool IsFocusable() const { return false; }
    bool HasFocus() const { return m_hasFocus; }

    virtual bool OnFocus() {
        if (!m_hasFocus) {
            m_hasFocus = true;
            return true;
        }
        return false;
    }

    virtual bool OnBlur() {
        if (m_hasFocus) {
            m_hasFocus = false;
            return true;
        }
        return false;
    }

    virtual bool OnKeyDown(WPARAM /*keyCode*/, LPARAM /*lParam*/) {
        return false;
    }

    virtual bool OnKeyUp(WPARAM /*keyCode*/, LPARAM /*lParam*/) {
        return false;
    }

    virtual bool OnChar(wchar_t /*ch*/) {
        return false;
    }

    virtual bool OnTimer(UINT_PTR /*timerId*/) {
        return false;
    }

    virtual UIElement* FindFocusableAt(float x, float y) {
        for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
            if ((*it)->HitTest(x, y)) {
                if (auto* focusable = (*it)->FindFocusableAt(x, y)) {
                    return focusable;
                }
            }
        }
        return IsFocusable() && HitTest(x, y) ? const_cast<UIElement*>(this) : nullptr;
    }

    virtual void CollectFocusable(std::vector<UIElement*>& out) {
        if (IsFocusable()) {
            out.push_back(this);
        }
        for (auto& child : m_children) {
            child->CollectFocusable(out);
        }
    }

    virtual bool OnMouseMove(float x, float y) {
        bool changed = false;
        bool hitChild = false;
        for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
            if (!hitChild && (*it)->HitTest(x, y)) {
                changed = (*it)->OnMouseMove(x, y) || changed;
                hitChild = true;
            } else {
                changed = (*it)->OnMouseLeave() || changed;
            }
        }
        return changed;
    }

    virtual bool OnMouseDown(float x, float y) {
        for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
            if ((*it)->HitTest(x, y) && (*it)->OnMouseDown(x, y)) {
                return true;
            }
        }
        return false;
    }

    virtual bool OnMouseUp(float x, float y) {
        bool changed = false;
        for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
            changed = (*it)->OnMouseUp(x, y) || changed;
        }
        return changed;
    }

    virtual bool OnMouseLeave() {
        bool changed = false;
        for (auto& child : m_children) {
            changed = child->OnMouseLeave() || changed;
        }
        return changed;
    }

protected:
    virtual float MeasurePreferredWidth(float availableWidth) const {
        return availableWidth;
    }

    virtual float MeasurePreferredHeight(float width) const {
        return 36.0f;
    }

public:
    bool HitTest(float x, float y) const {
        return x >= m_bounds.left && x <= m_bounds.right && y >= m_bounds.top && y <= m_bounds.bottom;
    }

    void AddChild(std::unique_ptr<UIElement> child) { m_children.emplace_back(std::move(child)); }
    std::vector<std::unique_ptr<UIElement>>& Children() { return m_children; }

protected:
    D2D1_RECT_F m_bounds = D2D1::RectF();
    D2D1_SIZE_F m_desiredSize = D2D1::SizeF();
    std::vector<std::unique_ptr<UIElement>> m_children;
    Thickness m_margin{};
    float m_fixedWidth = -1.0f;
    float m_fixedHeight = -1.0f;
    bool m_hasFocus = false;
};

class Panel : public UIElement {
public:
    enum class AlignItems {
        Stretch,
        Start,
        Center,
        End
    };

    enum class JustifyContent {
        Start,
        Center,
        End,
        SpaceBetween
    };

    Thickness padding{8, 8, 8, 8};
    float spacing = 6.0f;
    float cornerRadius = 0.0f;
    D2D1_COLOR_F background = D2D1::ColorF(0x1E1E1E);
    AlignItems alignItems = AlignItems::Stretch;
    JustifyContent justifyContent = JustifyContent::Start;

    void Arrange(const D2D1_RECT_F& finalRect) override {
        UIElement::Arrange(finalRect);

        const float contentLeft = m_bounds.left + padding.left;
        const float contentTop = m_bounds.top + padding.top;
        const float contentWidth = std::max(0.0f, (m_bounds.right - m_bounds.left) - padding.left - padding.right);
        const float contentHeight = std::max(0.0f, (m_bounds.bottom - m_bounds.top) - padding.top - padding.bottom);

        struct ChildLayout {
            UIElement* element;
            Thickness margin;
            float width;
            float height;
            float totalHeight;
        };

        std::vector<ChildLayout> layouts;
        layouts.reserve(m_children.size());

        float totalHeight = 0.0f;
        float totalChildHeight = 0.0f;
        bool first = true;
        for (auto& child : m_children) {
            const Thickness margin = child->Margin();
            const float availableChildWidth = std::max(0.0f, contentWidth - margin.left - margin.right);
            const float childWidth = (alignItems == AlignItems::Stretch && !child->HasFixedWidth())
                ? availableChildWidth
                : child->GetPreferredWidth(availableChildWidth);
            const float childHeight = child->GetPreferredHeight(childWidth);
            const float blockHeight = margin.top + childHeight + margin.bottom;

            if (!first) {
                totalHeight += spacing;
            }
            totalHeight += blockHeight;
            totalChildHeight += blockHeight;
            first = false;

            layouts.push_back(ChildLayout{
                child.get(),
                margin,
                childWidth,
                childHeight,
                blockHeight
            });
        }

        float y = contentTop;
        float spacingBetweenChildren = layouts.size() > 1 ? spacing : 0.0f;

        switch (justifyContent) {
            case JustifyContent::Center:
                y += std::max(0.0f, (contentHeight - totalHeight) * 0.5f);
                break;
            case JustifyContent::End:
                y += std::max(0.0f, contentHeight - totalHeight);
                break;
            case JustifyContent::SpaceBetween:
                if (layouts.size() > 1) {
                    const float distributedSpacing = (contentHeight - totalChildHeight) / static_cast<float>(layouts.size() - 1);
                    spacingBetweenChildren = std::max(spacing, distributedSpacing);
                }
                break;
            case JustifyContent::Start:
            default:
                break;
        }

        for (size_t i = 0; i < layouts.size(); ++i) {
            const auto& layout = layouts[i];
            const float availableChildWidth = std::max(0.0f, contentWidth - layout.margin.left - layout.margin.right);

            float x = contentLeft + layout.margin.left;
            if (alignItems == AlignItems::Center) {
                x += std::max(0.0f, (availableChildWidth - layout.width) * 0.5f);
            } else if (alignItems == AlignItems::End) {
                x += std::max(0.0f, availableChildWidth - layout.width);
            }

            y += layout.margin.top;
            layout.element->Arrange(D2D1::RectF(x, y, x + layout.width, y + layout.height));
            y += layout.height + layout.margin.bottom;
            if (i + 1 < layouts.size()) {
                y += spacingBetweenChildren;
            }
        }
    }

protected:
    float MeasurePreferredHeight(float width) const override {
        float totalHeight = padding.top + padding.bottom;
        const float contentWidth = std::max(0.0f, width - padding.left - padding.right);
        bool first = true;
        for (const auto& child : m_children) {
            if (!first) {
                totalHeight += spacing;
            }
            const Thickness& margin = child->Margin();
            const float availableChildWidth = std::max(0.0f, contentWidth - margin.left - margin.right);
            const float childWidth = (alignItems == AlignItems::Stretch && !child->HasFixedWidth())
                ? availableChildWidth
                : child->GetPreferredWidth(availableChildWidth);
            totalHeight += margin.top + child->GetPreferredHeight(childWidth) + margin.bottom;
            first = false;
        }
        return totalHeight;
    }

public:
    void Render(Renderer& renderer) override {
        if (cornerRadius > 0.0f) {
            renderer.FillRoundedRect(m_bounds, background, cornerRadius);
            renderer.DrawRoundedRect(m_bounds, D2D1::ColorF(0x333333), 1.0f, cornerRadius);
        } else {
            renderer.FillRect(m_bounds, background);
        }
        for (auto& child : m_children) {
            child->Render(renderer);
        }
    }
};

class GridPanel : public UIElement {
public:
    int columns = 3;
    float cellSpacing = 8.0f;
    float cornerRadius = 0.0f;
    Thickness padding{8, 8, 8, 8};
    D2D1_COLOR_F background = D2D1::ColorF(0x1A1A1A);
    float rowHeight = 120.0f;

    void Arrange(const D2D1_RECT_F& finalRect) override {
        UIElement::Arrange(finalRect);

        const float x0 = m_bounds.left + padding.left;
        float y = m_bounds.top + padding.top;
        const float contentWidth = (m_bounds.right - m_bounds.left) - padding.left - padding.right;
        const int cols = std::max(1, columns);
        const float cellWidth = (contentWidth - cellSpacing * (cols - 1)) / cols;

        float x = x0;
        int col = 0;
        for (auto& child : m_children) {
            const Thickness& margin = child->Margin();
            const float availableCellWidth = std::max(0.0f, cellWidth - margin.left - margin.right);
            const float childWidth = child->HasFixedWidth() ? child->GetPreferredWidth(availableCellWidth) : availableCellWidth;
            const float childHeight = std::min(
                std::max(0.0f, rowHeight - margin.top - margin.bottom),
                child->GetPreferredHeight(childWidth));
            child->Arrange(D2D1::RectF(
                x + margin.left,
                y + margin.top,
                x + margin.left + childWidth,
                y + margin.top + childHeight));
            col++;
            if (col >= cols) {
                col = 0;
                x = x0;
                y += rowHeight + cellSpacing;
            } else {
                x += cellWidth + cellSpacing;
            }
        }
    }

protected:
    float MeasurePreferredHeight(float width) const override {
        (void)width;
        const int cols = std::max(1, columns);
        const int cellCount = static_cast<int>(m_children.size());
        const int rows = (cellCount + cols - 1) / cols;
        float totalHeight = padding.top + padding.bottom;
        if (rows > 0) {
            totalHeight += rows * rowHeight;
            totalHeight += (rows - 1) * cellSpacing;
        }
        return totalHeight;
    }

public:
    void Render(Renderer& renderer) override {
        if (cornerRadius > 0.0f) {
            renderer.FillRoundedRect(m_bounds, background, cornerRadius);
            renderer.DrawRoundedRect(m_bounds, D2D1::ColorF(0x333333), 1.0f, cornerRadius);
        } else {
            renderer.FillRect(m_bounds, background);
        }
        for (auto& child : m_children) {
            child->Render(renderer);
        }
    }
};

class Label : public UIElement {
public:
    explicit Label(std::wstring text) : m_text(std::move(text)) {}

    void SetText(std::wstring text) { m_text = std::move(text); }

protected:
    float MeasurePreferredWidth(float availableWidth) const override {
        const D2D1_SIZE_F measured = MeasureTextLayout(m_text, m_fontSize, availableWidth > 0.0f ? availableWidth : 4096.0f);
        return std::min(availableWidth, measured.width + 4.0f);
    }

    float MeasurePreferredHeight(float width) const override {
        const D2D1_SIZE_F measured = MeasureTextLayout(m_text, m_fontSize, width > 0.0f ? width : 4096.0f);
        return measured.height + 8.0f;
    }

public:
    void Render(Renderer& renderer) override {
        renderer.DrawTextW(
            m_text.c_str(),
            static_cast<UINT32>(m_text.size()),
            m_bounds,
            m_color,
            m_fontSize
        );
    }

    float m_fontSize = 15.0f;
    D2D1_COLOR_F m_color = D2D1::ColorF(0xEDEDED);

private:
    std::wstring m_text;
};

class Image : public UIElement {
public:
    enum class StretchMode {
        Fill,
        Uniform,
        UniformToFill
    };

    explicit Image(std::wstring source) : m_source(std::move(source)) {}

    float cornerRadius = 0.0f;
    StretchMode stretch = StretchMode::Uniform;

    void SetImageData(std::vector<uint8_t> imageData) {
        m_imageData = std::move(imageData);
    }

protected:
    float MeasurePreferredHeight(float width) const override {
        (void)width;
        return 120.0f;
    }

public:
    void Render(Renderer& renderer) override {
        if (!m_bitmap && !m_imageData.empty()) {
            m_bitmap = renderer.CreateBitmapFromBytes(m_imageData.data(), m_imageData.size());
        }
        if (m_bitmap) {
            D2D1_RECT_F drawRect = m_bounds;
            if (stretch != StretchMode::Fill) {
                const D2D1_SIZE_F sourceSize = m_bitmap->GetSize();
                if (sourceSize.width > 0 && sourceSize.height > 0) {
                    const float targetWidth = m_bounds.right - m_bounds.left;
                    const float targetHeight = m_bounds.bottom - m_bounds.top;
                    const float sourceRatio = sourceSize.width / sourceSize.height;
                    const float targetRatio = targetWidth / targetHeight;

                    if (stretch == StretchMode::Uniform) {
                        if (sourceRatio > targetRatio) {
                            const float scaledHeight = targetWidth / sourceRatio;
                            const float offsetY = (targetHeight - scaledHeight) * 0.5f;
                            drawRect = D2D1::RectF(m_bounds.left, m_bounds.top + offsetY, m_bounds.right, m_bounds.top + offsetY + scaledHeight);
                        } else {
                            const float scaledWidth = targetHeight * sourceRatio;
                            const float offsetX = (targetWidth - scaledWidth) * 0.5f;
                            drawRect = D2D1::RectF(m_bounds.left + offsetX, m_bounds.top, m_bounds.left + offsetX + scaledWidth, m_bounds.bottom);
                        }
                    } else if (stretch == StretchMode::UniformToFill) {
                        if (sourceRatio > targetRatio) {
                            const float scaledWidth = targetHeight * sourceRatio;
                            const float offsetX = (targetWidth - scaledWidth) * 0.5f;
                            drawRect = D2D1::RectF(m_bounds.left + offsetX, m_bounds.top, m_bounds.left + offsetX + scaledWidth, m_bounds.bottom);
                        } else {
                            const float scaledHeight = targetWidth / sourceRatio;
                            const float offsetY = (targetHeight - scaledHeight) * 0.5f;
                            drawRect = D2D1::RectF(m_bounds.left, m_bounds.top + offsetY, m_bounds.right, m_bounds.top + offsetY + scaledHeight);
                        }
                    }
                }
            }

            if (cornerRadius > 0.0f) {
                renderer.PushRoundedClip(m_bounds, cornerRadius);
                renderer.DrawBitmap(m_bitmap.Get(), drawRect);
                renderer.PopLayer();
            } else {
                renderer.DrawBitmap(m_bitmap.Get(), drawRect);
            }
        } else {
            renderer.FillRect(m_bounds, D2D1::ColorF(0x404040));
        }
    }

private:
    std::wstring m_source;
    std::vector<uint8_t> m_imageData;
    Microsoft::WRL::ComPtr<ID2D1Bitmap> m_bitmap;
};

class Button : public UIElement {
public:
    explicit Button(std::wstring text) : m_text(std::move(text)) {}

    void SetOnClick(std::function<void()> onClick) { m_onClick = std::move(onClick); }
    void SetText(std::wstring text) { m_text = std::move(text); }

    float cornerRadius = 4.0f;
    float fontSize = 14.0f;
    D2D1_COLOR_F background = D2D1::ColorF(0x2D2D30);
    D2D1_COLOR_F foreground = D2D1::ColorF(0xFFFFFF);

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
        const D2D1_SIZE_F measured = MeasureTextLayout(m_text, fontSize, availableWidth > 0.0f ? availableWidth : 4096.0f);
        return std::min(availableWidth, measured.width + 24.0f);
    }

    float MeasurePreferredHeight(float width) const override {
        const D2D1_SIZE_F measured = MeasureTextLayout(m_text, fontSize, width > 0.0f ? width : 4096.0f);
        return measured.height + 14.0f;
    }

public:
    void Render(Renderer& renderer) override {
        D2D1_COLOR_F bg = background;
        if (m_pressed && m_hovered) {
            bg = D2D1::ColorF(0x0E639C);
        } else if (m_hovered) {
            bg = D2D1::ColorF(0x3E3E42);
        }

        if (cornerRadius > 0.0f) {
            renderer.FillRoundedRect(m_bounds, bg, cornerRadius);
            renderer.DrawRoundedRect(m_bounds, D2D1::ColorF(0x6A6A6A), 1.0f, cornerRadius);
            if (m_hasFocus) {
                renderer.DrawRoundedRect(m_bounds, D2D1::ColorF(0xFFFFFF), 2.0f, cornerRadius);
            }
        } else {
            renderer.FillRect(m_bounds, bg);
            renderer.DrawRect(m_bounds, D2D1::ColorF(0x6A6A6A), 1.0f);
            if (m_hasFocus) {
                renderer.DrawRect(m_bounds, D2D1::ColorF(0xFFFFFF), 2.0f);
            }
        }

        D2D1_RECT_F textRect = m_bounds;
        textRect.left += 10.0f;
        renderer.DrawTextW(
            m_text.c_str(),
            static_cast<UINT32>(m_text.size()),
            textRect,
            foreground,
            fontSize
        );
    }

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
    bool m_hovered = false;
    bool m_pressed = false;
    std::function<void()> m_onClick;
};

class TextInput : public UIElement {
public:
    explicit TextInput(std::wstring text = L"") : m_text(std::move(text)), m_caretPosition(m_text.size()) {}

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
        const D2D1_RECT_F textRect = D2D1::RectF(m_bounds.left + 8.0f, m_bounds.top + 6.0f, m_bounds.right - 8.0f, m_bounds.bottom - 6.0f);
        if (!HitTest(x, y)) {
            return false;
        }

        const float localX = x - textRect.left;
        size_t newCaret = 0;
        float cursorX = 0.0f;
        for (size_t i = 0; i <= m_text.size(); ++i) {
            const std::wstring prefix = m_text.substr(0, i);
            const D2D1_SIZE_F metrics = MeasureTextLayout(prefix.empty() ? L" " : prefix, fontSize, std::max(1.0f, textRect.right - textRect.left));
            if (localX < metrics.width + 4.0f) {
                newCaret = i;
                break;
            }
            cursorX = metrics.width;
        }
        m_caretPosition = newCaret;
        ClearSelection();
        ResetCaretBlink();
        return true;
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
    void SetBackgroundColor(const D2D1_COLOR_F& color) { background = color; }
    void SetTextColor(const D2D1_COLOR_F& color) { textColor = color; }
    void SetCornerRadius(float radius) { cornerRadius = radius; }

protected:
    float MeasurePreferredWidth(float availableWidth) const override {
        const std::wstring measureText = m_text.empty() ? L" " : m_text;
        const D2D1_SIZE_F measured = MeasureTextLayout(measureText, fontSize, std::max(1.0f, availableWidth - 16.0f));
        return std::min(availableWidth, measured.width + 16.0f);
    }

    float MeasurePreferredHeight(float width) const override {
        const std::wstring measureText = m_text.empty() ? L" " : m_text;
        const D2D1_SIZE_F measured = MeasureTextLayout(measureText, fontSize, std::max(1.0f, width - 16.0f));
        return measured.height + 12.0f;
    }

public:
    void Render(Renderer& renderer) override {
        renderer.FillRoundedRect(m_bounds, background, cornerRadius);
        renderer.DrawRoundedRect(m_bounds, borderColor, 1.0f, cornerRadius);
        if (m_hasFocus) {
            renderer.DrawRoundedRect(m_bounds, focusBorderColor, 2.0f, cornerRadius);
        }

        D2D1_RECT_F textRect = m_bounds;
        textRect.left += 8.0f;
        textRect.top += 6.0f;
        if (HasSelection()) {
            const auto [selStart, selEnd] = GetSelectionRange();
            std::wstring prefix = m_text.substr(0, selStart);
            std::wstring selection = m_text.substr(selStart, selEnd - selStart);
            const D2D1_SIZE_F prefixMetrics = MeasureTextLayout(prefix.empty() ? L" " : prefix, fontSize, std::max(1.0f, textRect.right - textRect.left));
            const D2D1_SIZE_F selectionMetrics = MeasureTextLayout(selection.empty() ? L" " : selection, fontSize, std::max(1.0f, textRect.right - textRect.left));
            D2D1_RECT_F selectionRect = D2D1::RectF(
                textRect.left + prefixMetrics.width,
                textRect.top,
                textRect.left + prefixMetrics.width + selectionMetrics.width,
                textRect.top + selectionMetrics.height
            );
            renderer.FillRect(selectionRect, D2D1::ColorF(0x3A86FF));
        }
        renderer.DrawTextW(m_text.c_str(), static_cast<UINT32>(m_text.size()), textRect, textColor, fontSize);

        if (m_hasFocus) {
            const std::wstring prefix = m_text.substr(0, m_caretPosition);
            const D2D1_SIZE_F caretMetrics = MeasureTextLayout(prefix.empty() ? L" " : prefix, fontSize, std::max(1.0f, textRect.right - textRect.left));
            const float caretX = textRect.left + caretMetrics.width;
            const float caretTop = textRect.top;
            const float caretBottom = textRect.top + MeasureTextLayout(L"|", fontSize, std::max(1.0f, textRect.right - textRect.left)).height;
            if (m_showCaret) {
                renderer.DrawRect(D2D1::RectF(caretX, caretTop, caretX + 1.0f, caretBottom), textColor, 1.0f);
            }
        }
    }

private:
    std::wstring m_text;
    size_t m_caretPosition = 0;
    size_t m_selectionStart = 0;
    size_t m_selectionEnd = 0;
    bool m_showCaret = true;
    DWORD m_lastBlink = 0;
    float fontSize = 14.0f;
    D2D1_COLOR_F background = D2D1::ColorF(0x1F1F1F);
    D2D1_COLOR_F borderColor = D2D1::ColorF(0x6A6A6A);
    D2D1_COLOR_F focusBorderColor = D2D1::ColorF(0xFFFFFF);
    D2D1_COLOR_F textColor = D2D1::ColorF(0xEDEDED);
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
};

class Checkbox : public UIElement {
public:
    explicit Checkbox(std::wstring text = L"") : m_text(std::move(text)) {}

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
        const D2D1_SIZE_F textSize = MeasureTextLayout(m_text.empty() ? L" " : m_text, fontSize, std::max(1.0f, availableWidth - 30.0f));
        return std::min(availableWidth, textSize.width + 30.0f);
    }

    float MeasurePreferredHeight(float width) const override {
        const D2D1_SIZE_F textSize = MeasureTextLayout(m_text.empty() ? L" " : m_text, fontSize, std::max(1.0f, width - 30.0f));
        return textSize.height + 10.0f;
    }

    void Render(Renderer& renderer) override {
        const D2D1_RECT_F box = D2D1::RectF(m_bounds.left, m_bounds.top + 2.0f, m_bounds.left + 18.0f, m_bounds.top + 20.0f);
        renderer.FillRect(box, D2D1::ColorF(0x2D2D30));
        renderer.DrawRect(box, D2D1::ColorF(0x6A6A6A), 1.0f);
        if (m_checked) {
            const D2D1_RECT_F mark = D2D1::RectF(box.left + 4.0f, box.top + 4.0f, box.right - 4.0f, box.bottom - 4.0f);
            renderer.FillRect(mark, D2D1::ColorF(0x2D6CDF));
        }
        if (m_hasFocus) {
            renderer.DrawRect(box, D2D1::ColorF(0xFFFFFF), 2.0f);
        }

        D2D1_RECT_F textRect = m_bounds;
        textRect.left += 26.0f;
        renderer.DrawTextW(m_text.c_str(), static_cast<UINT32>(m_text.size()), textRect, textColor, fontSize);
    }

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
    bool m_pressed = false;
    D2D1_COLOR_F textColor = D2D1::ColorF(0xEDEDED);
    float fontSize = 14.0f;
};

class RadioButton : public UIElement {
public:
    explicit RadioButton(std::wstring text = L"", std::wstring group = L"default")
        : m_text(std::move(text)), m_group(std::move(group)) {
        s_groups[m_group].push_back(this);
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
        const D2D1_SIZE_F textSize = MeasureTextLayout(m_text.empty() ? L" " : m_text, fontSize, std::max(1.0f, availableWidth - 28.0f));
        return std::min(availableWidth, textSize.width + 28.0f);
    }

    float MeasurePreferredHeight(float width) const override {
        const D2D1_SIZE_F textSize = MeasureTextLayout(m_text.empty() ? L" " : m_text, fontSize, std::max(1.0f, width - 28.0f));
        return textSize.height + 10.0f;
    }

    void Render(Renderer& renderer) override {
        const D2D1_RECT_F circle = D2D1::RectF(m_bounds.left, m_bounds.top + 2.0f, m_bounds.left + 18.0f, m_bounds.top + 20.0f);
        renderer.DrawRoundedRect(circle, D2D1::ColorF(0x6A6A6A), 1.0f, 9.0f);
        if (m_checked) {
            const D2D1_RECT_F dot = D2D1::RectF(circle.left + 5.0f, circle.top + 5.0f, circle.right - 5.0f, circle.bottom - 5.0f);
            renderer.FillRoundedRect(dot, D2D1::ColorF(0x2D6CDF), 5.0f);
        }
        if (m_hasFocus) {
            renderer.DrawRoundedRect(circle, D2D1::ColorF(0xFFFFFF), 2.0f, 9.0f);
        }

        D2D1_RECT_F textRect = m_bounds;
        textRect.left += 26.0f;
        renderer.DrawTextW(m_text.c_str(), static_cast<UINT32>(m_text.size()), textRect, textColor, fontSize);
    }

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
    bool m_pressed = false;
    D2D1_COLOR_F textColor = D2D1::ColorF(0xEDEDED);
    float fontSize = 14.0f;
    inline static std::map<std::wstring, std::vector<RadioButton*>> s_groups;
};

class Slider : public UIElement {
public:
    explicit Slider(std::wstring label = L"") : m_label(std::move(label)) {}

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
        UpdateValueFromPoint(x);
        return true;
    }

    bool OnMouseMove(float x, float y) override {
        if (m_dragging) {
            UpdateValueFromPoint(x);
            return true;
        }
        return false;
    }

    bool OnMouseUp(float x, float y) override {
        if (m_dragging) {
            m_dragging = false;
            return true;
        }
        return false;
    }

    bool OnMouseLeave() override {
        if (!m_dragging) {
            return false;
        }
        m_dragging = false;
        return true;
    }

    float MeasurePreferredWidth(float availableWidth) const override {
        return std::min(availableWidth, 220.0f);
    }

    float MeasurePreferredHeight(float width) const override {
        (void)width;
        return 42.0f;
    }

    void Render(Renderer& renderer) override {
        renderer.FillRoundedRect(m_bounds, background, cornerRadius);
        renderer.DrawRoundedRect(m_bounds, borderColor, 1.0f, cornerRadius);
        if (m_hasFocus) {
            renderer.DrawRoundedRect(m_bounds, focusBorderColor, 2.0f, cornerRadius);
        }

        const float trackLeft = m_bounds.left + 14.0f;
        const float trackRight = m_bounds.right - 14.0f;
        const float trackTop = m_bounds.top + (m_bounds.bottom - m_bounds.top) * 0.5f - 4.0f;
        const float trackBottom = trackTop + 8.0f;
        const D2D1_RECT_F trackRect = D2D1::RectF(trackLeft, trackTop, trackRight, trackBottom);
        renderer.FillRect(trackRect, D2D1::ColorF(0x333333));

        const float position = trackLeft + (trackRight - trackLeft) * ((m_value - m_min) / std::max(1.0f, m_max - m_min));
        const D2D1_RECT_F thumbRect = D2D1::RectF(position - 8.0f, trackTop - 6.0f, position + 8.0f, trackBottom + 6.0f);
        renderer.FillRoundedRect(thumbRect, highlightColor, 8.0f);
        renderer.DrawRoundedRect(thumbRect, D2D1::ColorF(0x6A6A6A), 1.0f, 8.0f);

        if (!m_label.empty()) {
            D2D1_RECT_F labelRect = m_bounds;
            labelRect.right = trackLeft - 8.0f;
            renderer.DrawTextW(m_label.c_str(), static_cast<UINT32>(m_label.size()), labelRect, textColor, fontSize);
        }
    }

    void SetText(std::wstring text) { m_label = std::move(text); }
    void SetRange(float minValue, float maxValue) { m_min = minValue; m_max = maxValue; SetValue(m_value); }
    void SetValue(float value) { m_value = std::clamp(value, m_min, m_max); }
    void SetStep(float step) { m_step = std::max(0.001f, step); }

private:
    void UpdateValueFromPoint(float x) {
        const float trackLeft = m_bounds.left + 14.0f;
        const float trackRight = m_bounds.right - 14.0f;
        const float ratio = std::clamp((x - trackLeft) / (trackRight - trackLeft), 0.0f, 1.0f);
        SetValue(m_min + ratio * (m_max - m_min));
    }

    std::wstring m_label;
    float m_min = 0.0f;
    float m_max = 1.0f;
    float m_value = 0.5f;
    float m_step = 0.05f;
    bool m_dragging = false;
    float fontSize = 13.0f;
    D2D1_COLOR_F background = D2D1::ColorF(0x1F1F1F);
    D2D1_COLOR_F borderColor = D2D1::ColorF(0x6A6A6A);
    D2D1_COLOR_F focusBorderColor = D2D1::ColorF(0xFFFFFF);
    D2D1_COLOR_F textColor = D2D1::ColorF(0xEDEDED);
    D2D1_COLOR_F highlightColor = D2D1::ColorF(0x2D6CDF);
    float cornerRadius = 8.0f;
};
