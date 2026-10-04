#pragma once

#include "types.hpp"
#include "dawg.hpp"
#include "trigrams.hpp"
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace Ultimakey {

struct SwapDecision {
    bool convert;
    bool to_cyrillic;

    static constexpr SwapDecision Keep() noexcept { return {false, false}; }
    static constexpr SwapDecision To(bool to_cyr) noexcept { return {true, to_cyr}; }
};

enum class ContextHint {
    None,
    Cyrillic,
    Latin
};

class LayoutDetector {
public:
    LayoutDetector(const Dawg& words_ru, const Dawg& words_en,
                   const TrigramTable& trigrams_ru, const TrigramTable& trigrams_en);

    SwapDecision Decide(std::wstring_view raw,
                        const TransparentStringSet& ignored,
                        const TransparentStringSet& learned,
                        const TransparentStringSet& force_swap,
                        std::wstring_view prev = L"",
                        std::wstring_view earlier = L"",
                        bool after_caret_jump = false) const;

    SwapDecision MixedRescue(std::wstring_view raw) const;

    static bool IsEmoticon(std::wstring_view raw) noexcept;
    static bool IsNumericToken(std::wstring_view s) noexcept;
    static bool IsImpossibleRussianSpelling(std::wstring_view w) noexcept;
    static bool IsLayoutLetter(wchar_t c) noexcept;
    static std::wstring_view LetterCore(std::wstring_view raw) noexcept;
    static std::vector<std::wstring> DeElongated(std::wstring_view s);

    static ContextHint ContextOf(std::wstring_view word) noexcept;

    double Plausibility(std::wstring_view word, bool cyrillic) const noexcept;
    bool WordExistsInSource(std::wstring_view w, bool cyrillic) const noexcept;

private:
    const Dawg& words_ru_;
    const Dawg& words_en_;
    const TrigramTable& trigrams_ru_;
    const TrigramTable& trigrams_en_;

    bool HasValidSourceBeforeTrailingPunctuation(std::wstring_view raw_core,
                                                 const TransparentStringSet& force_swap) const;
    bool RussianNJAfterLatinLabel(std::wstring_view word, std::wstring_view raw_core,
                                  std::wstring_view prev, std::wstring_view earlier) const;
};

} // namespace Ultimakey
