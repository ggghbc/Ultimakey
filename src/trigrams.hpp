#pragma once

#include "types.hpp"
#include <cstring>
#include <algorithm>
#include <array>

namespace Ultimakey {

class TrigramTable {
public:
    TrigramTable() = default;

    bool LoadFromMemory(const void* data, size_t size) {
        if (!data || size < 10) return false;
        const uint8_t* bytes = static_cast<const uint8_t*>(data);
        if (std::memcmp(bytes, "TRG2", 4) != 0) return false;

        uint16_t alpha_len = *reinterpret_cast<const uint16_t*>(bytes + 4);
        if (6 + alpha_len * sizeof(wchar_t) + 4 > size) return false;

        const wchar_t* alpha = reinterpret_cast<const wchar_t*>(bytes + 6);
        ascii_map_.fill(0xFF);
        cyr_map_.fill(0xFF);

        for (uint16_t i = 0; i < alpha_len; ++i) {
            wchar_t c = alpha[i];
            if (c < 128) {
                ascii_map_[static_cast<uint8_t>(c)] = static_cast<uint8_t>(i);
            } else if (c >= 0x0400 && c <= 0x045F) {
                cyr_map_[c - 0x0400] = static_cast<uint8_t>(i);
            }
        }

        size_t offset = 6 + alpha_len * sizeof(wchar_t);
        count_ = *reinterpret_cast<const uint32_t*>(bytes + offset);
        offset += 4;

        if (offset + count_ * sizeof(uint32_t) > size) return false;
        entries_ = reinterpret_cast<const uint32_t*>(bytes + offset);

        is_loaded_ = true;
        return true;
    }

    inline uint8_t GetCharId(wchar_t c) const noexcept {
        if (c < 128) return ascii_map_[c];
        if (c >= 0x0400 && c <= 0x045F) return cyr_map_[c - 0x0400];
        return 0xFF;
    }

    inline float ScoreByIds(uint8_t id0, uint8_t id1, uint8_t id2) const noexcept {
        if (!is_loaded_ || count_ == 0 || id0 == 0xFF || id1 == 0xFF || id2 == 0xFF) return floor_val_;

        uint32_t target_prefix = (static_cast<uint32_t>(id0) << 12) |
                                 (static_cast<uint32_t>(id1) << 6) |
                                 static_cast<uint32_t>(id2);
        uint32_t target_key = target_prefix << 14;

        const uint32_t* first = entries_;
        const uint32_t* last = entries_ + count_;

        const uint32_t* it = std::lower_bound(first, last, target_key);

        if (it != last && (*it >> 14) == target_prefix) {
            uint16_t mag = *it & 0x3FFF;
            return -static_cast<float>(mag) / 1000.0f;
        }

        return floor_val_;
    }

    float GetScore(wchar_t c0, wchar_t c1, wchar_t c2) const noexcept {
        return ScoreByIds(GetCharId(c0), GetCharId(c1), GetCharId(c2));
    }

    double Plausibility(std::wstring_view word) const noexcept {
        if (!is_loaded_ || word.empty() || count_ == 0) return 0.0;

        const size_t len = word.length();
        uint8_t stack_ids[66];
        uint8_t* ids = stack_ids;
        std::vector<uint8_t> heap_ids;
        if (len + 2 > sizeof(stack_ids)) {
            heap_ids.resize(len + 2);
            ids = heap_ids.data();
        }

        uint8_t space_id = GetCharId(L' ');
        ids[0] = space_id;
        for (size_t i = 0; i < len; ++i) {
            ids[i + 1] = GetCharId(word[i]);
        }
        ids[len + 1] = space_id;

        double total = 0.0;
        for (size_t i = 0; i < len; ++i) {
            total += ScoreByIds(ids[i], ids[i + 1], ids[i + 2]);
        }
        return total;
    }

    bool IsLoaded() const noexcept { return is_loaded_; }
    void SetFloor(float floor_val) noexcept { floor_val_ = floor_val; }

private:
    bool is_loaded_ = false;
    uint32_t count_ = 0;
    const uint32_t* entries_ = nullptr;
    float floor_val_ = -16.0f;
    std::array<uint8_t, 128> ascii_map_{};
    std::array<uint8_t, 96> cyr_map_{};
};

} // namespace Ultimakey
