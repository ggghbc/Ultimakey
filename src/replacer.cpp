#include "replacer.hpp"
#include <vector>

namespace Ultimakey {

TextReplacer& TextReplacer::Instance() {
    static TextReplacer instance;
    return instance;
}

TextReplacer::TextReplacer() {
    worker_ = std::thread(&TextReplacer::WorkerLoop, this);
}

TextReplacer::~TextReplacer() {
    Stop();
}

void TextReplacer::Stop() {
    if (running_.exchange(false)) {
        cv_.notify_all();
        if (worker_.joinable()) {
            worker_.join();
        }
    }
}

void TextReplacer::Replace(int delete_count, std::wstring_view text, bool then_return,
                           std::function<void(bool)> completion) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push(Job{delete_count, std::wstring(text), then_return, std::move(completion)});
    }
    cv_.notify_one();
}

void TextReplacer::WorkerLoop() {
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);

    while (running_) {
        Job job;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait(lock, [this]() { return !running_ || !queue_.empty(); });
            if (!running_ && queue_.empty()) break;
            job = std::move(queue_.front());
            queue_.pop();
        }

        PerformReplace(job.delete_count, job.text, job.then_return);
        bool success = true;

        if (job.completion) {
            job.completion(success);
        }
    }
}

void TextReplacer::PerformReplace(int delete_count, const std::wstring& text, bool then_return) {
    // Settle pause (5 ms with timeBeginPeriod(1)) to let target app consume keystrokes
    Sleep(5);

    std::vector<INPUT> inputs;
    inputs.reserve((delete_count + text.length() + (then_return ? 1 : 0)) * 2);

    // 1. Backspaces
    for (int i = 0; i < delete_count; ++i) {
        INPUT down = {};
        down.type = INPUT_KEYBOARD;
        down.ki.wVk = VK_BACK;
        down.ki.dwExtraInfo = SYNTH_MARKER;

        INPUT up = down;
        up.ki.dwFlags = KEYEVENTF_KEYUP;

        inputs.push_back(down);
        inputs.push_back(up);
    }

    // 2. Unicode characters
    for (wchar_t c : text) {
        INPUT down = {};
        down.type = INPUT_KEYBOARD;
        down.ki.wScan = static_cast<WORD>(c);
        down.ki.dwFlags = KEYEVENTF_UNICODE;
        down.ki.dwExtraInfo = SYNTH_MARKER;

        INPUT up = down;
        up.ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;

        inputs.push_back(down);
        inputs.push_back(up);
    }

    // 3. Return if requested
    if (then_return) {
        INPUT down = {};
        down.type = INPUT_KEYBOARD;
        down.ki.wVk = VK_RETURN;
        down.ki.dwExtraInfo = SYNTH_MARKER;

        INPUT up = down;
        up.ki.dwFlags = KEYEVENTF_KEYUP;

        inputs.push_back(down);
        inputs.push_back(up);
    }

    if (!inputs.empty()) {
        SendInput(static_cast<UINT>(inputs.size()), inputs.data(), sizeof(INPUT));
    }
}

} // namespace Ultimakey
