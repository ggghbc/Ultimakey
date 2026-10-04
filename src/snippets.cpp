#include "snippets.hpp"
#include "keymap.hpp"

namespace Ultimakey {

SnippetStore& SnippetStore::Instance() {
    static SnippetStore instance;
    return instance;
}

std::wstring SnippetStore::Canonical(std::wstring_view s) {
    std::wstring converted = Keymap::Instance().Convert(s, false);
    return ToLower(converted);
}

bool SnippetStore::EqualsIgnoreCase(std::wstring_view a, std::wstring_view b) noexcept {
    if (a.length() != b.length()) return false;
    for (size_t i = 0; i < a.length(); ++i) {
        if (ToLower(a[i]) != ToLower(b[i])) return false;
    }
    return true;
}

void SnippetStore::SetSnippets(const std::vector<std::pair<std::wstring, std::wstring>>& pairs) {
    pairs_ = pairs;
    indexed_.clear();
    for (const auto& [trig, exp] : pairs_) {
        if (!trig.empty()) {
            indexed_.push_back({trig, ToLower(trig), Canonical(trig), exp});
        }
    }
}

std::optional<SnippetMatch> SnippetStore::FindMatch(std::wstring_view recent) const {
    if (recent.empty() || indexed_.empty()) return std::nullopt;

    for (const auto& item : indexed_) {
        const size_t tlen = item.trigger.length();
        if (recent.length() < tlen) continue;

        std::wstring_view suffix = recent.substr(recent.length() - tlen);

        // 1. Direct case-insensitive match
        if (EqualsIgnoreCase(suffix, item.trigger)) {
            return SnippetMatch{tlen, item.expansion};
        }

        // 2. Cross-layout canonical match (user typed trigger in opposite layout)
        if (Canonical(suffix) == item.trigger_canon) {
            return SnippetMatch{tlen, item.expansion};
        }
    }
    return std::nullopt;
}

std::optional<std::wstring> SnippetStore::FindExpansion(std::wstring_view word) const {
    auto m = FindMatch(word);
    if (m.has_value() && m->trigger_length == word.length()) {
        return m->expansion;
    }
    return std::nullopt;
}

} // namespace Ultimakey
