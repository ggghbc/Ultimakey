#pragma once

#include "types.hpp"
#include <vector>
#include <string>
#include <unordered_map>
#include <optional>

namespace Ultimakey {

class SnippetStore {
public:
    static SnippetStore& Instance();

    void SetSnippets(const std::vector<std::pair<std::wstring, std::wstring>>& pairs);
    const std::vector<std::pair<std::wstring, std::wstring>>& Pairs() const noexcept { return pairs_; }

    std::optional<std::wstring> FindExpansion(std::wstring_view word) const;

    static std::wstring Canonical(std::wstring_view s);

private:
    SnippetStore() = default;
    void RebuildIndex();

    std::vector<std::pair<std::wstring, std::wstring>> pairs_;
    std::unordered_map<std::wstring, std::wstring> by_canonical_;
};

} // namespace Ultimakey
