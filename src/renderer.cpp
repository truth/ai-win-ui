#include "renderer.h"

bool Renderer::Initialize(HWND hwnd) {
    m_hwnd = hwnd;

    if (FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, m_d2dFactory.ReleaseAndGetAddressOf()))) {
        return false;
    }

    if (FAILED(DWriteCreateFactory(
            DWRITE_FACTORY_TYPE_SHARED,
            __uuidof(IDWriteFactory),
            reinterpret_cast<IUnknown**>(m_dwriteFactory.ReleaseAndGetAddressOf())))) {
        return false;
    }

    if (FAILED(m_dwriteFactory->CreateTextFormat(
            L"Segoe UI",
            nullptr,
            DWRITE_FONT_WEIGHT_NORMAL,
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            16.0f,
            L"",
            m_defaultTextFormat.ReleaseAndGetAddressOf()))) {
        return false;
    }

    m_defaultTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
    m_defaultTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);

    return EnsureRenderTarget();
}

bool Renderer::EnsureRenderTarget() {
    if (m_renderTarget) {
        return true;
    }
    if (!m_d2dFactory || !m_hwnd) {
        return false;
    }

    RECT rc{};
    GetClientRect(m_hwnd, &rc);
    const auto size = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);

    if (FAILED(m_d2dFactory->CreateHwndRenderTarget(
            D2D1::RenderTargetProperties(),
            D2D1::HwndRenderTargetProperties(m_hwnd, size),
            m_renderTarget.ReleaseAndGetAddressOf()))) {
        return false;
    }

    if (FAILED(m_renderTarget->CreateSolidColorBrush(
            D2D1::ColorF(D2D1::ColorF::White),
            m_solidBrush.ReleaseAndGetAddressOf()))) {
        return false;
    }

    return true;
}

void Renderer::Resize(UINT width, UINT height) {
    if (m_renderTarget) {
        m_renderTarget->Resize(D2D1::SizeU(width, height));
    }
}

void Renderer::BeginFrame(const D2D1_COLOR_F& clearColor) {
    if (!EnsureRenderTarget()) {
        return;
    }

    m_renderTarget->BeginDraw();
    m_renderTarget->Clear(clearColor);
}

void Renderer::EndFrame() {
    if (!m_renderTarget) {
        return;
    }

    const HRESULT hr = m_renderTarget->EndDraw();
    if (hr == D2DERR_RECREATE_TARGET) {
        m_solidBrush.Reset();
        m_renderTarget.Reset();
    }
}

void Renderer::FillRect(const D2D1_RECT_F& rect, const D2D1_COLOR_F& color) {
    if (!m_renderTarget || !m_solidBrush) {
        return;
    }

    m_solidBrush->SetColor(color);
    m_renderTarget->FillRectangle(rect, m_solidBrush.Get());
}

void Renderer::DrawRect(const D2D1_RECT_F& rect, const D2D1_COLOR_F& color, float strokeWidth) {
    if (!m_renderTarget || !m_solidBrush) {
        return;
    }

    m_solidBrush->SetColor(color);
    m_renderTarget->DrawRectangle(rect, m_solidBrush.Get(), strokeWidth);
}

void Renderer::DrawTextW(const wchar_t* text, UINT32 len, const D2D1_RECT_F& rect, const D2D1_COLOR_F& color, float fontSize) {
    if (!m_renderTarget || !m_solidBrush || !m_dwriteFactory) {
        return;
    }

    Microsoft::WRL::ComPtr<IDWriteTextFormat> format;
    if (FAILED(m_dwriteFactory->CreateTextFormat(
            L"Segoe UI",
            nullptr,
            DWRITE_FONT_WEIGHT_NORMAL,
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            fontSize,
            L"",
            format.ReleaseAndGetAddressOf()))) {
        return;
    }

    format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
    format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);

    m_solidBrush->SetColor(color);
    m_renderTarget->DrawTextW(text, len, format.Get(), rect, m_solidBrush.Get());
}
