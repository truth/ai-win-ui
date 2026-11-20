#pragma once

#include <Windows.h>
#include <memory>
#include <string>

class UIElement;
struct UIContext;
class Theme;
class StyleCatalog;
class IResourceProvider;

namespace ai_win_ui {

// Pure C++ host surface exposed by UiHost to the optional ai_win_ui_react
// library so ai_win_ui_lib keeps zero QuickJS dependencies.
struct IReactHostSurface {
    virtual ~IReactHostSurface() = default;

    virtual HWND GetHwnd() const = 0;
    virtual UIContext& Context() = 0;
    virtual Theme* GetTheme() const = 0;
    virtual StyleCatalog* GetStyleCatalog() const = 0;
    virtual IResourceProvider* GetResourceProvider() const = 0;

    virtual void SetRootElement(std::unique_ptr<UIElement> root) = 0;
    virtual UIElement* RootElement() const = 0;
    virtual void NotifySubtreeRemoving(UIElement* subtree) = 0;

    virtual void BeginBatchUpdate() = 0;
    virtual void EndBatchUpdate(bool requestLayout) = 0;

    virtual void SetWindowTitle(const std::wstring& title) = 0;
    virtual void RequestOpenDemo(
        const std::wstring& layoutPath,
        const std::wstring& chrome,
        const std::wstring& size) = 0;
    virtual void NotifyFirstReactCommitReady() = 0;
};

struct IReactBridge {
    virtual ~IReactBridge() = default;

    virtual void OnCommitMessage() = 0;
    virtual void OnHostDestroyed() = 0;
    virtual bool ReloadBundle() = 0;
    virtual void TickHotReload() = 0;
};

using ReactBridgeFactory = std::unique_ptr<IReactBridge> (*)(
    IReactHostSurface& surface,
    const std::string& bundlePath,
    bool watchMode);

void RegisterReactBridgeFactory(ReactBridgeFactory factory);
ReactBridgeFactory GetReactBridgeFactory();

} // namespace ai_win_ui

