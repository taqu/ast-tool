#pragma once
#include <string>
#include <vector>

namespace logger {

struct StructuredLog {
    std::string level;
    std::string message;
    std::string user_id;
    std::string ip_address;
};

class StructuredLogger {
public:
    static StructuredLogger& instance();
    void log(const StructuredLog& entry);
    const std::vector<StructuredLog>& logs() const { return entries_; }

private:
    StructuredLogger() = default;
    std::vector<StructuredLog> entries_;
};

} // namespace logger
