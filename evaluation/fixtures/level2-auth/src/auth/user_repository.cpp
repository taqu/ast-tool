#include "user_repository.h"
#include <iostream>

namespace auth {

User UserRepository::find(const std::string& identifier) {
    std::cout << "repo: finding " << identifier << "\n";
    return User{1, identifier, identifier + "-token"};
}

bool UserRepository::save(const User& user) {
    std::cout << "repo: saving user " << user.id << "\n";
    return true;
}

bool UserRepository::update(const User& user) {
    std::cout << "repo: updating user " << user.id << "\n";
    return true;
}

} // namespace auth
