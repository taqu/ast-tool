#include "auth_middleware.h"
#include "../token/token.h"
#include <iostream>

namespace middleware {

AuthMiddleware::AuthMiddleware(gw::Token& token) : token_(token) {}

bool AuthMiddleware::check(const std::string& token_str) {
    if (!token_.validate(token_str)) {
        std::cerr << "auth: rejected\n";
        return false;
    }
    return true;
}

} // namespace middleware
