#pragma once

#include "types.hpp"
#include <atomic>
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>

namespace Ultimakey {

class SecureInput {
public:
    static SecureInput& Instance();

    bool CachedIsPassword(HWND fg = nullptr) const noexcept;
    void KickAsync();
    bool IsPasswordFocused(int timeout_ms = 80);

    // Fast Win32 style check (no COM, nanosecond latency)
    static bool IsPasswordFastWin32(HWND fg) noexcept;

    // Selection query handled by the same single persistent COM worker
    std::wstring GetSelectionText(int timeout_ms = 120);

    void Shutdown();

private:
    SecureInput();
    ~SecureInput();

    void WorkerLoop();

    mutable HWND cached_hwnd_ = nullptr;
    mutable bool cached_value_ = false;
    mutable int64_t cached_at_ms_ = 0;

    std::thread worker_;
    std::mutex mutex_;
    std::condition_variable cv_;
    bool running_ = true;

    enum class JobType { None, CheckPassword, GetSelection };
    JobType current_job_ = JobType::None;
    bool job_done_ = false;
    std::wstring selection_result_;
};

} // namespace Ultimakey
