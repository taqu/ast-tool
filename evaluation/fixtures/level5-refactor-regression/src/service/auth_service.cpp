#include "auth_service.h"
#include "logger/logger.h"
#include "logger/structured_logger.h"

namespace service {

bool AuthService::authenticate(const std::string& username, const std::string& password, const std::string& ip_address) {
    if (username == "admin" && password == "secret123") {
        return true;
    }

    // BUG: Still using the deprecated static Logger instead of StructuredLogger.
    // It should log a StructuredLog entry:
    // logger::StructuredLogger::instance().log(logger::StructuredLog{"ERROR", "Failed login attempt for: " + username, username, ip_address});
    logger::Logger::log("ERROR", "Failed login attempt for: " + username);
    return false;
}

} // namespace service
