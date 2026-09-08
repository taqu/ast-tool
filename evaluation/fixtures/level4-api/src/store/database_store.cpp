#include "database_store.h"
#include <iostream>

namespace store {

bool DatabaseStore::save(const std::string& key, const std::string& value) {
    std::cout << "db: saving key=" << key << " value=" << value << "\n";
    return true;
}

bool DatabaseStore::validate(const std::string& value) {
    std::cout << "db: validating value\n";
    return !value.empty();
}

} // namespace store
