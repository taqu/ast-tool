#include "router.h"
#include <iostream>

namespace gw {

std::string Router::route(const Request& req) {
    std::cout << "router: " << req.method << " " << req.path << "\n";
    return req.path;
}

} // namespace gw
