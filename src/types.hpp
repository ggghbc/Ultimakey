#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
#include <shellapi.h>
#include <commctrl.h>
#include "uia_interfaces.hpp"

#include <cstdint>
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>
#include <memory>
#include <algorithm>
#include <chrono>

namespace Ultimakey {

// Synthetic input marker in KBDLLHOOKSTRUCT::dwExtraInfo ('KBOO')
inline constexpr uintptr_t SYNTH_MARKER = 0x4B424F4Fu;

// Script classification
enum class Script {
    Latin,
    Cyrillic,
    Other
};

// Character helper functions
inline bool IsCyrillic(wchar_t c) noexcept {
    return (c >= 0x0400 && c <= 0x04FF) || c == 0x0500 || c == 0x0501;
}

inline bool IsLatin(wchar_t c) noexcept {
    return (c >= L'A' && c <= L'Z') || (c >= L'a' && c <= L'z');
}

inline bool HasCyrillic(std::wstring_view s) noexcept {
    for (wchar_t c : s) {
        if (IsCyrillic(c)) return true;
    }
    return false;
}

inline bool HasLatin(std::wstring_view s) noexcept {
    for (wchar_t c : s) {
        if (IsLatin(c)) return true;
    }
    return false;
}

inline bool IsAsciiDigit(wchar_t c) noexcept {
    return c >= L'0' && c <= L'9';
}

inline wchar_t ToLower(wchar_t c) noexcept {
    if (c >= L'A' && c <= L'Z') return c + (L'a' - L'A');
    if (c >= 0x0410 && c <= 0x042F) return c + 0x20; // Russian А-Я to а-я
    if (c == 0x0401) return 0x0451;                 // Ё to ё
    return towlower(c);
}

inline wchar_t ToUpper(wchar_t c) noexcept {
    if (c >= L'a' && c <= L'z') return c - (L'a' - L'A');
    if (c >= 0x0430 && c <= 0x044F) return c - 0x20; // Russian а-я to А-Я
    if (c == 0x0451) return 0x0401;                 // ё to Ё
    return towupper(c);
}

inline std::wstring ToLower(std::wstring_view s) {
    std::wstring res;
    res.reserve(s.length());
    for (wchar_t c : s) {
        res.push_back(ToLower(c));
    }
    return res;
}

inline std::wstring ToUpper(std::wstring_view s) {
    std::wstring res;
    res.reserve(s.length());
    for (wchar_t c : s) {
        res.push_back(ToUpper(c));
    }
    return res;
}

inline double NowSeconds() noexcept {
    using namespace std::chrono;
    return duration_cast<duration<double>>(steady_clock::now().time_since_epoch()).count();
}

inline int64_t NowMilliseconds() noexcept {
    using namespace std::chrono;
    return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

} // namespace Ultimakey
