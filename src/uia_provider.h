#pragma once

#include <Windows.h>
#include <wrl/client.h>
#include <wrl/implements.h>
#include <uiautomation.h>

#include "ui_element.h"
#include <string>
#include <vector>

class UiHost;

// A simple element provider representing a UIElement in the UIA tree.
class UIAElementProvider : public Microsoft::WRL::RuntimeClass<
    Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>,
    IRawElementProviderSimple,
    IRawElementProviderFragment>
{
public:
    UIAElementProvider(UIElement* element, HWND hwnd);
    virtual ~UIAElementProvider();

    // Invalidate the pointer when UIElement is destroyed.
    void Detach();

    // IRawElementProviderSimple
    IFACEMETHODIMP get_ProviderOptions(ProviderOptions* pRetVal) override;
    IFACEMETHODIMP GetPatternProvider(PATTERNID patternId, IUnknown** pRetVal) override;
    IFACEMETHODIMP GetPropertyValue(PROPERTYID propertyId, VARIANT* pRetVal) override;
    IFACEMETHODIMP get_HostRawElementProvider(IRawElementProviderSimple** pRetVal) override;

    // IRawElementProviderFragment
    IFACEMETHODIMP Navigate(NavigateDirection direction, IRawElementProviderFragment** pRetVal) override;
    IFACEMETHODIMP GetRuntimeId(SAFEARRAY** pRetVal) override;
    IFACEMETHODIMP get_BoundingRectangle(UiaRect* pRetVal) override;
    IFACEMETHODIMP GetEmbeddedFragmentRoots(SAFEARRAY** pRetVal) override;
    IFACEMETHODIMP SetFocus() override;
    IFACEMETHODIMP get_FragmentRoot(IRawElementProviderFragmentRoot** pRetVal) override;

protected:
    UIElement* m_element;
    HWND m_hwnd;
};

// Root provider representing the application window.
class UIAHostProvider : public Microsoft::WRL::RuntimeClass<
    Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>,
    IRawElementProviderSimple,
    IRawElementProviderFragment,
    IRawElementProviderFragmentRoot>
{
public:
    UIAHostProvider(UiHost* host, HWND hwnd);
    virtual ~UIAHostProvider();

    void Detach();

    // IRawElementProviderSimple
    IFACEMETHODIMP get_ProviderOptions(ProviderOptions* pRetVal) override;
    IFACEMETHODIMP GetPatternProvider(PATTERNID patternId, IUnknown** pRetVal) override;
    IFACEMETHODIMP GetPropertyValue(PROPERTYID propertyId, VARIANT* pRetVal) override;
    IFACEMETHODIMP get_HostRawElementProvider(IRawElementProviderSimple** pRetVal) override;

    // IRawElementProviderFragment
    IFACEMETHODIMP Navigate(NavigateDirection direction, IRawElementProviderFragment** pRetVal) override;
    IFACEMETHODIMP GetRuntimeId(SAFEARRAY** pRetVal) override;
    IFACEMETHODIMP get_BoundingRectangle(UiaRect* pRetVal) override;
    IFACEMETHODIMP GetEmbeddedFragmentRoots(SAFEARRAY** pRetVal) override;
    IFACEMETHODIMP SetFocus() override;
    IFACEMETHODIMP get_FragmentRoot(IRawElementProviderFragmentRoot** pRetVal) override;

    // IRawElementProviderFragmentRoot
    IFACEMETHODIMP ElementProviderFromPoint(double x, double y, IRawElementProviderFragment** pRetVal) override;
    IFACEMETHODIMP GetFocus(IRawElementProviderFragment** pRetVal) override;

protected:
    UiHost* m_host;
    HWND m_hwnd;
};

// Registry to get or create UIA providers for UIElements to ensure singletons per element
Microsoft::WRL::ComPtr<UIAElementProvider> GetOrCreateProviderForElement(UIElement* element, HWND hwnd);
