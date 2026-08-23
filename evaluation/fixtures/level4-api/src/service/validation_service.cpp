#include "validation_service.h"
#include <iostream>

namespace service {

bool ValidationService::validate(const std::string& payload) {
    std::cout << "validation: validating payload size=" << payload.size() << "\n";
    return payload.find("invalid") == std::string::npos;
}

} // namespace service
