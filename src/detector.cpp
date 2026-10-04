#include "detector.hpp"
#include "keymap.hpp"
#include "data/extrawords_data.hpp"

namespace Ultimakey {

constexpr double kMargin = 2.0;
constexpr double kShortEnSwapFloor = -10.0;

LayoutDetector::LayoutDetector(const Dawg& words_ru, const Dawg& words_en,
                               const TrigramTable& trigrams_ru, const TrigramTable& trigrams_en)
    : words_ru_(words_ru), words_en_(words_en),
      trigrams_ru_(trigrams_ru), trigrams_en_(trigrams_en) {}

ContextHint LayoutDetector::ContextOf(std::wstring_view word) noexcept {
    if (word.empty()) return ContextHint::None;
    if (HasCyrillic(word)) return ContextHint::Cyrillic;
    if (HasLatin(word)) return ContextHint::Latin;
    return ContextHint::None;
}

double LayoutDetector::Plausibility(std::wstring_view word, bool cyrillic) const noexcept {
    return cyrillic ? trigrams_ru_.Plausibility(word) : trigrams_en_.Plausibility(word);
}

bool LayoutDetector::WordExistsInSource(std::wstring_view w, bool cyrillic) const noexcept {
    return cyrillic ? words_ru_.Contains(w) : words_en_.Contains(w);
}

bool LayoutDetector::IsLayoutLetter(wchar_t c) noexcept {
    if (IsLatin(c) || IsCyrillic(c)) return true;
    wchar_t r = Keymap::Instance().ConvertChar(c, true);
    if (IsCyrillic(r)) return true;
    wchar_t e = Keymap::Instance().ConvertChar(c, false);
    if (IsLatin(e)) return true;
    return false;
}

std::wstring_view LayoutDetector::LetterCore(std::wstring_view raw) noexcept {
    size_t s = 0, e = raw.length();
    while (s < e && !IsLayoutLetter(raw[s]) && !IsAsciiDigit(raw[s])) s++;
    while (e > s && !IsLayoutLetter(raw[e - 1]) && !IsAsciiDigit(raw[e - 1])) e--;
    return raw.substr(s, e - s);
}

std::vector<std::wstring> LayoutDetector::DeElongated(std::wstring_view s) {
    if (s.length() < 3) return {};

    bool has_elongation = false;
    int run = 0;
    wchar_t prev = 0;
    for (wchar_t c : s) {
        if (prev == c) {
            if (++run >= 3) {
                has_elongation = true;
                break;
            }
        } else {
            run = 1;
            prev = c;
        }
    }
    if (!has_elongation) return {};

    std::wstring to_two;
    std::wstring to_one;
    to_two.reserve(s.length());
    to_one.reserve(s.length());
    run = 0;
    prev = 0;

    for (wchar_t c : s) {
        if (prev == c) {
            run++;
        } else {
            run = 1;
            prev = c;
        }
        if (run <= 2) to_two.push_back(c);
        if (run == 1) to_one.push_back(c);
    }

    if (to_two == to_one) return {to_one};
    return {to_two, to_one};
}

bool LayoutDetector::IsEmoticon(std::wstring_view raw) noexcept {
    size_t end = raw.length();
    while (end > 0 && (raw[end - 1] == L'.' || raw[end - 1] == L',' ||
                       raw[end - 1] == L'!' || raw[end - 1] == L'?' || raw[end - 1] == 0x2026)) {
        end--;
    }
    std::wstring_view s = raw.substr(0, end);
    if (s.empty()) return false;
    wchar_t eyes = s[0];
    s = s.substr(1);
    if (s.length() > 1 && (s[0] == L'-' || s[0] == L'~' || s[0] == L'^' || s[0] == L'\'')) {
        s = s.substr(1);
    }
    if (s.length() != 1) return false;
    wchar_t mouth = s[0];
    static const wchar_t mouthSymbols[] = L")(|/\\*$@[]{}3";

    auto has_mouth = [mouth]() {
        for (size_t i = 0; i < sizeof(mouthSymbols)/sizeof(mouthSymbols[0]) - 1; ++i) {
            if (mouth == mouthSymbols[i]) return true;
        }
        return false;
    };

    switch (eyes) {
        case L':':
        case L'=':
            return IsLatin(mouth) || IsCyrillic(mouth) || has_mouth();
        case L';':
        case L'8':
            return (mouth >= L'A' && mouth <= L'Z') || (mouth >= 0x0410 && mouth <= 0x042F) || has_mouth();
        case L'X':
        case L'x':
            return mouth == L'D' || mouth == L'd';
        default:
            return false;
    }
}

bool LayoutDetector::IsNumericToken(std::wstring_view s) noexcept {
    if (s.empty()) return false;
    bool has_digit = false;
    for (wchar_t ch : s) {
        if (IsAsciiDigit(ch)) { has_digit = true; continue; }
        if (ch == L',' || ch == L'.' || ch == L'-' || ch == 0x2013 || ch == L'/') continue;
        return false;
    }
    return has_digit;
}

bool LayoutDetector::IsImpossibleRussianSpelling(std::wstring_view w) noexcept {
    std::wstring lower = ToLower(w);
    if (lower.length() < 2) return false;

    // Check all identical chars
    bool all_same = true;
    for (size_t i = 1; i < lower.length(); ++i) {
        if (lower[i] != lower[0]) { all_same = false; break; }
    }
    if (all_same) return false;

    if (lower[0] == L'ь' || lower[0] == L'ы') return true;

    static const wchar_t vowels[] = L"аеёиоуыэюя";
    auto is_vowel = [](wchar_t c) {
        for (size_t i = 0; i < 10; ++i) if (c == vowels[i]) return true;
        return false;
    };

    for (size_t i = 0; i < lower.length() - 1; ++i) {
        wchar_t a = lower[i], b = lower[i + 1];
        if (a == b && (a == L'ь' || a == L'ы' || a == L'й' || a == L'щ')) return true;
        if (is_vowel(a) && (b == L'ь' || b == L'ы')) return true;
    }
    return false;
}

namespace {

template <size_t N = 64>
struct StackBuf {
    wchar_t buf[N];
    size_t len = 0;

