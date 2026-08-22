#pragma once
#include <string>

namespace audit {

class AuditLogger {
public:
    void log(const std::string& event);
};

} // namespace audit
