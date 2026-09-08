#pragma once
#include <string>

namespace auth { class AuthService; class AuthToken; }

namespace web {

class AuthController {
    auth::AuthService& service_;
    auth::AuthToken& token_;
public:
    AuthController(auth::AuthService& service, auth::AuthToken& token);
    bool handleLogin(const std::string& username, const std::string& password);
    bool handleLogout(const std::string& token_str);
    bool handleRefresh(const std::string& token_str);
};

} // namespace web
