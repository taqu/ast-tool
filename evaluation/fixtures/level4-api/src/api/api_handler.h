#pragma once
#include <string>
#include "processor/request_processor.h"
#include "callback_registry.h"

namespace api {

class ApiHandler {
public:
    ApiHandler(processor::RequestProcessor& processor, CallbackRegistry& registry);
    bool handleRequest(const std::string& req);
};

} // namespace api
