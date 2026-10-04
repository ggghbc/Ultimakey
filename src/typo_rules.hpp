#pragma once

#include "types.hpp"
#include <string_view>
#include <cstring>
#include <algorithm>

namespace Ultimakey {

#pragma pack(push, 1)
struct TypoIndex {
    uint32_t typo_offset; // in wchar_t
    uint16_t typo_len;
    uint32_t fix_offset;  // in wchar_t
    uint16_t fix_len;
};
#pragma pack(pop)

static_assert(sizeof(TypoIndex) == 12, "TypoIndex must be 12 bytes");

class TypoRules {
public:
    TypoRules() = default;

    bool LoadFromMemory(const void* data, size_t size) {
        if (!data || size < 8) return false;
        const uint8_t* bytes = static_cast<const uint8_t*>(data);
        if (std::memcmp(bytes, "TYP2", 4) != 0) return false;

        count_ = *reinterpret_cast<const uint32_t*>(bytes + 4);
        size_t index_bytes = count_ * sizeof(TypoIndex);
        if (8 + index_bytes > size) return false;

        entries_ = reinterpret_cast<const TypoIndex*>(bytes + 8);
        pool_ = reinterpret_cast<const wchar_t*>(bytes + 8 + index_bytes);

        is_loaded_ = true;
        return true;
    }

    std::wstring_view FindCorrection(std::wstring_view typo) const noexcept {
        if (!is_loaded_ || count_ == 0 || typo.empty()) return {};

        const TypoIndex* first = entries_;
        const TypoIndex* last = entries_ + count_;

        auto comp = [this](const TypoIndex& idx, std::wstring_view target) {
            std::wstring_view sv(pool_ + idx.typo_offset, idx.typo_len);
            return sv < target;
        };

        const TypoIndex* it = std::lower_bound(first, last, typo, comp);
        if (it != last) {
            std::wstring_view sv(pool_ + it->typo_offset, it->typo_len);
            if (sv == typo) {
                return std::wstring_view(pool_ + it->fix_offset, it->fix_len);
            }
        }
        return {};
    }

    bool IsLoaded() const noexcept { return is_loaded_; }

private:
    bool is_loaded_ = false;
    uint32_t count_ = 0;
    const TypoIndex* entries_ = nullptr;
    const wchar_t* pool_ = nullptr;
};

} // namespace Ultimakey
