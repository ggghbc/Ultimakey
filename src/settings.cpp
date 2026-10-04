#include "settings.hpp"
#include <shlobj.h>
#include <fstream>
#include <sstream>

namespace Ultimakey {

Settings& Settings::Instance() {
    static Settings instance;
    return instance;
}

Settings::Settings() {
    SetDefaults();
}

void Settings::SetDefaults() {
    auto_enabled = true;
    trigger_space = true;
    trigger_enter = true;
    trigger_tab = false;
    enter_pre_convert = true;
    arrows_cancel = true;

    hotkey_vk = VK_PAUSE;
    hotkey_mods = 0;

    typofix_enabled = true;
    double_space_period = true;
    caps_remap_enabled = false;
    sound_enabled = true;
    autostart = false;
    write_log = true;

    app_modes = {
        {L"code", L"soft"},
        {L"idea64", L"soft"},
        {L"devenv", L"soft"},
        {L"pycharm64", L"soft"},
        {L"clion64", L"soft"},
        {L"webstorm64", L"soft"},
        {L"sublime_text", L"soft"},
        {L"notepad++", L"soft"},
        {L"cmd", L"off"},
        {L"powershell", L"off"},
        {L"windowsterminal", L"off"},
        {L"conhost", L"off"},
        {L"putty", L"off"},
        {L"mintty", L"off"},
        {L"bash", L"off"},
        {L"premiere", L"off"},
        {L"afterfx", L"off"},
        {L"davinci", L"off"}
    };

    static const wchar_t* const kDefaultIgnoredWords[] = {
        L"exe", L"dll", L"sys", L"com", L"bat", L"cmd", L"msi", L"ps1", L"vbs", L"sh", L"bin", L"iso",
        L"txt", L"pdf", L"doc", L"docx", L"xls", L"xlsx", L"ppt", L"pptx", L"rtf", L"csv", L"tsv", L"md",
        L"zip", L"rar", L"7z", L"tar", L"gz", L"bz2", L"xz", L"apk",
        L"png", L"jpg", L"jpeg", L"gif", L"bmp", L"webp", L"svg", L"ico", L"psd", L"tiff",
        L"mp3", L"wav", L"flac", L"ogg", L"aac", L"m4a", L"mp4", L"mkv", L"avi", L"mov", L"webm",
        L"c", L"cpp", L"h", L"hpp", L"cs", L"py", L"js", L"ts", L"rs", L"go", L"java", L"html", L"css",
        L"json", L"xml", L"yml", L"yaml", L"sql", L"php",
        L"ini", L"cfg", L"conf", L"log", L"env", L"torrent", L"url", L"lnk"
    };

    ignored_words.clear();
    for (const auto* w : kDefaultIgnoredWords) {
        ignored_words.insert(std::wstring(w));
    }
    learned_words.clear();
    force_swap_words.clear();
    snippets.clear();
}

std::wstring Settings::GetAppDataDirectory() {
    wchar_t path[MAX_PATH] = {};
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_APPDATA, nullptr, 0, path))) {
        std::wstring dir = std::wstring(path) + L"\\Ultimakey";
        CreateDirectoryW(dir.c_str(), nullptr);
        return dir;
    }
    return L".";
}

std::wstring Settings::GetSettingsFilePath() {
    return GetAppDataDirectory() + L"\\settings.json";
}

std::wstring Settings::GetAppModeString(std::wstring_view process_name) const {
    std::wstring lower = ToLower(process_name);
    // Strip .exe if present
    if (lower.length() > 4 && lower.substr(lower.length() - 4) == L".exe") {
        lower = lower.substr(0, lower.length() - 4);
    }
    auto it = app_modes.find(lower);
    if (it != app_modes.end()) {
        return it->second;
    }
    return L"default";
}

