#pragma once
#include "../gateway/request.h"

namespace gw { class Token; class Router; }

namespace handlers {

class ApiHandler {
    gw::Token& token_;
    gw::Router& router_;
public:
    ApiHandler(gw::Token& token, gw::Router& router);
    bool handle(const gw::Request& req);
};

} // namespace handlers
