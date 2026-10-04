#include "secure_input.hpp"
#include "uia_interfaces.hpp"
#include <chrono>

namespace Ultimakey {

SecureInput& SecureInput::Instance() {
    static SecureInput instance;
    return instance;
}

SecureInput::SecureInput() {
    worker_ = std::thread(&SecureInput::WorkerLoop, this);
}

SecureInput::~SecureInput() {
    Shutdown();
}

void SecureInput::Shutdown() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!running_) return;
        running_ = false;
        current_job_ = JobType::None;
    }
    cv_.notify_all();
    if (worker_.joinable()) {
        worker_.join();
    }
}

bool SecureInput::IsPasswordFastWin32(HWND fg) noexcept {
    if (!fg) return false;
    DWORD tid = GetWindowThreadProcessId(fg, nullptr);
    if (!tid) return false;
    GUITHREADINFO gti = {};
    gti.cbSize = sizeof(gti);
    if (GetGUIThreadInfo(tid, &gti) && gti.hwndFocus) {
        LONG style = GetWindowLongW(gti.hwndFocus, GWL_STYLE);
        if (style & ES_PASSWORD) return true;
    }
    return false;
}

bool SecureInput::CachedIsPassword(HWND fg) const noexcept {
    if (!fg) fg = GetForegroundWindow();
    int64_t now = NowMilliseconds();
    if (fg && fg == cached_hwnd_) {
        if (now - cached_at_ms_ < 1500) return cached_value_;
        // If window didn't change, stay protected and request fresh UIA check
        const_cast<SecureInput*>(this)->KickAsync();
        return cached_value_;
    }
    bool is_pass = IsPasswordFastWin32(fg);
    cached_hwnd_ = fg;
    cached_value_ = is_pass;
    cached_at_ms_ = now;
    const_cast<SecureInput*>(this)->KickAsync();
    return is_pass;
}

void SecureInput::KickAsync() {
    HWND fg = GetForegroundWindow();
    if (IsPasswordFastWin32(fg)) {
        cached_hwnd_ = fg;
        cached_value_ = true;
        cached_at_ms_ = NowMilliseconds();
        return;
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (current_job_ == JobType::None) {
            current_job_ = JobType::CheckPassword;
            job_done_ = false;
            cv_.notify_one();
        }
    }
}

bool SecureInput::IsPasswordFocused(int timeout_ms) {
    HWND fg = GetForegroundWindow();
    if (IsPasswordFastWin32(fg)) return true;
    if (fg == cached_hwnd_ && (NowMilliseconds() - cached_at_ms_ < 1500)) {
        return cached_value_;
    }

    KickAsync();
    {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait_for(lock, std::chrono::milliseconds(timeout_ms), [this]() {
            return job_done_ || !running_;
        });
    }
    return cached_value_;
}

std::wstring SecureInput::GetSelectionText(int timeout_ms) {
    {
        std::unique_lock<std::mutex> lock(mutex_);
        current_job_ = JobType::GetSelection;
        job_done_ = false;
        selection_result_.clear();
        cv_.notify_one();

        if (cv_.wait_for(lock, std::chrono::milliseconds(timeout_ms), [this]() { return job_done_ || !running_; })) {
            return selection_result_;
        }
    }
    return L"";
}

void SecureInput::WorkerLoop() {
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    IUIAutomation* uia = nullptr;
    if (FAILED(CoCreateInstance(CLSID_CUIAutomation, nullptr, CLSCTX_INPROC_SERVER,
                                IID_IUIAutomation, reinterpret_cast<void**>(&uia)))) {
        uia = nullptr;
    }

    while (running_) {
        JobType job = JobType::None;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait(lock, [this]() { return !running_ || current_job_ != JobType::None; });
            if (!running_) break;
            job = current_job_;
        }

        if (job == JobType::CheckPassword && uia) {
            HWND fg = GetForegroundWindow();
            bool result = false;
            IUIAutomationElement* element = nullptr;
            if (SUCCEEDED(uia->GetFocusedElement(&element)) && element) {
                BOOL is_pass = FALSE;
                if (SUCCEEDED(element->get_CurrentIsPassword(&is_pass))) {
                    result = (is_pass == TRUE);
                }
                element->Release();
            }
            cached_hwnd_ = fg;
            cached_value_ = result;
            cached_at_ms_ = NowMilliseconds();
        } else if (job == JobType::GetSelection && uia) {
            std::wstring res;
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
                                    res = text;
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
            std::lock_guard<std::mutex> lock(mutex_);
            selection_result_ = std::move(res);
            current_job_ = JobType::None;
            job_done_ = true;
        }
        cv_.notify_all();
    }

    if (uia) {
        uia->Release();
        uia = nullptr;
    }
    if (SUCCEEDED(hr)) {
        CoUninitialize();
    }
}

} // namespace Ultimakey
