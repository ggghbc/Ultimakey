#include "keymap.hpp"

namespace Ultimakey {

static const std::pair<wchar_t, wchar_t> kBasePairs[] = {
    {L'`', L'ё'}, {L'q', L'й'}, {L'w', L'ц'}, {L'e', L'у'}, {L'r', L'к'},
    {L't', L'е'}, {L'y', L'н'}, {L'u', L'г'}, {L'i', L'ш'}, {L'o', L'щ'},
    {L'p', L'з'}, {L'[', L'х'}, {L']', L'ъ'}, {L'a', L'ф'}, {L's', L'ы'},
    {L'd', L'в'}, {L'f', L'а'}, {L'g', L'п'}, {L'h', L'р'}, {L'j', L'о'},
    {L'k', L'л'}, {L'l', L'д'}, {L';', L'ж'}, {L'\'', L'э'}, {L'z', L'я'},
    {L'x', L'ч'}, {L'c', L'с'}, {L'v', L'м'}, {L'b', L'и'}, {L'n', L'т'},
    {L'm', L'ь'}, {L',', L'б'}, {L'.', L'ю'}, {L'/', L'.'}
};

static const std::pair<wchar_t, wchar_t> kShiftPairs[] = {
    {L'~', L'Ё'}, {L'@', L'\"'}, {L'#', L'№'}, {L'$', L';'}, {L'^', L':'},
    {L'&', L'?'}, {L'<', L'Б'}, {L'>', L'Ю'}, {L'?', L','}, {L'{', L'Х'},
    {L'}', L'Ъ'}, {L':', L'Ж'}, {L'\"', L'Э'}
};

Keymap& Keymap::Instance() {
    static Keymap instance;
    return instance;
}

Keymap::Keymap() {
    BuildStaticMaps();
    BuildDynamicMaps();
}

void Keymap::Initialize() {
    RefreshDynamicIfNeeded();
}

void Keymap::BuildStaticMaps() {
    static_en_to_ru_.Clear();
    static_ru_to_en_.Clear();

    for (const auto& [e, r] : kBasePairs) {
        static_en_to_ru_.Set(e, r);
        static_ru_to_en_.Set(r, e);

        wchar_t upper_e = ToUpper(e);
        wchar_t upper_r = ToUpper(r);
        if (upper_e != e && upper_r != r) {
            static_en_to_ru_.Set(upper_e, upper_r);
            static_ru_to_en_.Set(upper_r, upper_e);
        }
    }

    for (const auto& [e, r] : kShiftPairs) {
        static_en_to_ru_.Set(e, r);
        static_ru_to_en_.Set(r, e);
    }
}

bool Keymap::BuildDynamicMaps() {
    int count = GetKeyboardLayoutList(0, nullptr);
    if (count <= 0) return false;

    std::vector<HKL> layouts(count);
    count = GetKeyboardLayoutList(count, layouts.data());
    if (count <= 0) return false;

    HKL latin_hkl = nullptr;
    HKL cyrillic_hkl = nullptr;

    for (int i = 0; i < count; ++i) {
        WORD lang = LOWORD(layouts[i]);
        WORD primary = PRIMARYLANGID(lang);
        if (primary == LANG_ENGLISH && !latin_hkl) {
            latin_hkl = layouts[i];
        } else if (primary == LANG_RUSSIAN && !cyrillic_hkl) {
            cyrillic_hkl = layouts[i];
        }
    }

    if (!latin_hkl || !cyrillic_hkl) return false;

    if (dynamic_ready_ && cached_latin_hkl_ == latin_hkl && cached_cyrillic_hkl_ == cyrillic_hkl) {
        return true;
    }

    CharTable new_en_to_ru;
    CharTable new_ru_to_en;
    bool has_any = false;

    BYTE state_normal[256] = {};
    BYTE state_shift[256] = {};
    state_shift[VK_SHIFT] = 0x80;

    for (UINT sc = 1; sc <= 0x58; ++sc) {
        UINT vk_lat = MapVirtualKeyExW(sc, MAPVK_VSC_TO_VK_EX, latin_hkl);
        UINT vk_cyr = MapVirtualKeyExW(sc, MAPVK_VSC_TO_VK_EX, cyrillic_hkl);
        if (!vk_lat || !vk_cyr) continue;

        // Normal
        wchar_t buf_lat[8] = {};
        wchar_t buf_cyr[8] = {};
        int n_lat = ToUnicodeEx(vk_lat, sc, state_normal, buf_lat, 8, 4, latin_hkl);
        int n_cyr = ToUnicodeEx(vk_cyr, sc, state_normal, buf_cyr, 8, 4, cyrillic_hkl);
        if (n_lat == 1 && n_cyr == 1) {
            new_en_to_ru.Set(buf_lat[0], buf_cyr[0]);
            new_ru_to_en.Set(buf_cyr[0], buf_lat[0]);
            has_any = true;
        }

        // Shift
        wchar_t sbuf_lat[8] = {};
        wchar_t sbuf_cyr[8] = {};
        n_lat = ToUnicodeEx(vk_lat, sc, state_shift, sbuf_lat, 8, 4, latin_hkl);
        n_cyr = ToUnicodeEx(vk_cyr, sc, state_shift, sbuf_cyr, 8, 4, cyrillic_hkl);
        if (n_lat == 1 && n_cyr == 1) {
            new_en_to_ru.Set(sbuf_lat[0], sbuf_cyr[0]);
            new_ru_to_en.Set(sbuf_cyr[0], sbuf_lat[0]);
            has_any = true;
        }
    }

    dynamic_en_to_ru_ = new_en_to_ru;
    dynamic_ru_to_en_ = new_ru_to_en;
    cached_latin_hkl_ = latin_hkl;
    cached_cyrillic_hkl_ = cyrillic_hkl;
    dynamic_ready_ = has_any;
    return dynamic_ready_;
}

void Keymap::RefreshDynamicIfNeeded() {
    BuildDynamicMaps();
}

wchar_t Keymap::Straighten(wchar_t ch) noexcept {
    switch (ch) {
        case 0x2018: case 0x2019: case 0x00B4: case 0x2032: return L'\'';
        case 0x201C: case 0x201D: case 0x201F: return L'\"';
        default: return ch;
    }
}

bool Keymap::IsTrailingPunctuation(wchar_t c) noexcept {
    static const wchar_t punct[] = L".,!?;:…";
    for (size_t i = 0; i < sizeof(punct)/sizeof(punct[0]) - 1; ++i) {
        if (c == punct[i]) return true;
    }
    return false;
}

std::wstring_view Keymap::Core(std::wstring_view word) noexcept {
    size_t end = word.length();
    while (end > 0 && IsTrailingPunctuation(word[end - 1])) {
        end--;
    }
    return word.substr(0, end);
}

wchar_t Keymap::ConvertChar(wchar_t ch, bool to_cyrillic) const noexcept {
    const auto& dyn_map = to_cyrillic ? dynamic_en_to_ru_ : dynamic_ru_to_en_;
    const auto& stat_map = to_cyrillic ? static_en_to_ru_ : static_ru_to_en_;

    if (dynamic_ready_ && dyn_map.Contains(ch)) {
        return dyn_map.Map(ch);
    }

    if (stat_map.Contains(ch)) {
        return stat_map.Map(ch);
    }

    wchar_t straightened = Straighten(ch);
    if (straightened != ch && stat_map.Contains(straightened)) {
        return stat_map.Map(straightened);
    }

    return ch;
}

wchar_t Keymap::NumberSeparator(wchar_t ch, bool to_cyrillic, bool left_has_separator,
                                std::wstring_view text, size_t rest_from) const noexcept {
    if (to_cyrillic || ch == L'.' || left_has_separator) return ch;
    for (size_t j = rest_from; j < text.length(); ++j) {
        wchar_t c = text[j];
        if (IsAsciiDigit(c)) continue;
        if ((c == L',' || c == L'.') && j + 1 < text.length() && IsAsciiDigit(text[j + 1])) {
            return ch;
        }
        break;
    }
    return L'.';
}

std::wstring Keymap::Convert(std::wstring_view text, bool to_cyrillic) const {
    std::wstring result;
    result.reserve(text.length());
    bool run_has_separator = false;

    for (size_t i = 0; i < text.length(); ++i) {
        wchar_t ch = text[i];
        wchar_t prev = (i > 0) ? text[i - 1] : 0;
        wchar_t next = (i + 1 < text.length()) ? text[i + 1] : 0;

        if ((ch == L',' || ch == L'.') && IsAsciiDigit(prev) && IsAsciiDigit(next)) {
            result.push_back(NumberSeparator(ch, to_cyrillic, run_has_separator, text, i + 2));
            run_has_separator = true;
        } else {
            result.push_back(ConvertChar(ch, to_cyrillic));
            if (!IsAsciiDigit(ch)) run_has_separator = false;
        }
    }
    return result;
}

wchar_t Keymap::PeeledMark(wchar_t p, bool to_cyrillic) const noexcept {
    if (!to_cyrillic) return p;
    wchar_t mapped = ConvertChar(p, true);
    if (mapped != p && IsTrailingPunctuation(mapped)) {
        return mapped;
    }
    return p;
}

std::wstring Keymap::SmartConvert(std::wstring_view word, bool to_cyrillic,
                                  const std::function<bool(std::wstring_view)>& is_valid_target) const {
    if (to_cyrillic && is_valid_target) {
        std::wstring full = Convert(word, true);
        wchar_t buf[64];
        std::wstring heap;
        std::wstring_view lower;
        if (full.length() < 64) {
            for (size_t i = 0; i < full.length(); ++i) buf[i] = ToLower(full[i]);
            lower = std::wstring_view(buf, full.length());
        } else {
            heap = ToLower(full);
            lower = heap;
        }
        if (is_valid_target(lower)) {
            return full;
        }
    }

    size_t end = word.length();
    while (end > 0 && IsTrailingPunctuation(word[end - 1])) {
        end--;
    }

    if (end == word.length()) {
        return Convert(word, to_cyrillic);
    }

    std::wstring result = Convert(word.substr(0, end), to_cyrillic);
    result.reserve(result.length() + (word.length() - end));
    for (size_t i = end; i < word.length(); ++i) {
        result.push_back(PeeledMark(word[i], to_cyrillic));
    }
    return result;
}

} // namespace Ultimakey
