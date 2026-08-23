#pragma once
#include <string>

namespace store {

class DatabaseStore {
public:
    bool save(const std::string& key, const std::string& value);
    bool validate(const std::string& value); // for selective refactoring
};

} // namespace store
