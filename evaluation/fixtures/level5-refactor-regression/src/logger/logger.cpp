#include "logger.h"

namespace logger {

std::vector<std::string> Logger::legacy_logs;

void Logger::log(const std::string& level, const std::string& message) {
    legacy_logs.push_back("[" + level + "] " + message);
}

} // namespace logger
