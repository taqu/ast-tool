#pragma once
#include <string>

namespace audit { class AuditLogger; }

namespace store {

class DataStore {
    audit::AuditLogger& auditor_;
public:
    explicit DataStore(audit::AuditLogger& auditor);
    void save(const std::string& data);
    void purge();
    std::string query();
};

} // namespace store
