#pragma once

#include <windows.h>
#include <unknwn.h>

namespace Ultimakey {

inline constexpr IID CLSID_CUIAutomation       = {0xff48dba4, 0x60ef, 0x4201, {0xaa, 0x87, 0x54, 0x10, 0x3e, 0xef, 0x59, 0x4e}};
inline constexpr IID IID_IUIAutomation         = {0x30cbe57d, 0xd9d0, 0x452a, {0xab, 0x13, 0x7a, 0xc5, 0xac, 0x48, 0x25, 0xee}};
inline constexpr IID IID_IUIAutomationElement  = {0xd22108aa, 0x8ac5, 0x49a5, {0x83, 0x7b, 0x37, 0xbb, 0xb3, 0xd7, 0x59, 0x1e}};
inline constexpr IID IID_IUIAutomationTextPattern = {0x32eba289, 0x3583, 0x42c9, {0x9c, 0x59, 0x3b, 0x6d, 0x9a, 0x1e, 0x9b, 0x6a}};

inline constexpr int UIA_TextPatternId = 10014;

struct IUIAutomationTextRange : public IUnknown {
    virtual HRESULT STDMETHODCALLTYPE Clone(void**) = 0; // 3
    virtual HRESULT STDMETHODCALLTYPE Compare(void*, BOOL*) = 0; // 4
    virtual HRESULT STDMETHODCALLTYPE CompareEndpoints(int, void*, int, int*) = 0; // 5
    virtual HRESULT STDMETHODCALLTYPE ExpandToEnclosingUnit(int) = 0; // 6
    virtual HRESULT STDMETHODCALLTYPE FindAttribute(int, VARIANT, BOOL, void**) = 0; // 7
    virtual HRESULT STDMETHODCALLTYPE FindText(BSTR, BOOL, BOOL, void**) = 0; // 8
    virtual HRESULT STDMETHODCALLTYPE GetAttributeValue(int, VARIANT*) = 0; // 9
    virtual HRESULT STDMETHODCALLTYPE GetBoundingRectangles(SAFEARRAY**) = 0; // 10
    virtual HRESULT STDMETHODCALLTYPE GetEnclosingElement(void**) = 0; // 11
    virtual HRESULT STDMETHODCALLTYPE GetText(int maxLength, BSTR* text) = 0; // 12
};

struct IUIAutomationTextRangeArray : public IUnknown {
    virtual HRESULT STDMETHODCALLTYPE get_Length(int* length) = 0; // 3
    virtual HRESULT STDMETHODCALLTYPE GetElement(int index, IUIAutomationTextRange** element) = 0; // 4
};

struct IUIAutomationTextPattern : public IUnknown {
    virtual HRESULT STDMETHODCALLTYPE RangeFromPoint(POINT, void**) = 0; // 3
    virtual HRESULT STDMETHODCALLTYPE RangeFromChild(void*, void**) = 0; // 4
    virtual HRESULT STDMETHODCALLTYPE GetSelection(IUIAutomationTextRangeArray** ranges) = 0; // 5
};

struct IUIAutomationElement : public IUnknown {
    virtual HRESULT STDMETHODCALLTYPE SetFocus() = 0; // 3
    virtual HRESULT STDMETHODCALLTYPE GetRuntimeId(void*) = 0; // 4
    virtual HRESULT STDMETHODCALLTYPE FindFirst(int, void*, void*) = 0; // 5
    virtual HRESULT STDMETHODCALLTYPE FindAll(int, void*, void*) = 0; // 6
    virtual HRESULT STDMETHODCALLTYPE FindFirstBuildCache(int, void*, void*, void*) = 0; // 7
    virtual HRESULT STDMETHODCALLTYPE FindAllBuildCache(int, void*, void*, void*) = 0; // 8
    virtual HRESULT STDMETHODCALLTYPE BuildUpdatedCache(void*, void*) = 0; // 9
    virtual HRESULT STDMETHODCALLTYPE GetCurrentPropertyValue(int, void*) = 0; // 10
    virtual HRESULT STDMETHODCALLTYPE GetCurrentPropertyValueEx(int, BOOL, void*) = 0; // 11
    virtual HRESULT STDMETHODCALLTYPE GetCachedPropertyValue(int, void*) = 0; // 12
    virtual HRESULT STDMETHODCALLTYPE GetCachedPropertyValueEx(int, BOOL, void*) = 0; // 13
    virtual HRESULT STDMETHODCALLTYPE GetCurrentPatternAs(int patternId, REFIID riid, void** patternObject) = 0; // 14
    virtual HRESULT STDMETHODCALLTYPE GetCachedPatternAs(int, REFIID, void**) = 0; // 15
    virtual HRESULT STDMETHODCALLTYPE GetCurrentPattern(int, void**) = 0; // 16
    virtual HRESULT STDMETHODCALLTYPE GetCachedPattern(int, void**) = 0; // 17
    virtual HRESULT STDMETHODCALLTYPE GetCachedParent(void**) = 0; // 18
    virtual HRESULT STDMETHODCALLTYPE GetCachedChildren(void**) = 0; // 19
    virtual HRESULT STDMETHODCALLTYPE get_CurrentProcessId(void*) = 0; // 20
    virtual HRESULT STDMETHODCALLTYPE get_CurrentControlType(void*) = 0; // 21
    virtual HRESULT STDMETHODCALLTYPE get_CurrentLocalizedControlType(void*) = 0; // 22
    virtual HRESULT STDMETHODCALLTYPE get_CurrentName(void*) = 0; // 23
    virtual HRESULT STDMETHODCALLTYPE get_CurrentAcceleratorKey(void*) = 0; // 24
    virtual HRESULT STDMETHODCALLTYPE get_CurrentAccessKey(void*) = 0; // 25
    virtual HRESULT STDMETHODCALLTYPE get_CurrentHasKeyboardFocus(void*) = 0; // 26
    virtual HRESULT STDMETHODCALLTYPE get_CurrentIsKeyboardFocusable(void*) = 0; // 27
    virtual HRESULT STDMETHODCALLTYPE get_CurrentIsEnabled(void*) = 0; // 28
    virtual HRESULT STDMETHODCALLTYPE get_CurrentAutomationId(void*) = 0; // 29
    virtual HRESULT STDMETHODCALLTYPE get_CurrentClassName(void*) = 0; // 30
    virtual HRESULT STDMETHODCALLTYPE get_CurrentHelpText(void*) = 0; // 31
    virtual HRESULT STDMETHODCALLTYPE get_CurrentCulture(void*) = 0; // 32
    virtual HRESULT STDMETHODCALLTYPE get_CurrentIsControlElement(void*) = 0; // 33
    virtual HRESULT STDMETHODCALLTYPE get_CurrentIsContentElement(void*) = 0; // 34
    virtual HRESULT STDMETHODCALLTYPE get_CurrentIsPassword(BOOL* retVal) = 0; // 35
};

struct IUIAutomation : public IUnknown {
    virtual HRESULT STDMETHODCALLTYPE CompareElements(void*, void*, BOOL*) = 0; // 3
    virtual HRESULT STDMETHODCALLTYPE CompareRuntimeIds(void*, void*, BOOL*) = 0; // 4
    virtual HRESULT STDMETHODCALLTYPE GetRootElement(void**) = 0; // 5
    virtual HRESULT STDMETHODCALLTYPE ElementFromHandle(HWND, void**) = 0; // 6
    virtual HRESULT STDMETHODCALLTYPE ElementFromPoint(POINT, void**) = 0; // 7
    virtual HRESULT STDMETHODCALLTYPE GetFocusedElement(IUIAutomationElement** element) = 0; // 8
};

} // namespace Ultimakey
