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

        if (msg_hwnd_) {
            PostMessageW(msg_hwnd_, WM_APP + 102, success ? 1 : 0, 0);
        } else if (job.completion) {
            job.completion(success);
        }
    }
}

void TextReplacer::PerformReplace(int delete_count, const std::wstring& text, bool then_return) {
    // Settle pause (5 ms with timeBeginPeriod(1)) to let target app consume keystrokes
    Sleep(5);

    // 1. Backspaces
    if (delete_count > 0) {
        INPUT back_stack[64];
        INPUT* back_inputs = back_stack;
        std::vector<INPUT> back_heap;
        size_t back_needed = static_cast<size_t>(delete_count) * 2;
        if (back_needed > 64) {
            back_heap.resize(back_needed);
            back_inputs = back_heap.data();
        }
        size_t b_idx = 0;
        for (int i = 0; i < delete_count; ++i) {
            INPUT down = {};
            down.type = INPUT_KEYBOARD;
            down.ki.wVk = VK_BACK;
            down.ki.dwExtraInfo = SYNTH_MARKER;
            INPUT up = down;
            up.ki.dwFlags = KEYEVENTF_KEYUP;
            back_inputs[b_idx++] = down;
            back_inputs[b_idx++] = up;
        }
        SendInput(static_cast<UINT>(b_idx), back_inputs, sizeof(INPUT));
        Sleep(4); // Settle pause so target application processes deletion before new text arrives
    }

    // 2. Unicode characters
    if (!text.empty()) {
        INPUT text_stack[128];
        INPUT* text_inputs = text_stack;
        std::vector<INPUT> text_heap;
        size_t text_needed = text.length() * 2;
        if (text_needed > 128) {
            text_heap.resize(text_needed);
            text_inputs = text_heap.data();
        }
        size_t t_idx = 0;
        for (wchar_t c : text) {
            if (c == L'\r') continue;
            if (c == L'\n') {
                INPUT down = {};
                down.type = INPUT_KEYBOARD;
                down.ki.wVk = VK_RETURN;
                down.ki.dwExtraInfo = SYNTH_MARKER;
                INPUT up = down;
                up.ki.dwFlags = KEYEVENTF_KEYUP;
                text_inputs[t_idx++] = down;
                text_inputs[t_idx++] = up;
                continue;
            }
            INPUT down = {};
            down.type = INPUT_KEYBOARD;
            down.ki.wScan = static_cast<WORD>(c);
            down.ki.dwFlags = KEYEVENTF_UNICODE;
            down.ki.dwExtraInfo = SYNTH_MARKER;
            INPUT up = down;
            up.ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;
            text_inputs[t_idx++] = down;
            text_inputs[t_idx++] = up;
        }
        SendInput(static_cast<UINT>(t_idx), text_inputs, sizeof(INPUT));
    }

    // 3. Return if requested
    if (then_return) {
        Sleep(3);
        INPUT ret[2] = {};
        ret[0].type = INPUT_KEYBOARD;
        ret[0].ki.wVk = VK_RETURN;
        ret[0].ki.dwExtraInfo = SYNTH_MARKER;
        ret[1] = ret[0];
        ret[1].ki.dwFlags = KEYEVENTF_KEYUP;
        SendInput(2, ret, sizeof(INPUT));
    }
}

} // namespace Ultimakey
