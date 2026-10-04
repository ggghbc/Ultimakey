#pragma once

#include "types.hpp"
#include <cstring>
#include <algorithm>

namespace Ultimakey {

#pragma pack(push, 1)
struct TrigramEntry {
    wchar_t c0;
    wchar_t c1;
    wchar_t c2;
    int16_t pad;
    float score;
};
#pragma pack(pop)

static_assert(sizeof(TrigramEntry) == 12, "TrigramEntry must be 12 bytes");

class TrigramTable {
public:
    TrigramTable() = default;

    bool LoadFromMemory(const void* data, size_t size) {
        if (!data || size < 8) return false;
        const uint8_t* bytes = static_cast<const uint8_t*>(data);
        if (std::memcmp(bytes, "TRIG", 4) != 0) return false;

        count_ = *reinterpret_cast<const uint32_t*>(bytes + 4);
        if (8 + count_ * sizeof(TrigramEntry) > size) return false;

        entries_ = reinterpret_cast<const TrigramEntry*>(bytes + 8);
        is_loaded_ = true;
        return true;
    }

    float GetScore(wchar_t c0, wchar_t c1, wchar_t c2) const noexcept {
        if (!is_loaded_ || count_ == 0) return floor_val_;

        const TrigramEntry* first = entries_;
        const TrigramEntry* last = entries_ + count_;

        auto comp = [](const TrigramEntry& e, const uint64_t target) {
            uint64_t e_chars = (static_cast<uint64_t>(e.c0) << 32) |
                               (static_cast<uint64_t>(e.c1) << 16) |
                               static_cast<uint64_t>(e.c2);
            return e_chars < target;
        };

        uint64_t target = (static_cast<uint64_t>(c0) << 32) |
                          (static_cast<uint64_t>(c1) << 16) |
                          static_cast<uint64_t>(c2);

        const TrigramEntry* it = std::lower_bound(first, last, target, comp);
        if (it != last && it->c0 == c0 && it->c1 == c1 && it->c2 == c2) {
            return it->score;
        }
        return floor_val_;
    }

    double Plausibility(std::wstring_view word) const noexcept {
        if (!is_loaded_ || word.empty()) return 0.0;

        // Padded representation: L" " + word + L" "
        // Number of trigrams = word.length() + 2 - 3 + 1 = word.length()
        const size_t len = word.length();
        double total = 0.0;

        for (size_t i = 0; i < len; ++i) {
            wchar_t c0 = (i == 0) ? L' ' : word[i - 1];
            wchar_t c1 = word[i];
            wchar_t c2 = (i + 1 < len) ? word[i + 1] : L' ';
            total += GetScore(c0, c1, c2);
        }
        return total;
    }

    bool IsLoaded() const noexcept { return is_loaded_; }
    void SetFloor(float floor_val) noexcept { floor_val_ = floor_val; }

private:
    bool is_loaded_ = false;
    uint32_t count_ = 0;
    const TrigramEntry* entries_ = nullptr;
    float floor_val_ = -20.0f;
};

} // namespace Ultimakey
