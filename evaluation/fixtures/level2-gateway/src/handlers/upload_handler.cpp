#include "upload_handler.h"
#include "../gateway/router.h"
#include <iostream>

namespace handlers {

UploadHandler::UploadHandler(gw::Router& router) : router_(router) {}

bool UploadHandler::handle(const gw::Request& req) {
    std::cout << "upload: handling " << req.path << "\n";
    std::string target = router_.route(req);
    std::cout << "upload: routed to " << target << "\n";
    return true;
}

} // namespace handlers