Ultimakey::AppMode Settings::GetAppMode(std::wstring_view process_name) const noexcept {
    wchar_t buf[64];
    std::wstring heap;
    std::wstring_view lower;
    if (process_name.length() < 64) {
        for (size_t i = 0; i < process_name.length(); ++i) buf[i] = ToLower(process_name[i]);
        lower = std::wstring_view(buf, process_name.length());
    } else {
        heap = ToLower(process_name);
        lower = heap;
    }
    if (lower.length() > 4 && lower.substr(lower.length() - 4) == L".exe") {
        lower = lower.substr(0, lower.length() - 4);
    }
    auto it = app_modes.find(lower);
    if (it != app_modes.end()) {
        if (it->second == L"soft") return AppMode::Soft;
        if (it->second == L"off") return AppMode::Off;
    }
    return AppMode::Default;
}

// -------------------------------------------------------------
// Lightweight JSON Parser and Serializer
// -------------------------------------------------------------

static std::string WideToUtf8(std::wstring_view w) {
    if (w.empty()) return {};
    int len = WideCharToMultiByte(CP_UTF8, 0, w.data(), static_cast<int>(w.length()), nullptr, 0, nullptr, nullptr);
    if (len <= 0) return {};
    std::string s(len, 0);
    WideCharToMultiByte(CP_UTF8, 0, w.data(), static_cast<int>(w.length()), s.data(), len, nullptr, nullptr);
    return s;
}

static std::wstring Utf8ToWide(std::string_view s) {
    if (s.empty()) return {};
    int len = MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.length()), nullptr, 0);
    if (len <= 0) return {};
    std::wstring w(len, 0);
    MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.length()), w.data(), len);
    return w;
}

static std::string EscapeJsonString(std::wstring_view ws) {
    std::string s = WideToUtf8(ws);
    std::string out = "\"";
    for (char c : s) {
        if (c == '\"') out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else if (c == '\t') out += "\\t";
        else out += c;
    }
    out += "\"";
    return out;
}

bool Settings::Save() const {
    std::wstring path = GetSettingsFilePath();
    std::ofstream f(path.c_str(), std::ios::out | std::ios::trunc | std::ios::binary);
    if (!f.is_open()) return false;

    std::ostringstream ss;
    ss << "{\n";
    ss << "  \"auto_enabled\": " << (auto_enabled ? "true" : "false") << ",\n";
    ss << "  \"trigger_space\": " << (trigger_space ? "true" : "false") << ",\n";
    ss << "  \"trigger_enter\": " << (trigger_enter ? "true" : "false") << ",\n";
    ss << "  \"trigger_tab\": " << (trigger_tab ? "true" : "false") << ",\n";
    ss << "  \"enter_pre_convert\": " << (enter_pre_convert ? "true" : "false") << ",\n";
    ss << "  \"arrows_cancel\": " << (arrows_cancel ? "true" : "false") << ",\n";
    ss << "  \"hotkey_vk\": " << hotkey_vk << ",\n";
    ss << "  \"hotkey_mods\": " << hotkey_mods << ",\n";
    ss << "  \"typofix_enabled\": " << (typofix_enabled ? "true" : "false") << ",\n";
    ss << "  \"double_space_period\": " << (double_space_period ? "true" : "false") << ",\n";
    ss << "  \"caps_remap_enabled\": " << (caps_remap_enabled ? "true" : "false") << ",\n";
    ss << "  \"sound_enabled\": " << (sound_enabled ? "true" : "false") << ",\n";
    ss << "  \"autostart\": " << (autostart ? "true" : "false") << ",\n";
    ss << "  \"write_log\": " << (write_log ? "true" : "false") << ",\n";

    // app_modes
    ss << "  \"app_modes\": {\n";
    bool first = true;
    for (const auto& [k, v] : app_modes) {
        if (!first) ss << ",\n";
        first = false;
        ss << "    " << EscapeJsonString(k) << ": " << EscapeJsonString(v);
    }
    ss << "\n  },\n";

    // ignored_words
    ss << "  \"ignored_words\": [\n";
    first = true;
    for (const auto& w : ignored_words) {
        if (!first) ss << ",\n";
        first = false;
        ss << "    " << EscapeJsonString(w);
    }
    ss << "\n  ],\n";

    // learned_words
    ss << "  \"learned_words\": [\n";
    first = true;
    for (const auto& w : learned_words) {
        if (!first) ss << ",\n";
        first = false;
        ss << "    " << EscapeJsonString(w);
    }
    ss << "\n  ],\n";

    // force_swap_words
    ss << "  \"force_swap_words\": [\n";
    first = true;
    for (const auto& w : force_swap_words) {
        if (!first) ss << ",\n";
        first = false;
        ss << "    " << EscapeJsonString(w);
    }
    ss << "\n  ],\n";

    // snippets
    ss << "  \"snippets\": [\n";
    first = true;
    for (const auto& [t, e] : snippets) {
        if (!first) ss << ",\n";
        first = false;
        ss << "    [" << EscapeJsonString(t) << ", " << EscapeJsonString(e) << "]";
    }
    ss << "\n  ]\n";

    ss << "}\n";

    std::string str = ss.str();
    f.write(str.data(), str.length());
    return true;
}

