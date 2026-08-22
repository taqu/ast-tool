#include "health_handler.h"
#include <iostream>

namespace handlers {

bool HealthHandler::handle(const gw::Request& req) {
    std::cout << "health: ok\n";
    return true;
}

} // namespace handlers
