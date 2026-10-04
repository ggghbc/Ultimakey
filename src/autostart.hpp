#pragma once

#include "types.hpp"

namespace Ultimakey {

class Autostart {
public:
    static bool IsEnabled();
    static bool SetEnabled(bool enable);

private:
    static constexpr const wchar_t* kRunKey = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
    static constexpr const wchar_t* kAppName = L"Ultimakey";
};

} // namespace Ultimakey
