#include "secure_input.hpp"
#include <thread>
#include <chrono>

namespace Ultimakey {

SecureInput& SecureInput::Instance() {
    static SecureInput instance;
    return instance;
}

SecureInput::SecureInput() = default;
SecureInput::~SecureInput() = default;

bool SecureInput::CachedIsPassword() const noexcept {
    HWND fg = GetForegroundWindow();
    if (fg == cached_hwnd_ && (NowMilliseconds() - cached_at_ms_ < 1500)) {
        return cached_value_;
    }
    return false;
}

void SecureInput::KickAsync() {
    bool expected = false;
    if (!in_flight_.compare_exchange_strong(expected, true)) {
        return;
    }

    std::thread([this]() {
        QueryInternal();
        in_flight_.store(false);
    }).detach();
}

bool SecureInput::IsPasswordFocused(int timeout_ms) {
    HWND fg = GetForegroundWindow();
    if (fg == cached_hwnd_ && (NowMilliseconds() - cached_at_ms_ < 1500)) {
        return cached_value_;
    }

    // Trigger async query and wait briefly
    KickAsync();
    int waited = 0;
    while (in_flight_.load() && waited < timeout_ms) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        waited += 10;
    }
    return cached_value_;
}

void SecureInput::QueryInternal() {
    HWND fg = GetForegroundWindow();
    bool result = false;

    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    IUIAutomation* uia = nullptr;
    if (SUCCEEDED(CoCreateInstance(CLSID_CUIAutomation, nullptr, CLSCTX_INPROC_SERVER,
                                   IID_IUIAutomation, reinterpret_cast<void**>(&uia))) && uia) {
        IUIAutomationElement* element = nullptr;
        if (SUCCEEDED(uia->GetFocusedElement(&element)) && element) {
            BOOL is_pass = FALSE;
            if (SUCCEEDED(element->get_CurrentIsPassword(&is_pass))) {
                result = (is_pass == TRUE);
            }
            element->Release();
        }
        uia->Release();
    }
    if (SUCCEEDED(hr)) {
        CoUninitialize();
    }

    cached_hwnd_ = fg;
    cached_value_ = result;
    cached_at_ms_ = NowMilliseconds();
}

} // namespace Ultimakey
