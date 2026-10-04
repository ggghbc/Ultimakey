#pragma once

#include "types.hpp"
#include <unordered_map>
#include <vector>
#include <functional>
#include <cstring>

namespace Ultimakey {

class Keymap {
public:
    static Keymap& Instance();

    void Initialize();
    void RefreshDynamicIfNeeded();

    std::wstring Convert(std::wstring_view text, bool to_cyrillic) const;
    std::wstring SmartConvert(std::wstring_view word, bool to_cyrillic,
                              const std::function<bool(std::wstring_view)>& is_valid_target = nullptr) const;
    wchar_t ConvertChar(wchar_t ch, bool to_cyrillic) const noexcept;

    static std::wstring_view Core(std::wstring_view word) noexcept;
    static wchar_t Straighten(wchar_t ch) noexcept;
    static bool IsTrailingPunctuation(wchar_t c) noexcept;

    bool DynamicReady() const noexcept { return dynamic_ready_; }

private:
    Keymap();

    void BuildStaticMaps();
    bool BuildDynamicMaps();

    wchar_t PeeledMark(wchar_t p, bool to_cyrillic) const noexcept;
    wchar_t NumberSeparator(wchar_t ch, bool to_cyrillic, bool left_has_separator,
                            std::wstring_view text, size_t rest_from) const noexcept;

    struct CharTable {
        wchar_t ascii[128]{};
        wchar_t cyrillic[96]{}; // 0x0400 .. 0x045F

        inline wchar_t Map(wchar_t c) const noexcept {
            if (c < 128) return ascii[c] ? ascii[c] : c;
            if (c >= 0x0400 && c <= 0x045F) return cyrillic[c - 0x0400] ? cyrillic[c - 0x0400] : c;
            return c;
        }

        inline bool Contains(wchar_t c) const noexcept {
            if (c < 128) return ascii[c] != 0;
            if (c >= 0x0400 && c <= 0x045F) return cyrillic[c - 0x0400] != 0;
            return false;
        }

        inline void Set(wchar_t from, wchar_t to) noexcept {
            if (from < 128) ascii[from] = to;
            else if (from >= 0x0400 && from <= 0x045F) cyrillic[from - 0x0400] = to;
        }

        inline void Clear() noexcept {
            std::memset(ascii, 0, sizeof(ascii));
            std::memset(cyrillic, 0, sizeof(cyrillic));
        }
    };

    CharTable static_en_to_ru_;
    CharTable static_ru_to_en_;

    CharTable dynamic_en_to_ru_;
    CharTable dynamic_ru_to_en_;

    HKL cached_latin_hkl_ = nullptr;
    HKL cached_cyrillic_hkl_ = nullptr;
    bool dynamic_ready_ = false;
};

} // namespace Ultimakey
