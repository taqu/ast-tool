#pragma once
#include <string>

namespace validator {

class DataValidator {
public:
    bool validate(const std::string& data);
};

} // namespace validator
