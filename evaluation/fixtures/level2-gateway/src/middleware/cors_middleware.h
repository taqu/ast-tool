#pragma once
#include <string>

namespace gw { class Token; }

namespace middleware {

class CorsMiddleware {
    gw::Token& token_;
public:
    explicit CorsMiddleware(gw::Token& token);
    bool preflight(const std::string& token_str);
};

} // namespace middleware
