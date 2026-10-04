#include "antiresonance.hpp"

namespace Ultimakey {

AntiResonance::AntiResonance(double window, int max_flips, double freeze_for)
    : window_(window), max_flips_(max_flips), freeze_for_(freeze_for) {}

bool AntiResonance::Allow(std::wstring_view word, std::wstring_view produced) {
    double now = NowSeconds();
    if (now < frozen_until_) return false;

    // Remove expired entries
    recent_.erase(
        std::remove_if(recent_.begin(), recent_.end(),
            [this, now](const Record& r) { return now - r.at > window_; }),
        recent_.end());

    // Oscillation detection: have we produced 'word' recently?
    bool oscillation = false;
    for (const auto& r : recent_) {
        if (r.produced == word) {
            oscillation = true;
            break;
        }
    }

    recent_.push_back({std::wstring(produced), now});

    if (oscillation || static_cast<int>(recent_.size()) > max_flips_) {
        frozen_until_ = now + freeze_for_;
        recent_.clear();
        return false;
    }

    return true;
}

void AntiResonance::ResetHistory() {
    recent_.clear();
    frozen_until_ = 0.0;
}

} // namespace Ultimakey
