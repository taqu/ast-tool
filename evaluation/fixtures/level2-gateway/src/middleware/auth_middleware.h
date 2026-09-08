#pragma once
#include <string>

namespace gw { class Token; }

namespace middleware {

class AuthMiddleware {
    gw::Token& token_;
public:
    explicit AuthMiddleware(gw::Token& token);
    bool check(const std::string& token_str);
};

} // namespace middleware
