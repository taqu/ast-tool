#pragma once
#include <string>
#include <vector>

namespace logger {

class Logger {
public:
    static void log(const std::string& level, const std::string& message);
    static std::vector<std::string> legacy_logs;
};

} // namespace logger
