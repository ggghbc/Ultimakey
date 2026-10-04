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

void SnippetStore::SetSnippets(const std::vector<std::pair<std::wstring, std::wstring>>& pairs) {
    pairs_ = pairs;
    RebuildIndex();
}

void SnippetStore::RebuildIndex() {
    by_canonical_.clear();
    for (const auto& [trigger, expansion] : pairs_) {
        std::wstring c = Canonical(trigger);
        if (!c.empty()) {
            by_canonical_[c] = expansion;
        }
    }
}

std::optional<std::wstring> SnippetStore::FindExpansion(std::wstring_view word) const {
    if (word.empty() || by_canonical_.empty()) return std::nullopt;
    std::wstring c = Canonical(word);
    auto it = by_canonical_.find(c);
    if (it != by_canonical_.end()) {
        return it->second;
    }
    return std::nullopt;
}

} // namespace Ultimakey
