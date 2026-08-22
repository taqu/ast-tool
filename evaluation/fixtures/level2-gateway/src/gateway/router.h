#pragma once
#include "request.h"
#include <string>

namespace gw {

class Router {
public:
    std::string route(const Request& req);
};

} // namespace gw