// Minimal JSON Parser
class SimpleJsonParser {
public:
    SimpleJsonParser(std::string_view content) : src_(content), pos_(0) {}

    void SkipWhitespace() {
        while (pos_ < src_.length()) {
            char c = src_[pos_];
            if (c == ' ' || c == '\t' || c == '\r' || c == '\n') pos_++;
            else if (c == '/' && pos_ + 1 < src_.length() && src_[pos_+1] == '/') {
                while (pos_ < src_.length() && src_[pos_] != '\n') pos_++;
            } else break;
        }
    }

    bool Match(char expected) {
        SkipWhitespace();
        if (pos_ < src_.length() && src_[pos_] == expected) {
            pos_++;
            return true;
        }
        return false;
    }

    std::string ParseString() {
        SkipWhitespace();
        if (!Match('\"')) return {};
        std::string res;
        while (pos_ < src_.length()) {
            char c = src_[pos_++];
            if (c == '\"') break;
            if (c == '\\' && pos_ < src_.length()) {
                char esc = src_[pos_++];
                if (esc == '\"') res += '\"';
                else if (esc == '\\') res += '\\';
                else if (esc == 'n') res += '\n';
                else if (esc == 'r') res += '\r';
                else if (esc == 't') res += '\t';
                else res += esc;
            } else {
                res += c;
            }
        }
        return res;
    }

    bool ParseBool(bool default_val) {
        SkipWhitespace();
        if (src_.substr(pos_, 4) == "true") { pos_ += 4; return true; }
        if (src_.substr(pos_, 5) == "false") { pos_ += 5; return false; }
        return default_val;
    }

    int ParseInt(int default_val) {
        SkipWhitespace();
        size_t start = pos_;
        if (pos_ < src_.length() && (src_[pos_] == '-' || src_[pos_] == '+')) pos_++;
        while (pos_ < src_.length() && (src_[pos_] >= '0' && src_[pos_] <= '9')) pos_++;
        if (pos_ > start) {
            return std::stoi(std::string(src_.substr(start, pos_ - start)));
        }
        return default_val;
    }

    void SkipValue() {
        SkipWhitespace();
        if (pos_ >= src_.length()) return;
        char c = src_[pos_];
        if (c == '\"') { ParseString(); }
        else if (c == '{') {
            Match('{');
            int depth = 1;
            while (pos_ < src_.length() && depth > 0) {
                if (src_[pos_] == '{') depth++;
                else if (src_[pos_] == '}') depth--;
                pos_++;
            }
        } else if (c == '[') {
            Match('[');
            int depth = 1;
            while (pos_ < src_.length() && depth > 0) {
                if (src_[pos_] == '[') depth++;
                else if (src_[pos_] == ']') depth--;
                pos_++;
            }
        } else {
            while (pos_ < src_.length() && src_[pos_] != ',' && src_[pos_] != '}' && src_[pos_] != ']') pos_++;
        }
    }

private:
    std::string_view src_;
    size_t pos_;
};

