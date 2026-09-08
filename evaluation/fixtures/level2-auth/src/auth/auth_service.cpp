#include "auth_service.h"
#include "auth_token.h"
#include "token_cache.h"
#include "user_repository.h"
#include <iostream>

namespace auth {

AuthService::AuthService(AuthToken& token, TokenCache& cache, UserRepository& repo)
    : token_(token), cache_(cache), repo_(repo) {}

bool AuthService::login(const std::string& username, const std::string& password) {
    User u = repo_.find(username);
    return token_.validate(u.token);
}

bool AuthService::refresh(const std::string& token_str) {
    token_.expire(token_str);
    cache_.invalidate(token_str);
    User u = repo_.find(token_str);
    u.token = token_str + "-refreshed";
    repo_.update(u);
    return token_.refresh(u.token);
}

bool AuthService::update(const User& user) {
    std::cout << "auth: updating user " << user.id << "\n";
    return repo_.update(user);
}

bool AuthService::validate(const std::string& token_str) {
    return !cache_.get(token_str).empty();
}

} // namespace auth
