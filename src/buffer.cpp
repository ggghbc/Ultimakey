#include "buffer.hpp"
#include <algorithm>

namespace Ultimakey {

KeystrokeBuffer::KeystrokeBuffer() {
    last_activity_ = NowSeconds();
}

void KeystrokeBuffer::Append(wchar_t c) {
    double now = NowSeconds();
    double gap = now - last_activity_;
    last_activity_ = now;

    if (current_word_.empty()) {
        current_word_gap_ = 0.0;
    } else {
        current_word_gap_ = (std::max)(current_word_gap_, gap);
    }
    current_word_.push_back(c);
}

void KeystrokeBuffer::Append(std::wstring_view str) {
    for (wchar_t c : str) {
        Append(c);
    }
}

void KeystrokeBuffer::Backspace() {
    last_activity_ = NowSeconds();
    last_boundary_time_ = 0.0;
    if (!current_word_.empty()) {
        current_word_.pop_back();
    } else if (!last_tail_.empty()) {
        last_tail_.pop_back();
    } else if (!last_word_.empty()) {
        current_word_ = last_word_;
        last_word_.clear();
        if (!current_word_.empty()) {
            current_word_.pop_back();
        }
    }
}

void KeystrokeBuffer::Boundary(std::wstring_view ws) {
    if (!current_word_.empty()) {
        last_word_ = current_word_;
        last_word_gap_ = current_word_gap_;
        last_tail_ = ws;
        session_words_[session_head_].word = current_word_;
        session_words_[session_head_].tail = ws;
        session_head_ = (session_head_ + 1) % kHistoryCap;
        if (session_count_ < kHistoryCap) {
            session_count_++;
        }
        current_word_.clear();
        current_word_gap_ = 0.0;
    } else if (!last_word_.empty()) {
        last_tail_.append(ws);
        if (session_count_ > 0) {
            size_t latest_idx = (session_head_ + kHistoryCap - 1) % kHistoryCap;
            session_words_[latest_idx].tail.append(ws);
        }
    }
    double now = NowSeconds();
    last_activity_ = now;
    last_boundary_time_ = now;
}

std::optional<ConversionTarget> KeystrokeBuffer::WordForConversion(bool completed_only) {
    if (!completed_only && !current_word_.empty()) {
        return ConversionTarget{current_word_, static_cast<int>(current_word_.length()), std::wstring()};
    }
    if (!last_word_.empty()) {
        int del_count = static_cast<int>(last_word_.length() + last_tail_.length());
        return ConversionTarget{last_word_, del_count, last_tail_};
    }
    return std::nullopt;
}

void KeystrokeBuffer::ApplyConversion(std::wstring_view converted) {
    current_word_ = converted;
}

void KeystrokeBuffer::ApplyCompletedConversion(std::wstring_view converted) {
    last_word_ = converted;
    if (session_count_ > 0) {
        size_t latest_idx = (session_head_ + kHistoryCap - 1) % kHistoryCap;
        session_words_[latest_idx].word = converted;
    }
}

void KeystrokeBuffer::ApplyDoubleSpacePeriod() noexcept {
    last_tail_ = L". ";
    last_boundary_time_ = 0.0;
}

void KeystrokeBuffer::SoftContextReset() {
    session_head_ = 0;
    session_count_ = 0;
    last_boundary_time_ = 0.0;
}

void KeystrokeBuffer::Clear() {
    current_word_.clear();
    last_word_.clear();
    last_tail_.clear();
    session_head_ = 0;
    session_count_ = 0;
    current_word_gap_ = 0.0;
    last_word_gap_ = 0.0;
    last_boundary_time_ = 0.0;
    last_activity_ = NowSeconds();
}

std::wstring_view KeystrokeBuffer::ContextWord(bool context_for_current) const noexcept {
    if (context_for_current) {
        return last_word_;
    }
    if (session_count_ >= 2) {
        size_t idx = (session_head_ + kHistoryCap - 2) % kHistoryCap;
        return session_words_[idx].word;
    }
    return {};
}

std::wstring_view KeystrokeBuffer::EarlierContextWord(bool context_for_current) const noexcept {
    size_t need = context_for_current ? 2 : 3;
    if (session_count_ >= need) {
        size_t idx = (session_head_ + kHistoryCap - need) % kHistoryCap;
        return session_words_[idx].word;
    }
    return {};
}

std::wstring KeystrokeBuffer::RecentText() const {
    std::wstring result;
    result.reserve(128);
    if (session_count_ > 0) {
        size_t start = (session_head_ + kHistoryCap - session_count_) % kHistoryCap;
        for (size_t i = 0; i < session_count_; ++i) {
            size_t idx = (start + i) % kHistoryCap;
            result.append(session_words_[idx].word);
            result.append(session_words_[idx].tail);
        }
    }
    result.append(current_word_);
    return result;
}

void KeystrokeBuffer::OnSnippetReplaced() noexcept {
    Clear();
}

} // namespace Ultimakey
