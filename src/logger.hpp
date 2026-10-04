#pragma once

#include "types.hpp"
#include <string>
#include <string_view>

namespace Ultimakey {

class Logger {
public:
    static Logger& Instance();

    void Write(std::string_view msg);
    void Write(std::wstring_view msg);

    std::wstring GetLogFilePath() const;

private:
    Logger();
    ~Logger();

    std::wstring log_path_;
};

} // namespace Ultimakey
