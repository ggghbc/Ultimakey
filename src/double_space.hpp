#pragma once

#include "types.hpp"

namespace Ultimakey {

class DoubleSpacePeriod {
public:
    static bool ShouldTrigger(bool enabled, double gap_seconds, std::wstring_view last_word) noexcept {
        if (!enabled) return false;
        if (gap_seconds < 0.0 || gap_seconds > 0.45) return false;
        if (last_word.empty()) return false;
        wchar_t last_char = last_word.back();
        return IsLatin(last_char) || IsCyrillic(last_char) || IsAsciiDigit(last_char);
    }
};

} // namespace Ultimakey
