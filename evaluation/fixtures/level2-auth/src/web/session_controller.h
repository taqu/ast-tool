#pragma once
#include <string>

namespace auth { class AuthToken; }
namespace session { class SessionManager; }

namespace web {

class SessionController {
    auth::AuthToken& token_;
    session::SessionManager& sessions_;
public:
    SessionController(auth::AuthToken& token, session::SessionManager& sessions);
    bool handle(const std::string& token_str);
};

} // namespace web
