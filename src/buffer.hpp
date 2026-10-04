#pragma once

#include "types.hpp"
#include <string>
#include <vector>
#include <optional>

namespace Ultimakey {

struct ConversionTarget {
    std::wstring word;
    int delete_count;
    std::wstring tail;
};

class KeystrokeBuffer {
public:
    KeystrokeBuffer();

    void Append(wchar_t c);
    void Append(std::wstring_view str);
    void Backspace();
    void Boundary(std::wstring_view ws);

    std::optional<ConversionTarget> WordForConversion(bool completed_only);
    void ApplyConversion(std::wstring_view converted);
    void ApplyCompletedConversion(std::wstring_view converted);

    void SoftContextReset();
    void Clear();

    std::wstring ContextWord(bool context_for_current) const;
    std::wstring EarlierContextWord(bool context_for_current) const;

    const std::wstring& CurrentWord() const noexcept { return current_word_; }
    const std::wstring& LastWord() const noexcept { return last_word_; }
    const std::wstring& LastTail() const noexcept { return last_tail_; }

    double CurrentWordGap() const noexcept { return current_word_gap_; }
    double LastWordGap() const noexcept { return last_word_gap_; }

    void ClearSessionWords() { session_words_.clear(); }

private:
    std::wstring current_word_;
    std::wstring last_word_;
    std::wstring last_tail_;

    struct HistoryItem {
        std::wstring word;
        std::wstring tail;
    };
    std::vector<HistoryItem> session_words_;

    double current_word_gap_ = 0.0;
    double last_word_gap_ = 0.0;
    double last_activity_ = 0.0;
};

} // namespace Ultimakey
