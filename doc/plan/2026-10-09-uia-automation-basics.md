## Goal Description
Implement basic UI Automation (UIA) support for `ai-win-ui` so that screen readers (like Windows Narrator) and accessibility tools (like Accessibility Insights) can recognize and interact with the UI. This involves assigning UIA control types to built-in components, parsing `uiaName` and `uiaHelpText` from layouts, and exposing the UI tree to Windows via the `WM_GETOBJECT` message.

> [!NOTE]
> `ai-win-ui` is a windowless UI framework. To expose its controls to Windows Accessibility, we must provide a COM object tree that implements `IRawElementProviderSimple`, `IRawElementProviderFragment`, and `IRawElementProviderFragmentRoot`.

## User Review Required
> [!IMPORTANT]
> **COM Lifetime Management**: UIA providers are COM objects that can outlive the UI tree if a screen reader holds a reference to them. We will implement a weak back-pointer mechanism where `UIElement` nullifies its associated provider pointer upon destruction, preventing UIA from accessing dangling memory.

> [!WARNING]
> **New Dependency**: We will need to link against `uiautomationcore.lib` and `#include <uiautomation.h>`.

## Open Questions
> [!NOTE]
> 1. Are you okay with using `Microsoft::WRL::RuntimeClass` to implement the COM interfaces (`IRawElementProviderFragment`, etc.)? It's lightweight and already included via `<wrl/client.h>`.
> 2. Should we add a `uiaRole` property to layout XML to let users override the default `ControlType` (e.g., `UIA_ButtonControlTypeId`), or is setting it programmatically per component class enough for now?

---

## Proposed Changes

### Build System
#### [MODIFY] `CMakeLists.txt`
Add `uiautomationcore` to the `target_link_libraries` for the `ai_win_ui_lib` target.

```cmake
target_link_libraries(ai_win_ui_lib PRIVATE 
    d2d1 dwrite windowscodecs
    uiautomationcore # [NEW] Added for UI Automation
)
```

---

### Core UI Framework
#### [MODIFY] `src/ui_element.h`
Add fields to store UIA metadata.
```cpp
#include <uiautomation.h>

class UIElement {
public:
    // ...
    void SetUiaName(std::string name) { m_uiaName = std::move(name); }
    const std::string& UiaName() const { return m_uiaName; }

    void SetUiaHelpText(std::string text) { m_uiaHelpText = std::move(text); }
    const std::string& UiaHelpText() const { return m_uiaHelpText; }

    virtual long GetUiaControlType() const { return UIA_CustomControlTypeId; }

protected:
    std::string m_uiaName;
    std::string m_uiaHelpText;
};
```

#### [MODIFY] `src/ui_controls.h` (and similar control headers)
Override `GetUiaControlType` for built-in controls.
```cpp
class Button : public Panel {
public:
    long GetUiaControlType() const override { return UIA_ButtonControlTypeId; }
};

class Label : public UIElement {
public:
    long GetUiaControlType() const override { return UIA_TextControlTypeId; }
};
```

---

### Layout Parser
#### [MODIFY] `src/layout_parser_controls.cpp`
Parse `uiaName` and `uiaHelpText` from XML/JSON attributes for all elements.
```cpp
if (auto name = xml.attribute("uiaName")) {
    element->SetUiaName(name.value());
}
if (auto help = xml.attribute("uiaHelpText")) {
    element->SetUiaHelpText(help.value());
}
```

---

### UI Automation Providers
#### [NEW] `src/uia_provider.h`
#### [NEW] `src/uia_provider.cpp`
Implement the COM providers using WRL.
```cpp
class UIAElementProvider : public Microsoft::WRL::RuntimeClass<
    Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>,
    IRawElementProviderSimple,
    IRawElementProviderFragment> 
{
    // Implementation of GetPropertyValue, Navigate, etc.
    // Holds a weak reference to UIElement* (or UIElement notifies it on destruction).
};

class UIAHostProvider : public Microsoft::WRL::RuntimeClass<
    Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>,
    IRawElementProviderSimple,
    IRawElementProviderFragment,
    IRawElementProviderFragmentRoot> 
{
    // Represents the HWND root, implements ElementProviderFromPoint, GetFocus, etc.
};
```

---

### Window Host Integration
#### [MODIFY] `src/ui_host.cpp`
Handle the `WM_GETOBJECT` message to bootstrap the UIA tree.
```cpp
#include <uiautomation.h>
#include <uiautomationcoreapi.h>

// Inside HandleMessage:
case WM_GETOBJECT: {
    if (static_cast<long>(lParam) == static_cast<long>(UiaRootObjectId)) {
        // Return the UIAHostProvider COM object
        auto provider = GetOrCreateHostProvider();
        return UiaReturnRawElementProvider(hwnd, wParam, lParam, provider.Get());
    }
    break;
}
```

## Verification Plan

### Automated Tests
Run local CI to ensure the COM additions and `WM_GETOBJECT` handling do not regress existing functionality:
```powershell
.\scripts\run_ci_local.ps1 -SkipBuild
```

### Manual Verification
1. Launch the application (`build/Debug/ai_win_ui.exe`).
2. Open **Accessibility Insights for Windows** or **Windows Narrator**.
3. Hover over elements in the UI and confirm that they are detected, displaying their correct control type (e.g., Button, Text) and any `uiaName` assigned to them in the layout XML.
4. Close the application while Accessibility Insights is inspecting it, ensuring there are no COM-related crashes (validating the weak reference/lifetime mechanism).
