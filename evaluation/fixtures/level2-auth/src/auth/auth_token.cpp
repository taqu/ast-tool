#include "auth_token.h"
#include <iostream>

namespace auth {

bool AuthToken::validate(const std::string& token_str) {
    return !token_str.empty();
}

bool AuthToken::expire(const std::string& token_str) {
    std::cout << "token: expiring " << token_str << "\n";
    return true;
}

bool AuthToken::refresh(const std::string& token_str) {
    std::cout << "token: refreshing " << token_str << "\n";
    return true;
}

} // namespace auth
