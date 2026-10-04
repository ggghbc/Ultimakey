#include "selection.hpp"
#include <future>
#include <chrono>

namespace Ultimakey {

std::wstring SelectionText::TryGet(int timeout_ms) {
    auto future = std::async(std::launch::async, []() -> std::wstring {
        std::wstring result;
        HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        IUIAutomation* uia = nullptr;
        if (SUCCEEDED(CoCreateInstance(CLSID_CUIAutomation, nullptr, CLSCTX_INPROC_SERVER,
                                       IID_IUIAutomation, reinterpret_cast<void**>(&uia))) && uia) {
            IUIAutomationElement* element = nullptr;
            if (SUCCEEDED(uia->GetFocusedElement(&element)) && element) {
                IUIAutomationTextPattern* text_pattern = nullptr;
                if (SUCCEEDED(element->GetCurrentPatternAs(UIA_TextPatternId, IID_IUIAutomationTextPattern,
                                                           reinterpret_cast<void**>(&text_pattern))) && text_pattern) {
                    IUIAutomationTextRangeArray* ranges = nullptr;
                    if (SUCCEEDED(text_pattern->GetSelection(&ranges)) && ranges) {
                        int count = 0;
                        ranges->get_Length(&count);
                        if (count == 1) {
                            IUIAutomationTextRange* range = nullptr;
                            if (SUCCEEDED(ranges->GetElement(0, &range)) && range) {
                                BSTR text = nullptr;
                                if (SUCCEEDED(range->GetText(2000, &text)) && text) {
                                    result = text;
                                    SysFreeString(text);
                                }
                                range->Release();
                            }
                        }
                        ranges->Release();
                    }
                    text_pattern->Release();
                }
                element->Release();
            }
            uia->Release();
        }
        if (SUCCEEDED(hr)) {
            CoUninitialize();
        }
        return result;
    });

    if (future.wait_for(std::chrono::milliseconds(timeout_ms)) == std::future_status::ready) {
        return future.get();
    }
    return L"";
}

} // namespace Ultimakey
