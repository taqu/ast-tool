#pragma once
#include <string>

namespace service {

class AuthService {
public:
    bool authenticate(const std::string& username, const std::string& password, const std::string& ip_address);
};

} // namespace service
