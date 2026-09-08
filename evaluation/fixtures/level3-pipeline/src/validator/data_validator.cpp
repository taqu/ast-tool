#include "data_validator.h"
#include <iostream>

namespace validator {

bool DataValidator::validate(const std::string& data) {
    std::cout << "validator: validating\n";
    return !data.empty();
}

} // namespace validator
