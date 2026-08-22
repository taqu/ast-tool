#include "audit_logger.h"
#include <iostream>

namespace audit {

void AuditLogger::log(const std::string& event) {
    std::cout << "audit: " << event << "\n";
}

} // namespace audit
