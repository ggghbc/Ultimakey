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
        session_words_.push_back({current_word_, std::wstring(ws)});
        if (session_words_.size() > 8) {
            session_words_.erase(session_words_.begin());
        }
        current_word_.clear();
        current_word_gap_ = 0.0;
    } else if (!last_word_.empty()) {
        last_tail_.append(ws);
        if (!session_words_.empty()) {
            session_words_.back().tail.append(ws);
        }
    }
    last_activity_ = NowSeconds();
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
    if (!session_words_.empty()) {
        session_words_.back().word = converted;
    }
}

void KeystrokeBuffer::SoftContextReset() {
    session_words_.clear();
}

void KeystrokeBuffer::Clear() {
    current_word_.clear();
    last_word_.clear();
    last_tail_.clear();
    session_words_.clear();
    current_word_gap_ = 0.0;
    last_word_gap_ = 0.0;
    last_activity_ = NowSeconds();
}

std::wstring KeystrokeBuffer::ContextWord(bool context_for_current) const {
    if (context_for_current) {
        return last_word_;
    }
    if (session_words_.size() >= 2) {
        return session_words_[session_words_.size() - 2].word;
    }
    return L"";
}

std::wstring KeystrokeBuffer::EarlierContextWord(bool context_for_current) const {
    size_t need = context_for_current ? 2 : 3;
    if (session_words_.size() >= need) {
        return session_words_[session_words_.size() - need].word;
    }
    return L"";
}

} // namespace Ultimakey
