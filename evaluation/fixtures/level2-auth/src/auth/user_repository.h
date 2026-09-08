#pragma once
#include "../model/user.h"
#include <string>

namespace auth {

class UserRepository {
public:
    User find(const std::string& identifier);
    bool save(const User& user);
    bool update(const User& user);
};

} // namespace auth
