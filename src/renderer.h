#pragma once

#include "graphics_types.h"

#include <Windows.h>
#include <cstddef>
#include <cstdint>
#include <memory>

class IRenderer {
public:
    virtual ~IRenderer() = default;
    virtual bool Initialize(HWND hwnd) = 0;
    virtual void Resize(UINT width, UINT height) = 0;
    virtual void BeginFrame(const Color& clearColor) = 0;
    virtual void EndFrame() = 0;

    virtual void FillRect(const Rect& rect, const Color& color) = 0;
    virtual void FillRoundedRect(const Rect& rect, const Color& color, float radius) = 0;
    virtual void DrawRect(const Rect& rect, const Color& color, float strokeWidth = 1.0f) = 0;
    virtual void DrawRoundedRect(const Rect& rect, const Color& color, float strokeWidth, float radius) = 0;
    virtual void PushRoundedClip(const Rect& rect, float radius) = 0;
    virtual void PopLayer() = 0;
    virtual void DrawTextW(const wchar_t* text, UINT32 len, const Rect& rect, const Color& color, float fontSize = 16.0f) = 0;
    virtual Size GetBitmapSize(BitmapHandle bitmap) = 0;
    virtual BitmapHandle CreateBitmapFromBytes(const uint8_t* data, size_t size) = 0;
    virtual void DrawBitmap(BitmapHandle bitmap, const Rect& rect) = 0;
};

std::unique_ptr<IRenderer> CreateRenderer();
