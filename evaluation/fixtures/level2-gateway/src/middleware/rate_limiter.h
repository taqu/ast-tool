#pragma once
#include <string>

namespace gw { class Token; }

namespace middleware {

class RateLimiter {
    gw::Token& token_;
public:
    explicit RateLimiter(gw::Token& token);
    bool allow(const std::string& token_str);
};

} // namespace middleware
