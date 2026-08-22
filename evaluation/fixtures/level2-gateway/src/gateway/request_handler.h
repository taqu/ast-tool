#pragma once
#include "request.h"
#include <string>

namespace gw {

class RequestHandler {
public:
    void process(int status_code);
    void process(const std::string& raw_data);
    void process(const Request& req);
};

} // namespace gw
