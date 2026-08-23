#pragma once
#include "base_serializer.h"

namespace serializer {

class JsonSerializer : public BaseSerializer {
public:
    std::string format(const model::LogMessage& msg) override {
        return "{\"level\":\"info\",\"message\":\"" + msg.get_text() + "\"}";
    }

    // Overload of format, but cannot override a non-virtual function from base.
    std::string format(const model::AuditLogMessage& msg) {
        return "{\"level\":\"audit\",\"message\":\"" + msg.get_text() + "\",\"user_id\":\"" + msg.user_id() + "\",\"ip\":\"" + msg.ip_address() + "\"}";
    }
};

} // namespace serializer
