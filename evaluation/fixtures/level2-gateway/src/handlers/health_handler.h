#pragma once
#include "../gateway/request.h"

namespace handlers {

class HealthHandler {
public:
    bool handle(const gw::Request& req);
};

} // namespace handlers
