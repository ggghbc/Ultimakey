#pragma once

#include "types.hpp"
#include "layout_mgr.hpp"

namespace Ultimakey {

class CapsRemap {
public:
    static bool HandleCapsKey(bool enabled, bool key_up) noexcept {
        if (!enabled) return false;
        if (!key_up) {
            // Switch layout on key down
            bool cyr = LayoutManager::Instance().CurrentIsCyrillic();
            LayoutManager::Instance().SelectLayout(!cyr);
        }
        // Swallow CapsLock key event so Windows doesn't toggle CapsLock state/LED
        return true;
    }
};

} // namespace Ultimakey
