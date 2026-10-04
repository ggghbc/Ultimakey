#pragma once

#include "types.hpp"
#include <unordered_map>
#include <vector>
#include <functional>

namespace Ultimakey {

class Keymap {
public:
    static Keymap& Instance();

    void Initialize();
    void RefreshDynamicIfNeeded();

    std::wstring Convert(std::wstring_view text, bool to_cyrillic) const;
    std::wstring SmartConvert(std::wstring_view word, bool to_cyrillic,
                              const std::function<bool(std::wstring_view)>& is_valid_target = nullptr) const;

    static std::wstring_view Core(std::wstring_view word) noexcept;
    static wchar_t Straighten(wchar_t ch) noexcept;
    static bool IsTrailingPunctuation(wchar_t c) noexcept;

    bool DynamicReady() const noexcept { return dynamic_ready_; }

private:
    Keymap();

    void BuildStaticMaps();
    bool BuildDynamicMaps();

    wchar_t ConvertChar(wchar_t ch, bool to_cyrillic) const noexcept;
    wchar_t PeeledMark(wchar_t p, bool to_cyrillic) const noexcept;
    wchar_t NumberSeparator(wchar_t ch, bool to_cyrillic, bool left_has_separator,
                            std::wstring_view text, size_t rest_from) const noexcept;

    std::unordered_map<wchar_t, wchar_t> static_en_to_ru_;
    std::unordered_map<wchar_t, wchar_t> static_ru_to_en_;

    std::unordered_map<wchar_t, wchar_t> dynamic_en_to_ru_;
    std::unordered_map<wchar_t, wchar_t> dynamic_ru_to_en_;

    std::wstring built_for_signature_;
    bool dynamic_ready_ = false;
};

} // namespace Ultimakey
