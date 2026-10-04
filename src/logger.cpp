#include "logger.hpp"
#include "settings.hpp"
#include <fstream>
#include <mutex>
#include <ctime>
#include <iomanip>

namespace Ultimakey {

static std::mutex g_log_mutex;

Logger& Logger::Instance() {
    static Logger instance;
    return instance;
}

Logger::Logger() {
    log_path_ = Settings::GetAppDataDirectory() + L"\\ultimakey.log";
}

Logger::~Logger() = default;

std::wstring Logger::GetLogFilePath() const {
    return log_path_;
}

void Logger::Write(std::string_view msg) {
    if (!Settings::Instance().write_log) return;

    std::lock_guard<std::mutex> lock(g_log_mutex);
    bool need_bom = false;
    {
        std::ifstream check(log_path_.c_str(), std::ios::binary | std::ios::ate);
        if (!check.is_open() || check.tellg() == 0) {
            need_bom = true;
        }
    }

    std::ofstream f(log_path_.c_str(), std::ios::out | std::ios::app | std::ios::binary);
    if (!f.is_open()) return;

    f.seekp(0, std::ios::end);
    if (f.tellp() > 512 * 1024) {
        f.close();
        f.open(log_path_.c_str(), std::ios::out | std::ios::trunc | std::ios::binary);
        if (!f.is_open()) return;
        static const unsigned char bom[] = {0xEF, 0xBB, 0xBF};
        f.write(reinterpret_cast<const char*>(bom), sizeof(bom));
        f << "[Log rotated: size exceeded 512 KB]\n";
    } else if (need_bom) {
        static const unsigned char bom[] = {0xEF, 0xBB, 0xBF};
        f.write(reinterpret_cast<const char*>(bom), sizeof(bom));
    }

    auto now = std::time(nullptr);
    auto tm = *std::localtime(&now);
    f << std::put_time(&tm, "[%Y-%m-%d %H:%M:%S] ") << msg << "\n";
}

void Logger::Write(std::wstring_view msg) {
    if (!Settings::Instance().write_log) return;
    int len = WideCharToMultiByte(CP_UTF8, 0, msg.data(), static_cast<int>(msg.length()), nullptr, 0, nullptr, nullptr);
    if (len <= 0) return;
    std::string s(len, 0);
    WideCharToMultiByte(CP_UTF8, 0, msg.data(), static_cast<int>(msg.length()), s.data(), len, nullptr, nullptr);
    Write(s);
}

void Logger::ResetLogFile() {
    std::lock_guard<std::mutex> lock(g_log_mutex);
    std::ofstream f(log_path_.c_str(), std::ios::out | std::ios::trunc | std::ios::binary);
    if (!f.is_open()) return;
    static const unsigned char bom[] = {0xEF, 0xBB, 0xBF};
    f.write(reinterpret_cast<const char*>(bom), sizeof(bom));
    auto now = std::time(nullptr);
    auto tm = *std::localtime(&now);
    f << std::put_time(&tm, "[%Y-%m-%d %H:%M:%S] ") << "Ultimakey: Журнал очищен\n";
}

} // namespace Ultimakey
