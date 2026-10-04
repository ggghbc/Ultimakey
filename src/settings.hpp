#pragma once

#include "types.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

namespace Ultimakey {

struct Settings {
    static constexpr int ModCtrl  = 1;
    static constexpr int ModAlt   = 2;
    static constexpr int ModShift = 4;
    static constexpr int ModWin   = 8;

    bool auto_enabled = true;
    bool trigger_space = true;
    bool trigger_enter = true;
    bool trigger_tab = false;
    bool enter_pre_convert = true;
    bool arrows_cancel = true;

    int hotkey_vk = VK_PAUSE;
    int hotkey_mods = 0;

    bool typofix_enabled = true;
    bool double_space_period = true;
    bool caps_remap_enabled = false;
    bool sound_enabled = true;
    bool autostart = false;
    bool write_log = true;
    int launch_count = 0;

    std::unordered_map<std::wstring, std::wstring> app_modes;
    TransparentStringSet ignored_words;
    TransparentStringSet learned_words;
    TransparentStringSet force_swap_words;
    std::vector<std::pair<std::wstring, std::wstring>> snippets;

    static Settings& Instance();

    bool Load();
    bool Save() const;

    std::wstring GetAppModeString(std::wstring_view process_name) const;
    Ultimakey::AppMode GetAppMode(std::wstring_view process_name) const noexcept;
    static std::wstring GetSettingsFilePath();
    static std::wstring GetAppDataDirectory();

private:
    Settings();
    void SetDefaults();
};

} // namespace Ultimakey
