#pragma once
#include "../model/user.h"
#include <string>

namespace auth {

class AuthToken;
class TokenCache;
class UserRepository;

class AuthService {
    AuthToken& token_;
    TokenCache& cache_;
    UserRepository& repo_;
public:
    AuthService(AuthToken& token, TokenCache& cache, UserRepository& repo);
    bool login(const std::string& username, const std::string& password);
    bool refresh(const std::string& token_str);
    bool update(const User& user);
    bool validate(const std::string& token_str);
};

} // namespace auth
