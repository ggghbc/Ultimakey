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
    std::ofstream f(log_path_.c_str(), std::ios::out | std::ios::app);
    if (!f.is_open()) return;

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

} // namespace Ultimakey
