#include "rate_limiter.h"
#include "../token/token.h"
#include <iostream>

namespace middleware {

RateLimiter::RateLimiter(gw::Token& token) : token_(token) {}

bool RateLimiter::allow(const std::string& token_str) {
    if (!token_.validate(token_str)) {
        std::cerr << "rate: invalid token\n";
        return false;
    }
    std::cout << "rate: allowed\n";
    return true;
}

} // namespace middleware
