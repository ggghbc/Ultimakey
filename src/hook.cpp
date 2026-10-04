#include "hook.hpp"
#include "engine.hpp"
#include "logger.hpp"

namespace Ultimakey {

HookManager& HookManager::Instance() {
    static HookManager instance;
    return instance;
}

HookManager::~HookManager() {
    Uninstall();
}

bool HookManager::Install() {
    if (kb_hook_) return true;

    HINSTANCE hinst = GetModuleHandleW(nullptr);
    kb_hook_ = SetWindowsHookExW(WH_KEYBOARD_LL, LowLevelKeyboardProc, hinst, 0);
    mouse_hook_ = SetWindowsHookExW(WH_MOUSE_LL, LowLevelMouseProc, hinst, 0);

    if (!kb_hook_) {
        Logger::Instance().Write("Hook: Ошибка установки хука клавиатуры");
        return false;
    }

    Logger::Instance().Write("Hook: Хуки клавиатуры и мыши успешно установлены");
    return true;
}

void HookManager::Uninstall() {
    if (kb_hook_) {
        UnhookWindowsHookEx(kb_hook_);
        kb_hook_ = nullptr;
    }
    if (mouse_hook_) {
        UnhookWindowsHookEx(mouse_hook_);
        mouse_hook_ = nullptr;
    }
}

LRESULT CALLBACK HookManager::LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0 && lParam) {
        const auto& kb = *reinterpret_cast<const KBDLLHOOKSTRUCT*>(lParam);
        if (Engine::Instance().OnKeyboardHook(nCode, wParam, kb)) {
            return 1; // Swallow key
        }
    }
    return CallNextHookEx(Instance().kb_hook_, nCode, wParam, lParam);
}

LRESULT CALLBACK HookManager::LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0) {
        Engine::Instance().OnMouseHook(nCode, wParam);
    }
    return CallNextHookEx(Instance().mouse_hook_, nCode, wParam, lParam);
}

} // namespace Ultimakey
