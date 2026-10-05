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
    HWND fg = GetForegroundWindow();
    if (fg) {
        DWORD thread_id = GetWindowThreadProcessId(fg, nullptr);
        if (thread_id) {
            HKL h = GetKeyboardLayout(thread_id);
            if (h) return h;
        }
    }
    DWORD tid = Engine::Instance().CurrentForegroundThreadId();
    if (tid != 0) {
        HKL h = GetKeyboardLayout(tid);
        if (h) return h;
    }
    HKL h = GetKeyboardLayout(0);
    if (h) return h;
    return cached_en_hkl_ ? cached_en_hkl_ : (HKL)0x04090409;
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
    if (!hkl) return false;
    HWND fg = GetForegroundWindow();
    if (!hwnd) hwnd = fg;
    if (!hwnd) return false;

    HWND root = GetAncestor(hwnd, GA_ROOT);
    if (!root) root = hwnd;

    // 1. Post to root window and specific hwnd
    PostMessageW(root, WM_INPUTLANGCHANGEREQUEST, 0, reinterpret_cast<LPARAM>(hkl));
    if (hwnd != root) {
        PostMessageW(hwnd, WM_INPUTLANGCHANGEREQUEST, 0, reinterpret_cast<LPARAM>(hkl));
    }

    DWORD target_tid = GetWindowThreadProcessId(root, nullptr);
    if (target_tid) {
        GUITHREADINFO gti = {};
        gti.cbSize = sizeof(gti);
        if (GetGUIThreadInfo(target_tid, &gti) && gti.hwndFocus && gti.hwndFocus != root && gti.hwndFocus != hwnd) {
            PostMessageW(gti.hwndFocus, WM_INPUTLANGCHANGEREQUEST, 0, reinterpret_cast<LPARAM>(hkl));
        }

        // 2. Direct thread activation via AttachThreadInput for apps that don't process WM_INPUTLANGCHANGEREQUEST
        DWORD my_tid = GetCurrentThreadId();
        if (target_tid != my_tid) {
            if (AttachThreadInput(my_tid, target_tid, TRUE)) {
                ActivateKeyboardLayout(hkl, 0);
                AttachThreadInput(my_tid, target_tid, FALSE);
            }
        }
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
    if (!fg) fg = Engine::Instance().LastForegroundHwnd();
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
    HWND fg = Engine::Instance().LastForegroundHwnd();
    if (!fg) fg = GetForegroundWindow();
    return RequestLayout(fg, target);
}

} // namespace Ultimakey
