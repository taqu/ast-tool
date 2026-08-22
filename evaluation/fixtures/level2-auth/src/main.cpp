#include "auth/auth_service.h"
#include "auth/auth_token.h"
#include "auth/token_cache.h"
#include "auth/user_repository.h"
#include "session/session_manager.h"
#include "web/auth_controller.h"
#include "web/session_controller.h"

int main() {
    auth::AuthToken token;
    auth::TokenCache cache;
    auth::UserRepository repo;
    auth::AuthService service(token, cache, repo);
    session::SessionManager sessions;

    web::AuthController auth_ctrl(service, token);
    web::SessionController session_ctrl(token, sessions);

    auth_ctrl.handleLogin("alice", "secret");
    session_ctrl.handle("alice-token");
    auth_ctrl.handleRefresh("alice-token");
    auth_ctrl.handleLogout("alice-token");

    return 0;
}
