#pragma once

#include "types.hpp"
#include <array>
#include <vector>

namespace Ultimakey {

class LayoutManager {
public:
    static LayoutManager& Instance();

    void Initialize();
    void RefreshLayouts();

    HKL CurrentHkl() const;
    Script CurrentScript() const;
    bool CurrentIsCyrillic() const;

    bool SelectLayout(bool cyrillic);
    bool CycleLayout();

    std::vector<HKL> InstalledLayouts() const;

private:
    LayoutManager();
    bool RequestLayout(HWND hwnd, HKL hkl);

    std::array<HKL, 8> cached_layouts_{};
    size_t layout_count_ = 0;
    HKL cached_ru_hkl_ = nullptr;
    HKL cached_en_hkl_ = nullptr;
};

} // namespace Ultimakey
