#pragma once
#include "model/log_message.h"
#include "model/audit_log_message.h"

namespace serializer {

class BaseSerializer {
public:
    virtual ~BaseSerializer() = default;
    virtual std::string format(const model::LogMessage& msg) {
        return "[LOG] " + msg.get_text();
    }

    // BUG: Missing virtual declaration of AuditLogMessage formatting overload.
    // It should be declared as:
    // virtual std::string format(const model::AuditLogMessage& msg);
    // to allow derived class overrides to execute when called polymorphically.
};

} // namespace serializer
