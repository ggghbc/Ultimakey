#pragma once

#include "types.hpp"
#include <vector>
#include <string>

namespace Ultimakey {

class AntiResonance {
public:
    AntiResonance(double window = 0.7, int max_flips = 6, double freeze_for = 2.5);

    bool Allow(std::wstring_view word, std::wstring_view produced);
    void ResetHistory();

private:
    struct Record {
        std::wstring produced;
        double at;
    };

    double window_;
    int max_flips_;
    double freeze_for_;
    double frozen_until_ = 0.0;
    std::vector<Record> recent_;
};

} // namespace Ultimakey
