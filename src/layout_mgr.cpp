#include "layout_mgr.hpp"
#include "engine.hpp"
#include <algorithm>

namespace Ultimakey {

LayoutManager& LayoutManager::Instance() {
    static LayoutManager instance;
    return instance;
}

LayoutManager::LayoutManager() {
    RefreshLayouts();
}

void LayoutManager::Initialize() {
    RefreshLayouts();
}

void LayoutManager::RefreshLayouts() {
    int n = GetKeyboardLayoutList(0, nullptr);
    if (n <= 0) return;
    std::array<HKL, 16> temp{};
    int count = GetKeyboardLayoutList(static_cast<int>(temp.size()), temp.data());
    layout_count_ = (std::min)(static_cast<size_t>(count), cached_layouts_.size());
    cached_ru_hkl_ = nullptr;
    cached_en_hkl_ = nullptr;

    for (size_t i = 0; i < layout_count_; ++i) {
        cached_layouts_[i] = temp[i];
        WORD primary = PRIMARYLANGID(LOWORD(temp[i]));
        if (!cached_ru_hkl_ && primary == LANG_RUSSIAN) cached_ru_hkl_ = temp[i];
        if (!cached_en_hkl_ && primary == LANG_ENGLISH) cached_en_hkl_ = temp[i];
    }
}

std::vector<HKL> LayoutManager::InstalledLayouts() const {
    std::vector<HKL> layouts;
    layouts.reserve(layout_count_);
    for (size_t i = 0; i < layout_count_; ++i) {
        layouts.push_back(cached_layouts_[i]);
    }
    return layouts;
}

HKL LayoutManager::CurrentHkl() const {
    DWORD tid = Engine::Instance().CurrentForegroundThreadId();
    if (tid != 0) return GetKeyboardLayout(tid);
    HWND fg = GetForegroundWindow();
    if (!fg) return GetKeyboardLayout(0);
    DWORD thread_id = GetWindowThreadProcessId(fg, nullptr);
    return GetKeyboardLayout(thread_id);
}

Script LayoutManager::CurrentScript() const {
    HKL hkl = CurrentHkl();
    WORD lang = LOWORD(hkl);
    WORD primary = PRIMARYLANGID(lang);
    if (primary == LANG_RUSSIAN) return Script::Cyrillic;
    if (primary == LANG_ENGLISH) return Script::Latin;
    return Script::Other;
}

bool LayoutManager::CurrentIsCyrillic() const {
    return CurrentScript() == Script::Cyrillic;
}

bool LayoutManager::RequestLayout(HWND hwnd, HKL hkl) {
    if (!hwnd || !hkl) return false;

    // Post to top-level window
    PostMessageW(hwnd, WM_INPUTLANGCHANGEREQUEST, 0, reinterpret_cast<LPARAM>(hkl));

    // Also post to specific focused child control if available
    DWORD thread_id = GetWindowThreadProcessId(hwnd, nullptr);
    GUITHREADINFO gti = {};
    gti.cbSize = sizeof(gti);
    if (GetGUIThreadInfo(thread_id, &gti) && gti.hwndFocus && gti.hwndFocus != hwnd) {
        PostMessageW(gti.hwndFocus, WM_INPUTLANGCHANGEREQUEST, 0, reinterpret_cast<LPARAM>(hkl));
    }

    return true;
}

bool LayoutManager::SelectLayout(bool cyrillic) {
    HKL target = cyrillic ? cached_ru_hkl_ : cached_en_hkl_;
    if (!target) {
        RefreshLayouts();
        target = cyrillic ? cached_ru_hkl_ : cached_en_hkl_;
    }

    if (!target) return false;
    HWND fg = GetForegroundWindow();
    return RequestLayout(fg, target);
}

bool LayoutManager::CycleLayout() {
    if (layout_count_ < 2) {
        RefreshLayouts();
        if (layout_count_ < 2) return false;
    }

    HKL cur = CurrentHkl();
    size_t next_idx = 0;
    for (size_t i = 0; i < layout_count_; ++i) {
        if (cached_layouts_[i] == cur) {
            next_idx = (i + 1) % layout_count_;
            break;
        }
    }

    HKL target = cached_layouts_[next_idx];
    HWND fg = GetForegroundWindow();
    return RequestLayout(fg, target);
}

} // namespace Ultimakey
