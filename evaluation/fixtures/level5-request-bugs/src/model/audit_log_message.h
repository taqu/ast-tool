#pragma once
#include "log_message.h"

namespace model {

class AuditLogMessage : public LogMessage {
public:
    AuditLogMessage(const std::string& message, const std::string& user_id, const std::string& ip_address)
        : LogMessage(message), user_id_(user_id), ip_address_(ip_address) {}

    std::string user_id() const { return user_id_; }
    std::string ip_address() const { return ip_address_; }

private:
    std::string user_id_;
    std::string ip_address_;
};

} // namespace model
