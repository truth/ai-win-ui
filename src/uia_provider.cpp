#include "uia_provider.h"
#include "ui_host.h"
#include <unordered_map>
#include <mutex>

static std::unordered_map<UIElement*, UIAElementProvider*> g_providerMap;
static std::mutex g_providerMapMutex;

Microsoft::WRL::ComPtr<UIAElementProvider> GetOrCreateProviderForElement(UIElement* element, HWND hwnd) {
    if (!element) return nullptr;
    std::lock_guard<std::mutex> lock(g_providerMapMutex);
    auto it = g_providerMap.find(element);
    if (it != g_providerMap.end()) {
        return Microsoft::WRL::ComPtr<UIAElementProvider>(it->second);
    }
    auto provider = Microsoft::WRL::Make<UIAElementProvider>(element, hwnd);
    g_providerMap[element] = provider.Get();
    return provider;
}

static std::wstring Utf8ToUtf16(const std::string& str) {
    if (str.empty()) return std::wstring();
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

// ---------------- UIAElementProvider ----------------

UIAElementProvider::UIAElementProvider(UIElement* element, HWND hwnd)
    : m_element(element), m_hwnd(hwnd) {
}

UIAElementProvider::~UIAElementProvider() {
    std::lock_guard<std::mutex> lock(g_providerMapMutex);
    g_providerMap.erase(m_element);
}

void UIAElementProvider::Detach() {
    m_element = nullptr;
}

IFACEMETHODIMP UIAElementProvider::get_ProviderOptions(ProviderOptions* pRetVal) {
    if (!pRetVal) return E_POINTER;
    *pRetVal = ProviderOptions_ServerSideProvider | ProviderOptions_UseComThreading;
    return S_OK;
}

IFACEMETHODIMP UIAElementProvider::GetPatternProvider(PATTERNID patternId, IUnknown** pRetVal) {
    if (!pRetVal) return E_POINTER;
    *pRetVal = nullptr;
    return S_OK;
}

IFACEMETHODIMP UIAElementProvider::GetPropertyValue(PROPERTYID propertyId, VARIANT* pRetVal) {
    if (!pRetVal) return E_POINTER;
    VariantInit(pRetVal);

    if (!m_element) return UIA_E_ELEMENTNOTAVAILABLE;

    if (propertyId == UIA_ControlTypePropertyId) {
        pRetVal->vt = VT_I4;
        pRetVal->lVal = m_element->GetUiaControlType();
    } else if (propertyId == UIA_NamePropertyId) {
        if (!m_element->UiaName().empty()) {
            pRetVal->vt = VT_BSTR;
            pRetVal->bstrVal = SysAllocString(Utf8ToUtf16(m_element->UiaName()).c_str());
        }
    } else if (propertyId == UIA_HelpTextPropertyId) {
        if (!m_element->UiaHelpText().empty()) {
            pRetVal->vt = VT_BSTR;
            pRetVal->bstrVal = SysAllocString(Utf8ToUtf16(m_element->UiaHelpText()).c_str());
        }
    } else if (propertyId == UIA_AutomationIdPropertyId) {
        if (!m_element->Name().empty()) {
            pRetVal->vt = VT_BSTR;
            pRetVal->bstrVal = SysAllocString(Utf8ToUtf16(m_element->Name()).c_str());
        }
    } else if (propertyId == UIA_IsControlElementPropertyId || propertyId == UIA_IsContentElementPropertyId) {
        pRetVal->vt = VT_BOOL;
        pRetVal->boolVal = VARIANT_TRUE;
    }
    return S_OK;
}

IFACEMETHODIMP UIAElementProvider::get_HostRawElementProvider(IRawElementProviderSimple** pRetVal) {
    if (!pRetVal) return E_POINTER;
    *pRetVal = nullptr;
    return S_OK;
}

IFACEMETHODIMP UIAElementProvider::Navigate(NavigateDirection direction, IRawElementProviderFragment** pRetVal) {
    if (!pRetVal) return E_POINTER;
    *pRetVal = nullptr;
    if (!m_element) return UIA_E_ELEMENTNOTAVAILABLE;

    // We do not have parent pointers in UIElement by default in ai-win-ui.
    // Full navigation requires parent tracking, which might need to be added to UIContext
    // or we only support child traversal from Root. For now, we only implement simple children traversal if requested,
    // or return nullptr to let UIA fall back. Let's return nullptr for now.
    return S_OK;
}

IFACEMETHODIMP UIAElementProvider::GetRuntimeId(SAFEARRAY** pRetVal) {
    if (!pRetVal) return E_POINTER;
    *pRetVal = nullptr;
    if (!m_element) return UIA_E_ELEMENTNOTAVAILABLE;

    int runtimeId[] = { UiaAppendRuntimeId, static_cast<int>(reinterpret_cast<intptr_t>(m_element)) };
    SAFEARRAY* psa = SafeArrayCreateVector(VT_I4, 0, 2);
    if (!psa) return E_OUTOFMEMORY;

    for (LONG i = 0; i < 2; i++) {
        SafeArrayPutElement(psa, &i, &runtimeId[i]);
    }
    *pRetVal = psa;
    return S_OK;
}

IFACEMETHODIMP UIAElementProvider::get_BoundingRectangle(UiaRect* pRetVal) {
    if (!pRetVal) return E_POINTER;
    pRetVal->left = pRetVal->top = pRetVal->width = pRetVal->height = 0;
    if (!m_element) return UIA_E_ELEMENTNOTAVAILABLE;

    Rect bounds = m_element->Bounds();
    POINT pt{ static_cast<LONG>(bounds.left), static_cast<LONG>(bounds.top) };
    ClientToScreen(m_hwnd, &pt);

    pRetVal->left = pt.x;
    pRetVal->top = pt.y;
    pRetVal->width = bounds.Width();
    pRetVal->height = bounds.Height();
    return S_OK;
}

IFACEMETHODIMP UIAElementProvider::GetEmbeddedFragmentRoots(SAFEARRAY** pRetVal) {
    if (!pRetVal) return E_POINTER;
    *pRetVal = nullptr;
    return S_OK;
}

IFACEMETHODIMP UIAElementProvider::SetFocus() {
    return S_OK;
}

IFACEMETHODIMP UIAElementProvider::get_FragmentRoot(IRawElementProviderFragmentRoot** pRetVal) {
    if (!pRetVal) return E_POINTER;
    *pRetVal = nullptr;
    // Should return the HostProvider, but this requires access to the host provider singleton.
    return S_OK;
}

// ---------------- UIAHostProvider ----------------

UIAHostProvider::UIAHostProvider(UiHost* host, HWND hwnd)
    : m_host(host), m_hwnd(hwnd) {
}

UIAHostProvider::~UIAHostProvider() {
}

void UIAHostProvider::Detach() {
    m_host = nullptr;
}

IFACEMETHODIMP UIAHostProvider::get_ProviderOptions(ProviderOptions* pRetVal) {
    if (!pRetVal) return E_POINTER;
    *pRetVal = ProviderOptions_ServerSideProvider | ProviderOptions_UseComThreading;
    return S_OK;
}

IFACEMETHODIMP UIAHostProvider::GetPatternProvider(PATTERNID patternId, IUnknown** pRetVal) {
    if (!pRetVal) return E_POINTER;
    *pRetVal = nullptr;
    return S_OK;
}

IFACEMETHODIMP UIAHostProvider::GetPropertyValue(PROPERTYID propertyId, VARIANT* pRetVal) {
    if (!pRetVal) return E_POINTER;
    VariantInit(pRetVal);
    if (!m_host) return UIA_E_ELEMENTNOTAVAILABLE;

    if (propertyId == UIA_NamePropertyId) {
        pRetVal->vt = VT_BSTR;
        pRetVal->bstrVal = SysAllocString(L"AI WinUI Root");
    } else if (propertyId == UIA_ControlTypePropertyId) {
        pRetVal->vt = VT_I4;
        pRetVal->lVal = UIA::WindowControlType;
    } else if (propertyId == UIA_IsControlElementPropertyId || propertyId == UIA_IsContentElementPropertyId) {
        pRetVal->vt = VT_BOOL;
        pRetVal->boolVal = VARIANT_TRUE;
    }
    return S_OK;
}

IFACEMETHODIMP UIAHostProvider::get_HostRawElementProvider(IRawElementProviderSimple** pRetVal) {
    if (!pRetVal) return E_POINTER;
    return UiaHostProviderFromHwnd(m_hwnd, pRetVal);
}

IFACEMETHODIMP UIAHostProvider::Navigate(NavigateDirection direction, IRawElementProviderFragment** pRetVal) {
    if (!pRetVal) return E_POINTER;
    *pRetVal = nullptr;
    if (!m_host) return UIA_E_ELEMENTNOTAVAILABLE;

    if (direction == NavigateDirection_FirstChild || direction == NavigateDirection_LastChild) {
        // Return the first root element from m_host's elements
        // Wait, m_host->GetRootElements() or similar?
    }
    return S_OK;
}

IFACEMETHODIMP UIAHostProvider::GetRuntimeId(SAFEARRAY** pRetVal) {
    if (!pRetVal) return E_POINTER;
    *pRetVal = nullptr;
    return S_OK;
}

IFACEMETHODIMP UIAHostProvider::get_BoundingRectangle(UiaRect* pRetVal) {
    if (!pRetVal) return E_POINTER;
    RECT rect;
    GetWindowRect(m_hwnd, &rect);
    pRetVal->left = rect.left;
    pRetVal->top = rect.top;
    pRetVal->width = rect.right - rect.left;
    pRetVal->height = rect.bottom - rect.top;
    return S_OK;
}

IFACEMETHODIMP UIAHostProvider::GetEmbeddedFragmentRoots(SAFEARRAY** pRetVal) {
    if (!pRetVal) return E_POINTER;
    *pRetVal = nullptr;
    return S_OK;
}

IFACEMETHODIMP UIAHostProvider::SetFocus() {
    return S_OK;
}

IFACEMETHODIMP UIAHostProvider::get_FragmentRoot(IRawElementProviderFragmentRoot** pRetVal) {
    if (!pRetVal) return E_POINTER;
    this->QueryInterface(IID_PPV_ARGS(pRetVal));
    return S_OK;
}

IFACEMETHODIMP UIAHostProvider::ElementProviderFromPoint(double x, double y, IRawElementProviderFragment** pRetVal) {
    if (!pRetVal) return E_POINTER;
    *pRetVal = nullptr;
    // We can do hit testing later.
    return S_OK;
}

IFACEMETHODIMP UIAHostProvider::GetFocus(IRawElementProviderFragment** pRetVal) {
    if (!pRetVal) return E_POINTER;
    *pRetVal = nullptr;
    return S_OK;
}
