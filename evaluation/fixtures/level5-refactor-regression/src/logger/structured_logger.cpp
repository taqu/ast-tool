#include "structured_logger.h"

namespace logger {

StructuredLogger& StructuredLogger::instance() {
    static StructuredLogger inst;
    return inst;
}

void StructuredLogger::log(const StructuredLog& entry) {
    entries_.push_back(entry);
}

} // namespace logger
