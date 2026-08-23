#pragma once
#include <string>

namespace service {

class ValidationService {
public:
    bool validate(const std::string& payload);
};

} // namespace service
