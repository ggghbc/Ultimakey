#include "antiresonance.hpp"

namespace Ultimakey {

AntiResonance::AntiResonance(double window, int max_flips, double freeze_for)
    : window_(window), max_flips_(max_flips), freeze_for_(freeze_for) {}

bool AntiResonance::Allow(std::wstring_view word, std::wstring_view produced) {
    double now = NowSeconds();
    if (now < frozen_until_) return false;

    // Filter expired entries in-place
    size_t write_idx = 0;
    for (size_t i = 0; i < count_; ++i) {
        if (now - recent_[i].at <= window_) {
            if (write_idx != i) recent_[write_idx] = recent_[i];
            write_idx++;
        }
    }
    count_ = write_idx;

    // Oscillation detection: have we produced 'word' recently?
    bool oscillation = false;
    for (size_t i = 0; i < count_; ++i) {
        if (recent_[i].Equals(word)) {
            oscillation = true;
            break;
        }
    }

    if (count_ < kMaxRecent) {
        recent_[count_++].Set(produced, now);
    } else {
        // Shift left by 1 and append
        for (size_t i = 0; i + 1 < kMaxRecent; ++i) {
            recent_[i] = recent_[i + 1];
        }
        recent_[kMaxRecent - 1].Set(produced, now);
    }

    if (oscillation || static_cast<int>(count_) > max_flips_) {
        frozen_until_ = now + freeze_for_;
        count_ = 0;
        return false;
    }

    return true;
}

void AntiResonance::ResetHistory() {
    count_ = 0;
    frozen_until_ = 0.0;
}

} // namespace Ultimakey
