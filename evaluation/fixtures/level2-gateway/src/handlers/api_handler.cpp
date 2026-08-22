#include "api_handler.h"
#include "../token/token.h"
#include "../gateway/router.h"
#include <iostream>

namespace handlers {

ApiHandler::ApiHandler(gw::Token& token, gw::Router& router)
    : token_(token), router_(router) {}

bool ApiHandler::handle(const gw::Request& req) {
    std::string tok = req.headers.count("Authorization")
        ? req.headers.at("Authorization")
        : "";
    if (!token_.validate(tok)) {
        std::cerr << "api: unauthorized\n";
        return false;
    }
    std::string target = router_.route(req);
    std::cout << "api: routed to " << target << "\n";
    return true;
}

} // namespace handlers
