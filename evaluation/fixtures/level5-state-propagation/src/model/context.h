#pragma once
#include <string>

namespace model {

struct RequestContext {
    std::string request_id;
    std::string correlation_id;
    bool is_admin = false;
};

} // namespace model
