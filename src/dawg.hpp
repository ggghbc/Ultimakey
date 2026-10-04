#pragma once

#include "types.hpp"
#include <array>
#include <cstring>

namespace Ultimakey {

class Dawg {
public:
    Dawg() = default;

    bool LoadFromMemory(const void* data, size_t size) {
        if (!data || size < 12) return false;
        const uint8_t* bytes = static_cast<const uint8_t*>(data);
        if (std::memcmp(bytes, "DAWG", 4) != 0) return false;

        uint16_t alpha_len = *reinterpret_cast<const uint16_t*>(bytes + 4);
        root_offset_ = *reinterpret_cast<const uint16_t*>(bytes + 6);

        const wchar_t* alpha = reinterpret_cast<const wchar_t*>(bytes + 8);
        char_map_.fill(0xFF);
        for (uint16_t i = 0; i < alpha_len; ++i) {
            char_map_[static_cast<uint16_t>(alpha[i])] = static_cast<uint8_t>(i);
        }

        size_t trans_offset = 8 + alpha_len * sizeof(wchar_t);
        if (trans_offset + 4 > size) return false;
        num_transitions_ = *reinterpret_cast<const uint32_t*>(bytes + trans_offset);
        transitions_ = reinterpret_cast<const uint32_t*>(bytes + trans_offset + 4);

        is_loaded_ = true;
        return true;
    }

    bool Contains(std::wstring_view word) const noexcept {
        if (!is_loaded_ || word.empty()) return false;
        uint32_t node = root_offset_;
        const size_t len = word.length();
        for (size_t i = 0; i < len; ++i) {
            uint8_t cid = char_map_[static_cast<uint16_t>(word[i])];
            if (cid == 0xFF) return false;

            bool found = false;
            uint32_t edge_idx = node;
            while (true) {
                uint32_t entry = transitions_[edge_idx];
                uint8_t edge_char = static_cast<uint8_t>((entry >> 26) & 0x3F);
                bool is_final = (entry & (1u << 25)) != 0;
                bool is_last = (entry & (1u << 24)) != 0;
                uint32_t next_node = entry & 0x00FFFFFF;

                if (edge_char == cid) {
                    if (i == len - 1) {
                        return is_final;
                    }
                    node = next_node;
                    found = true;
                    break;
                }
                if (is_last) break;
                edge_idx++;
            }
            if (!found) return false;
        }
        return false;
    }

    bool IsLoaded() const noexcept { return is_loaded_; }

private:
    bool is_loaded_ = false;
    uint16_t root_offset_ = 0;
    uint32_t num_transitions_ = 0;
    const uint32_t* transitions_ = nullptr;
    std::array<uint8_t, 65536> char_map_{};
};

} // namespace Ultimakey
