#include <cassert>
#include <iostream>
#include "service/auth_service.h"
#include "logger/logger.h"
#include "logger/structured_logger.h"

int main() {
    service::AuthService auth_service;

    // Fail login to trigger logs
    bool success = auth_service.authenticate("admin", "wrong_password", "10.0.0.5");
    assert(!success);

    // Verify it is logged in StructuredLogger
    const auto& s_logs = logger::StructuredLogger::instance().logs();
    if (s_logs.empty()) {
        std::cerr << "FAIL: No structured log entry found for failed login" << std::endl;
        return 1;
    }

    assert(s_logs[0].level == "ERROR");
    assert(s_logs[0].user_id == "admin");
    assert(s_logs[0].ip_address == "10.0.0.5");

    // Make sure standard legacy logging isn't used
    if (!logger::Logger::legacy_logs.empty()) {
        std::cerr << "FAIL: Legacy logger incorrectly called for structured logs" << std::endl;
        return 1;
    }

    std::cout << "SUCCESS" << std::endl;
    return 0;
}
