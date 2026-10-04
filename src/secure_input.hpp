#pragma once

#include "types.hpp"
#include <atomic>

namespace Ultimakey {

class SecureInput {
public:
    static SecureInput& Instance();

    bool CachedIsPassword() const noexcept;
    void KickAsync();
    bool IsPasswordFocused(int timeout_ms = 120);

private:
    SecureInput();
    ~SecureInput();

    void QueryInternal();

    mutable HWND cached_hwnd_ = nullptr;
    mutable bool cached_value_ = false;
    mutable int64_t cached_at_ms_ = 0;

    std::atomic<bool> in_flight_{false};
};

} // namespace Ultimakey