bool Settings::Load() {
    std::wstring path = GetSettingsFilePath();
    std::ifstream f(path.c_str(), std::ios::in | std::ios::binary);
    if (!f.is_open()) {
        Save(); // create default settings file
        return false;
    }

    std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    SimpleJsonParser p(content);

    if (!p.Match('{')) return false;

    while (true) {
        p.SkipWhitespace();
        if (p.Match('}')) break;

        std::string key = p.ParseString();
        if (!p.Match(':')) break;

        if (key == "auto_enabled") auto_enabled = p.ParseBool(auto_enabled);
        else if (key == "trigger_space") trigger_space = p.ParseBool(trigger_space);
        else if (key == "trigger_enter") trigger_enter = p.ParseBool(trigger_enter);
        else if (key == "trigger_tab") trigger_tab = p.ParseBool(trigger_tab);
        else if (key == "enter_pre_convert") enter_pre_convert = p.ParseBool(enter_pre_convert);
        else if (key == "arrows_cancel") arrows_cancel = p.ParseBool(arrows_cancel);
        else if (key == "hotkey_vk") hotkey_vk = p.ParseInt(hotkey_vk);
        else if (key == "hotkey_mods") hotkey_mods = p.ParseInt(hotkey_mods);
        else if (key == "typofix_enabled") typofix_enabled = p.ParseBool(typofix_enabled);
        else if (key == "double_space_period") double_space_period = p.ParseBool(double_space_period);
        else if (key == "caps_remap_enabled") caps_remap_enabled = p.ParseBool(caps_remap_enabled);
        else if (key == "sound_enabled") sound_enabled = p.ParseBool(sound_enabled);
        else if (key == "autostart") autostart = p.ParseBool(autostart);
        else if (key == "write_log") write_log = p.ParseBool(write_log);
        else if (key == "launch_count") { p.ParseInt(0); }
        else if (key == "app_modes") {
            if (p.Match('{')) {
                app_modes.clear();
                while (true) {
                    p.SkipWhitespace();
                    if (p.Match('}')) break;
                    std::string k = p.ParseString();
                    if (p.Match(':')) {
                        std::string v = p.ParseString();
                        app_modes[Utf8ToWide(k)] = Utf8ToWide(v);
                    }
                    p.Match(',');
                }
            }
        } else if (key == "ignored_words" || key == "learned_words" || key == "force_swap_words") {
            auto& target_set = (key == "ignored_words") ? ignored_words :
                               (key == "learned_words") ? learned_words : force_swap_words;
            if (p.Match('[')) {
                target_set.clear();
                while (true) {
                    p.SkipWhitespace();
                    if (p.Match(']')) break;
                    std::string item = p.ParseString();
                    if (!item.empty()) target_set.insert(Utf8ToWide(item));
                    p.Match(',');
                }
            }
        } else if (key == "snippets") {
            if (p.Match('[')) {
                snippets.clear();
                while (true) {
                    p.SkipWhitespace();
                    if (p.Match(']')) break;
                    if (p.Match('[')) {
                        std::string t = p.ParseString();
                        p.Match(',');
                        std::string e = p.ParseString();
                        p.Match(']');
                        snippets.emplace_back(Utf8ToWide(t), Utf8ToWide(e));
                    }
                    p.Match(',');
                }
            }
        } else {
            p.SkipValue();
        }

        p.Match(',');
    }

    if (ignored_words.empty()) {
        static const wchar_t* const kDefaults[] = {
            L"exe", L"dll", L"sys", L"com", L"bat", L"cmd", L"msi", L"ps1", L"vbs", L"sh", L"bin", L"iso",
            L"txt", L"pdf", L"doc", L"docx", L"xls", L"xlsx", L"ppt", L"pptx", L"rtf", L"csv", L"tsv", L"md",
            L"zip", L"rar", L"7z", L"tar", L"gz", L"bz2", L"xz", L"apk",
            L"png", L"jpg", L"jpeg", L"gif", L"bmp", L"webp", L"svg", L"ico", L"psd", L"tiff",
            L"mp3", L"wav", L"flac", L"ogg", L"aac", L"m4a", L"mp4", L"mkv", L"avi", L"mov", L"webm",
            L"c", L"cpp", L"h", L"hpp", L"cs", L"py", L"js", L"ts", L"rs", L"go", L"java", L"html", L"css",
            L"json", L"xml", L"yml", L"yaml", L"sql", L"php",
            L"ini", L"cfg", L"conf", L"log", L"env", L"torrent", L"url", L"lnk"
        };
        for (const auto* w : kDefaults) {
            ignored_words.insert(std::wstring(w));
        }
    }

    return true;
}

} // namespace Ultimakey
