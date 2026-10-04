#pragma once

#include "types.hpp"
#include <string>

namespace Ultimakey {

class SelectionText {
public:
    static std::wstring TryGet(int timeout_ms = 150);
};

} // namespace Ultimakey
