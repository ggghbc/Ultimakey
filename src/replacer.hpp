#pragma once

#include "types.hpp"
#include <string>
#include <functional>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>

namespace Ultimakey {

class TextReplacer {
public:
    static TextReplacer& Instance();

    void Replace(int delete_count, std::wstring_view text, bool then_return = false,
                 std::function<void(bool)> completion = nullptr);

    void Stop();
    void SetMessageHwnd(HWND hwnd) noexcept { msg_hwnd_ = hwnd; }

private:
    TextReplacer();
    ~TextReplacer();

    void WorkerLoop();
    void PerformReplace(int delete_count, const std::wstring& text, bool then_return);

    struct Job {
        int delete_count;
        std::wstring text;
        bool then_return;
        std::function<void(bool)> completion;
    };

    std::queue<Job> queue_;
    std::mutex mutex_;
    std::condition_variable cv_;
    std::atomic<bool> running_{true};
    std::thread worker_;
    HWND msg_hwnd_ = nullptr;
};

} // namespace Ultimakey
