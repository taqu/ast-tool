#pragma once
#include <string>

namespace auth {

struct User {
    int id;
    std::string name;
    std::string token;
};

} // namespace auth
