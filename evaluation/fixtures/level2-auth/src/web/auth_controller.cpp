#include "auth_controller.h"
#include "../auth/auth_service.h"
#include "../auth/auth_token.h"
#include <iostream>

namespace web {

AuthController::AuthController(auth::AuthService& service, auth::AuthToken& token)
    : service_(service), token_(token) {}

bool AuthController::handleLogin(const std::string& username, const std::string& password) {
    std::string candidate = username + "-token";
    if (!token_.validate(candidate)) {
        std::cerr << "login: token check failed\n";
        return false;
    }
    return service_.login(username, password);
}

bool AuthController::handleLogout(const std::string& token_str) {
    token_.expire(token_str);
    return true;
}

bool AuthController::handleRefresh(const std::string& token_str) {
    if (!token_.validate(token_str)) {
        std::cerr << "refresh: token check failed\n";
        return false;
    }
    return service_.refresh(token_str);
}

} // namespace web
