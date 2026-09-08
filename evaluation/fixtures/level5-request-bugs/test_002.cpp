#include <cassert>
#include <iostream>
#include "serializer/json_serializer.h"
#include "logger/audit_logger.h"
#include "model/log_message.h"
#include "model/audit_log_message.h"

int main() {
    serializer::JsonSerializer serializer;
    logger::AuditLogger logger(serializer);

    model::AuditLogMessage msg("User logged in", "user_123", "192.168.1.1");
    logger.log(msg);

    // Verify that the formatted log contains the audit fields (user_id and ip)
    const auto& logs = logger.formatted_logs();
    assert(logs.size() == 1);

    std::string formatted = logs[0];
    if (formatted.find("user_123") == std::string::npos || formatted.find("192.168.1.1") == std::string::npos) {
        std::cerr << "FAIL: Audit fields missing. Formatted log: " << formatted << std::endl;
        return 1;
    }

    std::cout << "SUCCESS" << std::endl;
    return 0;
}
