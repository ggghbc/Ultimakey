#include "autostart.hpp"

namespace Ultimakey {

bool Autostart::IsEnabled() {
    HKEY hkey = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, kRunKey, 0, KEY_READ, &hkey) != ERROR_SUCCESS) {
        return false;
    }

    wchar_t buffer[MAX_PATH * 2] = {};
    DWORD size = sizeof(buffer);
    DWORD type = 0;
    LSTATUS status = RegQueryValueExW(hkey, kAppName, nullptr, &type, reinterpret_cast<LPBYTE>(buffer), &size);
    RegCloseKey(hkey);

    return (status == ERROR_SUCCESS);
}

bool Autostart::SetEnabled(bool enable) {
    HKEY hkey = nullptr;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, kRunKey, 0, nullptr, 0, KEY_SET_VALUE, nullptr, &hkey, nullptr) != ERROR_SUCCESS) {
        return false;
    }

    bool ok = false;
    if (enable) {
        wchar_t exe_path[MAX_PATH] = {};
        GetModuleFileNameW(nullptr, exe_path, MAX_PATH);
        std::wstring quoted = L"\"" + std::wstring(exe_path) + L"\"";
        DWORD size = static_cast<DWORD>((quoted.length() + 1) * sizeof(wchar_t));
        ok = (RegSetValueExW(hkey, kAppName, 0, REG_SZ, reinterpret_cast<const BYTE*>(quoted.c_str()), size) == ERROR_SUCCESS);
    } else {
        LSTATUS st = RegDeleteValueW(hkey, kAppName);
        ok = (st == ERROR_SUCCESS || st == ERROR_FILE_NOT_FOUND);
    }

    RegCloseKey(hkey);
    return ok;
}

} // namespace Ultimakey
