#include "token.h"
#include <iostream>

namespace gw {

bool Token::validate(const std::string& token_str) {
    return token_str.length() >= 8;
}

bool Token::refresh(const std::string& token_str) {
    std::cout << "token: refreshing\n";
    return true;
}

} // namespace gw
