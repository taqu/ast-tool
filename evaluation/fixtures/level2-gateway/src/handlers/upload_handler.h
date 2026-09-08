#pragma once
#include "../gateway/request.h"

namespace gw { class Router; }

namespace handlers {

class UploadHandler {
    gw::Router& router_;
public:
    explicit UploadHandler(gw::Router& router);
    bool handle(const gw::Request& req);
};

} // namespace handlers
