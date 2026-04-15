#pragma once

#include "renderer.h"
#include <wrl/client.h>

#include <algorithm>
#include <functional>
#include <memory>
#include <string>
#include <vector>

struct Thickness {
    float left = 0;
    float top = 0;
    float right = 0;
    float bottom = 0;
};

class UIElement {
public:
    virtual ~UIElement() = default;

    void SetBounds(const D2D1_RECT_F& rect) { m_bounds = rect; }
    D2D1_RECT_F Bounds() const { return m_bounds; }

    virtual void Measure(float availableWidth, float availableHeight) {
        m_desiredSize.width = availableWidth;
        m_desiredSize.height = availableHeight;
    }
    virtual void Arrange(const D2D1_RECT_F& finalRect) { m_bounds = finalRect; }
    virtual void Render(Renderer& renderer) {
        for (auto& child : m_children) {
            child->Render(renderer);
        }
    }

    virtual bool OnMouseMove(float x, float y) {
        for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
            if ((*it)->HitTest(x, y) && (*it)->OnMouseMove(x, y)) {
                return true;
            }
        }
        return false;
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
        for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
            if ((*it)->HitTest(x, y) && (*it)->OnMouseUp(x, y)) {
                return true;
            }
        }
        return false;
    }

    bool HitTest(float x, float y) const {
        return x >= m_bounds.left && x <= m_bounds.right && y >= m_bounds.top && y <= m_bounds.bottom;
    }

    void AddChild(std::unique_ptr<UIElement> child) { m_children.emplace_back(std::move(child)); }
    std::vector<std::unique_ptr<UIElement>>& Children() { return m_children; }

protected:
    D2D1_RECT_F m_bounds = D2D1::RectF();
    D2D1_SIZE_F m_desiredSize = D2D1::SizeF();
    std::vector<std::unique_ptr<UIElement>> m_children;
};

class Panel : public UIElement {
public:
    Thickness padding{8, 8, 8, 8};
    float spacing = 6.0f;
    D2D1_COLOR_F background = D2D1::ColorF(0x1E1E1E);

    void Arrange(const D2D1_RECT_F& finalRect) override {
        UIElement::Arrange(finalRect);

        const float x = m_bounds.left + padding.left;
        float y = m_bounds.top + padding.top;
        const float maxWidth = (m_bounds.right - m_bounds.left) - padding.left - padding.right;

        for (auto& child : m_children) {
            const float h = 36.0f;
            child->Arrange(D2D1::RectF(x, y, x + maxWidth, y + h));
            y += h + spacing;
        }
    }

    void Render(Renderer& renderer) override {
        renderer.FillRect(m_bounds, background);
        for (auto& child : m_children) {
            child->Render(renderer);
        }
    }
};

class Label : public UIElement {
public:
    explicit Label(std::wstring text) : m_text(std::move(text)) {}

    void SetText(std::wstring text) { m_text = std::move(text); }

    void Render(Renderer& renderer) override {
        renderer.DrawTextW(
            m_text.c_str(),
            static_cast<UINT32>(m_text.size()),
            m_bounds,
            D2D1::ColorF(0xEDEDED),
            15.0f
        );
    }

private:
    std::wstring m_text;
};

class Image : public UIElement {
public:
    explicit Image(std::wstring source) : m_source(std::move(source)) {}

    void SetImageData(std::vector<uint8_t> imageData) {
        m_imageData = std::move(imageData);
    }

    void Render(Renderer& renderer) override {
        if (!m_bitmap && !m_imageData.empty()) {
            m_bitmap = renderer.CreateBitmapFromBytes(m_imageData.data(), m_imageData.size());
        }
        if (m_bitmap) {
            renderer.DrawBitmap(m_bitmap.Get(), m_bounds);
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

    void Render(Renderer& renderer) override {
        D2D1_COLOR_F bg = D2D1::ColorF(0x2D2D30);
        if (m_pressed) {
            bg = D2D1::ColorF(0x0E639C);
        } else if (m_hovered) {
            bg = D2D1::ColorF(0x3E3E42);
        }

        renderer.FillRect(m_bounds, bg);
        renderer.DrawRect(m_bounds, D2D1::ColorF(0x6A6A6A), 1.0f);

        D2D1_RECT_F textRect = m_bounds;
        textRect.left += 10.0f;
        renderer.DrawTextW(
            m_text.c_str(),
            static_cast<UINT32>(m_text.size()),
            textRect,
            D2D1::ColorF(0xFFFFFF),
            14.0f
        );
    }

    bool OnMouseMove(float x, float y) override {
        m_hovered = HitTest(x, y);
        return m_hovered;
    }

    bool OnMouseDown(float x, float y) override {
        m_pressed = HitTest(x, y);
        return m_pressed;
    }

    bool OnMouseUp(float x, float y) override {
        const bool wasPressed = m_pressed;
        m_pressed = false;
        if (wasPressed && HitTest(x, y) && m_onClick) {
            m_onClick();
            return true;
        }
        return false;
    }

private:
    std::wstring m_text;
    bool m_hovered = false;
    bool m_pressed = false;
    std::function<void()> m_onClick;
};
