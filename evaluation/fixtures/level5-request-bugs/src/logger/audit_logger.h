#pragma once
#include "serializer/base_serializer.h"
#include "model/audit_log_message.h"
#include <vector>

namespace logger {

class AuditLogger {
public:
    explicit AuditLogger(serializer::BaseSerializer& serializer) : serializer_(serializer) {}

    void log(const model::AuditLogMessage& msg) {
        formatted_logs_.push_back(serializer_.format(msg));
    }

    const std::vector<std::string>& formatted_logs() const { return formatted_logs_; }

private:
    serializer::BaseSerializer& serializer_;
    std::vector<std::string> formatted_logs_;
};

} // namespace logger
