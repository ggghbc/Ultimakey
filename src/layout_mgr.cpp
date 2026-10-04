#include "layout_mgr.hpp"
#include "engine.hpp"
#include <algorithm>

namespace Ultimakey {

LayoutManager& LayoutManager::Instance() {
    static LayoutManager instance;
    return instance;
}

std::vector<HKL> LayoutManager::InstalledLayouts() const {
    int n = GetKeyboardLayoutList(0, nullptr);
    if (n <= 0) return {};
    std::vector<HKL> layouts(n);
    n = GetKeyboardLayoutList(n, layouts.data());
    if (n <= 0) return {};
    layouts.resize(n);
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
    auto all = InstalledLayouts();
    HKL target = nullptr;

    for (HKL h : all) {
        WORD primary = PRIMARYLANGID(LOWORD(h));
        if (cyrillic && primary == LANG_RUSSIAN) {
            target = h;
            break;
        } else if (!cyrillic && primary == LANG_ENGLISH) {
            target = h;
            break;
        }
    }

    if (!target) return false;
    HWND fg = GetForegroundWindow();
    return RequestLayout(fg, target);
}

bool LayoutManager::CycleLayout() {
    auto all = InstalledLayouts();
    if (all.size() < 2) return false;

    HKL cur = CurrentHkl();
    auto it = std::find(all.begin(), all.end(), cur);
    size_t next_idx = 0;
    if (it != all.end()) {
        next_idx = (std::distance(all.begin(), it) + 1) % all.size();
    }
    HKL target = all[next_idx];
    HWND fg = GetForegroundWindow();
    return RequestLayout(fg, target);
}

} // namespace Ultimakey
