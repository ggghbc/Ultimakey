#pragma once

#include "types.hpp"
#include <string>
#include <string_view>
#include <array>
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

    std::wstring_view ContextWord(bool context_for_current) const noexcept;
    std::wstring_view EarlierContextWord(bool context_for_current) const noexcept;

    const std::wstring& CurrentWord() const noexcept { return current_word_; }
    const std::wstring& LastWord() const noexcept { return last_word_; }
    const std::wstring& LastTail() const noexcept { return last_tail_; }

    double CurrentWordGap() const noexcept { return current_word_gap_; }
    double LastWordGap() const noexcept { return last_word_gap_; }
    double LastBoundaryTime() const noexcept { return last_boundary_time_; }
    void ApplyDoubleSpacePeriod() noexcept;

    void ClearSessionWords() noexcept {
        session_head_ = 0;
        session_count_ = 0;
    }

private:
    std::wstring current_word_;
    std::wstring last_word_;
    std::wstring last_tail_;

    struct HistoryItem {
        std::wstring word;
        std::wstring tail;
    };
    static constexpr size_t kHistoryCap = 8;
    std::array<HistoryItem, kHistoryCap> session_words_{};
    size_t session_head_ = 0;
    size_t session_count_ = 0;

    double current_word_gap_ = 0.0;
    double last_word_gap_ = 0.0;
    double last_boundary_time_ = 0.0;
    double last_activity_ = 0.0;
};

} // namespace Ultimakey
