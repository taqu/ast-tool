#include "data_store.h"
#include "../audit/audit_logger.h"
#include <iostream>

namespace store {

DataStore::DataStore(audit::AuditLogger& auditor) : auditor_(auditor) {}

void DataStore::save(const std::string& data) {
    std::cout << "store: saving " << data << "\n";
    auditor_.log("save: " + data);
}

void DataStore::purge() {
    std::cout << "store: purging\n";
    auditor_.log("purge");
}

std::string DataStore::query() {
    std::cout << "store: querying\n";
    return "stored-data";
}

} // namespace store
