#include "request_handler.h"
#include <iostream>

namespace gw {

void RequestHandler::process(int status_code) {
    std::cout << "handler: status " << status_code << "\n";
}

void RequestHandler::process(const std::string& raw_data) {
    std::cout << "handler: raw data len=" << raw_data.size() << "\n";
}

void RequestHandler::process(const Request& req) {
    std::cout << "handler: " << req.method << " " << req.path << "\n";
}

} // namespace gw
