#pragma once

#include "types.hpp"

namespace Ultimakey {

class SettingsDialog {
public:
    static void Show(HWND parent_hwnd, HINSTANCE hinstance);
    static HWND GetHwnd() noexcept;

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
};

} // namespace Ultimakey
