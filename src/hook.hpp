#pragma once

#include "types.hpp"

namespace Ultimakey {

class HookManager {
public:
    static HookManager& Instance();

    bool Install();
    void Uninstall();

    bool IsInstalled() const noexcept { return kb_hook_ != nullptr; }

private:
    HookManager() = default;
    ~HookManager();

    static LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam);

    HHOOK kb_hook_ = nullptr;
    HHOOK mouse_hook_ = nullptr;
};

} // namespace Ultimakey
