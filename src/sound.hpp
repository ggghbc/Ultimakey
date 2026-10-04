#pragma once

#include "types.hpp"
#include <vector>

namespace Ultimakey {

class SoundEffect {
public:
    static SoundEffect& Instance();

    void PlaySwitchSound();
    void SetEnabled(bool enabled) noexcept { enabled_ = enabled; }
    bool IsEnabled() const noexcept { return enabled_; }

private:
    SoundEffect();
    void GenerateWav();

    bool enabled_ = false;
    std::vector<uint8_t> wav_data_;
};

} // namespace Ultimakey
