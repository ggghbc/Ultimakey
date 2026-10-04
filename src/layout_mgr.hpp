#pragma once

#include "types.hpp"
#include <vector>

namespace Ultimakey {

class LayoutManager {
public:
    static LayoutManager& Instance();

    HKL CurrentHkl() const;
    Script CurrentScript() const;
    bool CurrentIsCyrillic() const;

    bool SelectLayout(bool cyrillic);
    bool CycleLayout();

    std::vector<HKL> InstalledLayouts() const;

private:
    LayoutManager() = default;
    bool RequestLayout(HWND hwnd, HKL hkl);
};

} // namespace Ultimakey
