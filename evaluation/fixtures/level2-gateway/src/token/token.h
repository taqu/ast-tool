#pragma once
#include <string>

namespace gw {

class Token {
public:
    bool validate(const std::string& token_str);
    bool refresh(const std::string& token_str);
};

} // namespace gw
