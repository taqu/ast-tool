#include "token/token.h"
#include "gateway/router.h"
#include "gateway/request.h"
#include "middleware/auth_middleware.h"
#include "middleware/rate_limiter.h"
#include "middleware/cors_middleware.h"
#include "handlers/api_handler.h"
#include "handlers/upload_handler.h"
#include "handlers/health_handler.h"

int main() {
    gw::Token token;
    gw::Router router;

    middleware::AuthMiddleware auth(token);
    middleware::RateLimiter limiter(token);
    middleware::CorsMiddleware cors(token);
    handlers::ApiHandler api(token, router);
    handlers::UploadHandler upload(router);
    handlers::HealthHandler health;

    gw::Request req{
        "GET",
        "/api/data",
        {{"Authorization", "abc12345"}},
        ""
    };

    auth.check("abc12345");
    limiter.allow("abc12345");
    cors.preflight("abc12345");
    api.handle(req);
    upload.handle(req);
    health.handle(req);

    return 0;
}