    void LowerFrom(std::wstring_view s) noexcept {
        len = (std::min)(s.length(), N - 1);
        for (size_t i = 0; i < len; ++i) {
            buf[i] = ToLower(s[i]);
        }
        buf[len] = 0;
    }

    std::wstring_view view() const noexcept { return {buf, len}; }
    operator std::wstring_view() const noexcept { return {buf, len}; }
};

} // namespace

bool LayoutDetector::HasValidSourceBeforeTrailingPunctuation(std::wstring_view raw_core,
                                                             const TransparentStringSet& force_swap) const {
    StackBuf<64> whole_buf;
    std::wstring whole_heap;
    std::wstring_view whole;
    if (raw_core.length() < 64) {
        whole_buf.LowerFrom(raw_core);
        whole = whole_buf.view();
    } else {
        whole_heap = ToLower(raw_core);
        whole = whole_heap;
    }

    std::wstring_view semantic = Keymap::Core(whole);
    if (semantic == whole || semantic.length() < 2) return false;
    bool cyr = HasCyrillic(semantic);
    bool lat = HasLatin(semantic);
    if (cyr == lat) return false;

    if (Data::IsInForceSwapBuiltin(semantic) || force_swap.count(semantic) > 0) return true;
    if (lat && Data::IsInTechLatinTokens(semantic)) return true;
    if (lat && Data::IsInForceRuAmb(semantic)) return false;

    const Dawg& words = cyr ? words_ru_ : words_en_;
    if (words.Contains(semantic)) return true;

    auto de = DeElongated(semantic);
    for (const auto& dw : de) {
        if (words.Contains(dw)) return true;
    }
    return false;
}

SwapDecision LayoutDetector::MixedRescue(std::wstring_view raw) const {
    if (!HasCyrillic(raw) || !HasLatin(raw)) return SwapDecision::Keep();
    std::wstring_view core = LetterCore(raw);
    if (core.length() < 2) return SwapDecision::Keep();

    for (wchar_t c : core) {
        if (!IsLayoutLetter(c)) return SwapDecision::Keep();
    }

    std::wstring to_ru = Keymap::Instance().Convert(core, true);
    std::wstring to_en = Keymap::Instance().Convert(core, false);

    bool ru_ok = !HasLatin(to_ru) && words_ru_.Contains(ToLower(to_ru));
    bool en_ok = !HasCyrillic(to_en) && words_en_.Contains(ToLower(to_en));

    if (ru_ok && !en_ok) return SwapDecision::To(true);
    if (en_ok && !ru_ok) return SwapDecision::To(false);
    return SwapDecision::Keep();
}

bool LayoutDetector::RussianNJAfterLatinLabel(std::wstring_view word, std::wstring_view raw_core,
                                             std::wstring_view prev, std::wstring_view earlier) const {
    if (word != L"nj" || raw_core != word) return false;
    if (prev.empty() || prev.back() != L',') return false;
    if (earlier.empty() || !HasCyrillic(earlier) || HasLatin(earlier)) return false;

    std::wstring_view label = prev.substr(0, prev.length() - 1);
    if (label.empty()) return false;
    int upper_count = 0;
    for (wchar_t ch : label) {
        if (!IsLatin(ch)) return false;
        if (ch >= L'A' && ch <= L'Z') upper_count++;
    }
    return upper_count >= 2;
}

bool LayoutDetector::IsKnownExtension(std::wstring_view ext) noexcept {
    static constexpr std::wstring_view kExts[] = {
        L"7z", L"aac", L"apk", L"app", L"avi", L"bak", L"bat", L"bin", L"bmp", L"bz2",
        L"c", L"cfg", L"cmd", L"com", L"conf", L"cpp", L"cs", L"css", L"csv", L"dart",
        L"deb", L"dll", L"dmg", L"doc", L"docx", L"env", L"exe", L"flac", L"flv", L"gif",
        L"gz", L"h", L"hpp", L"htm", L"html", L"ico", L"ini", L"iso", L"java", L"jpeg",
        L"jpg", L"js", L"json", L"kt", L"less", L"lnk", L"log", L"lua", L"m4a", L"md",
        L"mkv", L"mov", L"mp3", L"mp4", L"msi", L"ogg", L"pdf", L"php", L"png", L"ppt",
        L"pptx", L"ps1", L"psd", L"py", L"rar", L"rb", L"rpm", L"rs", L"rtf", L"sass",
        L"scss", L"sh", L"sql", L"svg", L"swift", L"sys", L"tar", L"tex", L"tgz", L"tif",
        L"tiff", L"tmp", L"torrent", L"ts", L"tsv", L"txt", L"url", L"vbs", L"wav", L"webm",
        L"webp", L"wmv", L"xls", L"xlsx", L"xml", L"xz", L"yaml", L"yml", L"zip"
    };
    return std::binary_search(std::begin(kExts), std::end(kExts), ext);
}

static bool ShouldKeepToken(std::wstring_view token,
                            const TransparentStringSet& ignored,
                            const TransparentStringSet& learned) noexcept {
    if (token.empty()) return false;
    if (ignored.count(token) > 0 || learned.count(token) > 0) return true;
    if (Data::IsInDefaultKeep(token)) return true;
    if (LayoutDetector::IsKnownExtension(token)) return true;
    return false;
}

SwapDecision LayoutDetector::Decide(std::wstring_view raw,
                                    const TransparentStringSet& ignored,
                                    const TransparentStringSet& learned,
                                    const TransparentStringSet& force_swap,
                                    std::wstring_view prev,
                                    std::wstring_view earlier,
                                    bool after_caret_jump) const {
    ContextHint context = ContextOf(prev);
    std::wstring_view letter_core = LetterCore(raw);

    // 1. File extensions (e.g. .exe, file.exe, photo.png, C:\test\app.exe)
    size_t last_dot = raw.rfind(L'.');
    if (last_dot != std::wstring_view::npos && last_dot + 1 < raw.length()) {
        std::wstring_view ext_raw = raw.substr(last_dot + 1);
        StackBuf<32> ext_b;
        std::wstring_view ext;
        if (ext_raw.length() < 32) {
            ext_b.LowerFrom(ext_raw);
            ext = ext_b.view();
        } else {
            std::wstring ext_h = ToLower(ext_raw);
            ext = ext_h;
        }
        if (ShouldKeepToken(ext, ignored, learned)) {
            return SwapDecision::Keep();
        }
    }

    // 2. Stripped leading punctuation (e.g. .exe, /exe, -exe, _exe)
    std::wstring_view stripped = raw;
    while (!stripped.empty() && (stripped.front() == L'.' || stripped.front() == L'/' ||
                                stripped.front() == L'\\' || stripped.front() == L'-' ||
                                stripped.front() == L'_')) {
        stripped.remove_prefix(1);
    }
    if (!stripped.empty() && stripped.length() != raw.length()) {
        StackBuf<64> str_b;
        std::wstring str_h;
        std::wstring_view str_v;
        if (stripped.length() < 64) {
            str_b.LowerFrom(stripped);
            str_v = str_b.view();
        } else {
            str_h = ToLower(stripped);
            str_v = str_h;
        }
        if (ShouldKeepToken(str_v, ignored, learned)) {
            return SwapDecision::Keep();
        }
    }

    // 3. Literal token, letter core, and typed core
    StackBuf<64> lit_b, typ_b, who_b;
    std::wstring lit_h, typ_h, who_h;
    std::wstring_view literal, typed, whole;

    if (letter_core.length() < 64) {
        lit_b.LowerFrom(letter_core);
        literal = lit_b.view();
    } else { lit_h = ToLower(letter_core); literal = lit_h; }

    std::wstring_view core_v = Keymap::Core(letter_core);
    if (core_v.length() < 64) {
        typ_b.LowerFrom(core_v);
        typed = typ_b.view();
    } else { typ_h = ToLower(core_v); typed = typ_h; }

    if (raw.length() < 64) {
        who_b.LowerFrom(raw);
        whole = who_b.view();
    } else { who_h = ToLower(raw); whole = who_h; }

    if (ShouldKeepToken(literal, ignored, learned) ||
        ShouldKeepToken(typed, ignored, learned) ||
        ShouldKeepToken(whole, ignored, learned)) {
        return SwapDecision::Keep();
    }

    if (IsEmoticon(raw)) return SwapDecision::Keep();

    // Strip digits from ends
    std::wstring_view core_raw = letter_core;
    bool had_digits = false;
    for (wchar_t c : core_raw) {
        if (IsAsciiDigit(c)) { had_digits = true; break; }
    }
    if (had_digits) {
        size_t s = 0, e = core_raw.length();
        while (s < e && IsAsciiDigit(core_raw[s])) s++;
        while (e > s && IsAsciiDigit(core_raw[e - 1])) e--;
        core_raw = core_raw.substr(s, e - s);
    }

    StackBuf<64> w_stack;
    std::wstring w_heap;
    std::wstring_view w;
    if (core_raw.length() < 64) {
        w_stack.LowerFrom(core_raw);
        w = w_stack.view();
    } else {
        w_heap = ToLower(core_raw);
        w = w_heap;
    }

    // Hyphen tokens
    if (w.find(L'-') != std::wstring_view::npos) {
        bool cyr = HasCyrillic(w);
        bool lat = HasLatin(w);
        if (cyr != lat) {
            bool to_cyr = !cyr;
            std::wstring swapped_h = ToLower(Keymap::Instance().Convert(core_raw, to_cyr));
            if (Data::IsInHyphenTerms(w) || Data::IsInRuHyphenTerms(w)) return SwapDecision::Keep();
            if (Data::IsInHyphenTerms(swapped_h) || Data::IsInRuHyphenTerms(swapped_h)) return SwapDecision::To(to_cyr);
        }
        return SwapDecision::Keep();
    }

    if (w.length() < (had_digits ? 4 : 1)) return SwapDecision::Keep();
    for (wchar_t c : w) {
        if (!IsLayoutLetter(c)) return SwapDecision::Keep();
    }

    if (ignored.count(w) || learned.count(w) || Data::IsInDefaultKeep(w)) {
        return SwapDecision::Keep();
    }

    bool source_cyrillic = HasCyrillic(w);
    bool source_latin = HasLatin(w);
    if (source_cyrillic == source_latin) return SwapDecision::Keep();
    bool to_cyrillic = !source_cyrillic;

    StackBuf<64> swapped_stack;
    std::wstring swapped_heap;
    std::wstring_view swapped;
    if (core_raw.length() < 64) {
        swapped_stack.len = core_raw.length();
        for (size_t i = 0; i < core_raw.length(); ++i) {
            swapped_stack.buf[i] = ToLower(Keymap::Instance().ConvertChar(core_raw[i], to_cyrillic));
        }
        swapped_stack.buf[swapped_stack.len] = 0;
        swapped = swapped_stack.view();
    } else {
        swapped_heap = ToLower(Keymap::Instance().Convert(core_raw, to_cyrillic));
        swapped = swapped_heap;
    }

    // Trailing punctuation rule
    if (source_cyrillic && !words_ru_.Contains(w) && !swapped.empty() &&
        Keymap::IsTrailingPunctuation(swapped.back())) {
        size_t end = core_raw.length();
        while (end > 0) {
            wchar_t mapped = Keymap::Instance().ConvertChar(core_raw[end - 1], false);
            if (Keymap::IsTrailingPunctuation(mapped)) end--;
            else break;
        }
        if (end == 0) return SwapDecision::Keep();
        return Decide(core_raw.substr(0, end), ignored, learned, force_swap, prev, earlier, after_caret_jump);
    }

    if (swapped == w) return SwapDecision::Keep();

    // Check if swapped is a file extension starting with a dot (e.g. accidental юучу -> .exe)
    if (source_cyrillic && swapped.front() == L'.' && swapped.length() > 1) {
        std::wstring_view swapped_ext = swapped.substr(1);
        if (IsKnownExtension(swapped_ext) || ShouldKeepToken(swapped_ext, ignored, learned)) {
            bool all_lat = true;
            for (wchar_t c : swapped_ext) {
                if (!IsLatin(c)) { all_lat = false; break; }
            }
            if (all_lat && !words_ru_.Contains(w)) {
                return SwapDecision::To(false);
            }
        }
    }

    for (wchar_t c : swapped) {
        if (!IsLatin(c) && !IsCyrillic(c) && c != L'\'') return SwapDecision::Keep();
    }

    bool source_is_real_word = HasValidSourceBeforeTrailingPunctuation(core_raw, force_swap) ||
                               words_ru_.Contains(w) || words_en_.Contains(w);

    if (!source_is_real_word) {
        auto de = DeElongated(w);
        for (const auto& dw : de) {
            if (words_ru_.Contains(dw) || words_en_.Contains(dw)) {
                source_is_real_word = true;
                break;
            }
        }
    }

    if (Data::IsInForceSwapBuiltin(swapped) && !source_is_real_word) return SwapDecision::To(to_cyrillic);
    if (force_swap.count(swapped) > 0) return SwapDecision::To(to_cyrillic);
    if (Data::IsInForceSwapBuiltin(w) || force_swap.count(w) > 0) return SwapDecision::Keep();
    if (source_latin && Data::IsInTechLatinTokens(w)) return SwapDecision::Keep();

    if (w.length() >= 2 && source_is_real_word && !(source_latin && Data::IsInForceRuAmb(w))) {
        return SwapDecision::Keep();
    }

    // Single-letter words
    if (w.length() == 1) {
        if (after_caret_jump && context == ContextHint::None) return SwapDecision::Keep();
        static const wchar_t ru_single[] = L"авикосуя";
        auto is_ru_single = [](wchar_t c) {
            for (size_t i = 0; i < 8; ++i) if (c == ru_single[i]) return true;
            return false;
        };
        if (source_latin && is_ru_single(swapped[0])) {
            if (!prev.empty()) {
                StackBuf<64> prev_buf;
                std::wstring_view p;
                if (prev.length() < 64) {
                    prev_buf.LowerFrom(prev);
                    p = prev_buf.view();
                } else {
                    std::wstring p_heap = ToLower(prev);
                    p = p_heap;
                }
                if (context == ContextHint::Latin && Data::IsInLabelClassifiers(p)) return SwapDecision::Keep();
                if (context == ContextHint::Cyrillic && Data::IsInRuLabelClassifiers(p)) return SwapDecision::Keep();
            }
            return SwapDecision::To(true);
        }
        if (source_cyrillic && context != ContextHint::Cyrillic && (swapped == L"i" || swapped == L"u" || swapped == L"a")) {
            if (IsNumericToken(prev)) return SwapDecision::Keep();
            return SwapDecision::To(false);
        }
        return SwapDecision::Keep();
    }

    auto en_swap_not_junk = [&]() {
        if (w.length() == 2) return Data::IsInCommonEnTwoLetter(swapped);
        return w.length() >= 4 || trigrams_en_.Plausibility(swapped) > kShortEnSwapFloor;
    };

    // Dictionary decisions
    if (source_latin) {
        if (words_en_.Contains(w) && !Data::IsInForceRuAmb(w)) {
            if (context == ContextHint::Cyrillic && words_ru_.Contains(swapped) && !Data::IsInEnKeepShort(w)) {
                return SwapDecision::To(true);
            }
            return SwapDecision::Keep();
        }
        if (words_ru_.Contains(swapped)) {
            if (context == ContextHint::Latin && Data::IsInEnKeepShort(w) &&
                !RussianNJAfterLatinLabel(w, core_raw, prev, earlier)) {
                return SwapDecision::Keep();
            }
            return SwapDecision::To(true);
        }
    } else {
        if (Data::IsInForceEnAmb(swapped) && !words_ru_.Contains(w)) return SwapDecision::To(false);
        if (words_ru_.Contains(w)) {
            if (context == ContextHint::Latin && words_en_.Contains(swapped)) return SwapDecision::To(false);
            return SwapDecision::Keep();
        }
        if (words_en_.Contains(swapped) && en_swap_not_junk()) return SwapDecision::To(false);
    }

    // Trigram statistics (only for words >= 4 letters)
    if (w.length() < 4) return SwapDecision::Keep();

    if (source_cyrillic && !IsImpossibleRussianSpelling(w)) {
        bool has_lower = false;
        int letter_count = 0;
        for (wchar_t c : core_raw) {
            if (c >= 0x0430 && c <= 0x044F) has_lower = true;
            if (IsCyrillic(c)) letter_count++;
        }
        if (!has_lower && letter_count >= 2) return SwapDecision::Keep();
    }

    double orig_score = Plausibility(w, source_cyrillic);
    double swap_score = Plausibility(swapped, to_cyrillic);

    if (swap_score > orig_score + kMargin) return SwapDecision::To(to_cyrillic);
    if (orig_score <= -19.0 && swap_score > orig_score + 1.0) return SwapDecision::To(to_cyrillic);

    return SwapDecision::Keep();
}

} // namespace Ultimakey
