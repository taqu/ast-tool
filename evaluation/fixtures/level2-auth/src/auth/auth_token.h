#pragma once
#include <string>

namespace auth {

class AuthToken {
public:
    bool validate(const std::string& token_str);
    bool expire(const std::string& token_str);
    bool refresh(const std::string& token_str);
};

} // namespace auth
