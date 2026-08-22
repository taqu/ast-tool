#include "cors_middleware.h"
#include "../token/token.h"
#include <iostream>

namespace middleware {

CorsMiddleware::CorsMiddleware(gw::Token& token) : token_(token) {}

bool CorsMiddleware::preflight(const std::string& token_str) {
    if (!token_.validate(token_str)) {
        std::cerr << "cors: invalid token\n";
        return false;
    }
    std::cout << "cors: preflight ok\n";
    return true;
}

} // namespace middleware
