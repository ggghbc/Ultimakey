#pragma once

#include "types.hpp"

namespace Ultimakey {

class SoundEffect {
public:
    static SoundEffect& Instance();

    void PlaySwitchSound();
    void PlayTestSound();
    void SetEnabled(bool enabled) noexcept { enabled_ = enabled; }
    bool IsEnabled() const noexcept { return enabled_; }

private:
    SoundEffect() = default;

    bool enabled_ = false;
};

} // namespace Ultimakey
