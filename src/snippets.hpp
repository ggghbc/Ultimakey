#pragma once

#include "types.hpp"
#include <vector>
#include <string>
#include <string_view>
#include <optional>

namespace Ultimakey {

struct SnippetMatch {
    size_t trigger_length;
    std::wstring expansion;
};

class SnippetStore {
public:
    static SnippetStore& Instance();

    void SetSnippets(const std::vector<std::pair<std::wstring, std::wstring>>& pairs);
    const std::vector<std::pair<std::wstring, std::wstring>>& Pairs() const noexcept { return pairs_; }

    std::optional<SnippetMatch> FindMatch(std::wstring_view recent_text) const;
    std::optional<std::wstring> FindExpansion(std::wstring_view word) const;

    static std::wstring Canonical(std::wstring_view s);
    static bool EqualsIgnoreCase(std::wstring_view a, std::wstring_view b) noexcept;

private:
    SnippetStore() = default;

    struct IndexedSnippet {
        std::wstring trigger;
        std::wstring trigger_lower;
        std::wstring trigger_canon;
        std::wstring expansion;
    };

    std::vector<std::pair<std::wstring, std::wstring>> pairs_;
    std::vector<IndexedSnippet> indexed_;
};

} // namespace Ultimakey
