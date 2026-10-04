#pragma once

#include "types.hpp"
#include <array>
#include <algorithm>

namespace Ultimakey {

class AntiResonance {
public:
    AntiResonance(double window = 0.7, int max_flips = 6, double freeze_for = 2.5);

    bool Allow(std::wstring_view word, std::wstring_view produced);
    void ResetHistory();

private:
    struct FixedRecord {
        wchar_t produced[32];
        size_t len = 0;
        double at = 0.0;

        void Set(std::wstring_view s, double t) noexcept {
            len = (std::min)(s.length(), size_t(31));
            for (size_t i = 0; i < len; ++i) produced[i] = s[i];
            produced[len] = 0;
            at = t;
        }

        bool Equals(std::wstring_view s) const noexcept {
            if (len != s.length()) return false;
            for (size_t i = 0; i < len; ++i) {
                if (produced[i] != s[i]) return false;
            }
            return true;
        }
    };

    double window_;
    int max_flips_;
    double freeze_for_;
    double frozen_until_ = 0.0;

    static constexpr size_t kMaxRecent = 8;
    std::array<FixedRecord, kMaxRecent> recent_{};
    size_t count_ = 0;
};

} // namespace Ultimakey
