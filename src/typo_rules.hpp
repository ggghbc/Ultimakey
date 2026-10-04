#pragma once

#include "types.hpp"
#include <vector>
#include <string_view>
#include <cstring>
#include <algorithm>

namespace Ultimakey {

class TypoRules {
public:
    TypoRules() = default;

    bool LoadFromMemory(const void* data, size_t size) {
        if (!data || size < 8) return false;
        const uint8_t* bytes = static_cast<const uint8_t*>(data);
        if (std::memcmp(bytes, "TYPO", 4) != 0) return false;

        uint32_t count = *reinterpret_cast<const uint32_t*>(bytes + 4);
        entries_.clear();
        entries_.reserve(count);

        size_t offset = 8;
        for (uint32_t i = 0; i < count; ++i) {
            if (offset + 2 > size) return false;
            uint16_t typo_len = *reinterpret_cast<const uint16_t*>(bytes + offset);
            offset += 2;

            if (offset + typo_len * sizeof(wchar_t) + 2 > size) return false;
            const wchar_t* typo_str = reinterpret_cast<const wchar_t*>(bytes + offset);
            offset += typo_len * sizeof(wchar_t);

            uint16_t fix_len = *reinterpret_cast<const uint16_t*>(bytes + offset);
            offset += 2;

            if (offset + fix_len * sizeof(wchar_t) > size) return false;
            const wchar_t* fix_str = reinterpret_cast<const wchar_t*>(bytes + offset);
            offset += fix_len * sizeof(wchar_t);

            entries_.emplace_back(std::wstring_view(typo_str, typo_len),
                                  std::wstring_view(fix_str, fix_len));
        }

        is_loaded_ = true;
        return true;
    }

    std::wstring_view FindCorrection(std::wstring_view typo) const noexcept {
        if (!is_loaded_ || entries_.empty() || typo.empty()) return {};

        auto it = std::lower_bound(entries_.begin(), entries_.end(), typo,
            [](const std::pair<std::wstring_view, std::wstring_view>& entry, std::wstring_view val) {
                return entry.first < val;
            });

        if (it != entries_.end() && it->first == typo) {
            return it->second;
        }
        return {};
    }

    bool IsLoaded() const noexcept { return is_loaded_; }

private:
    bool is_loaded_ = false;
    std::vector<std::pair<std::wstring_view, std::wstring_view>> entries_;
};

} // namespace Ultimakey
