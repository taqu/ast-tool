#include "api_handler.h"
#include <iostream>

namespace api {

ApiHandler::ApiHandler(processor::RequestProcessor& processor, CallbackRegistry& registry)
    : processor_(processor), registry_(registry) {}

bool ApiHandler::handleRequest(const std::string& req) {
    std::cout << "api: received request=" << req << "\n";
    std::string req_id = "req_" + req;
    registry_.trigger(req_id, req);
    return processor_.process(req_id, req);
}

} // namespace api
