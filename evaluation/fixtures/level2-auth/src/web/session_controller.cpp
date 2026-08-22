#include "session_controller.h"
#include "../auth/auth_token.h"
#include "../session/session_manager.h"
#include <iostream>

namespace web {

SessionController::SessionController(auth::AuthToken& token, session::SessionManager& sessions)
    : token_(token), sessions_(sessions) {}

bool SessionController::handle(const std::string& token_str) {
    if (!token_.validate(token_str)) {
        std::cerr << "session: token check failed\n";
        return false;
    }
    session::Session s = sessions_.get(token_str);
    std::cout << "session: user=" << s.user << "\n";
    return true;
}

} // namespace web
