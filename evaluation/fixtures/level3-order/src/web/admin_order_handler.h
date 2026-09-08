#pragma once
#include <string>

namespace service { class AdminService; }

namespace web {

class AdminOrderHandler {
    service::AdminService& admin_;
public:
    explicit AdminOrderHandler(service::AdminService& admin);
    bool handle(const std::string& admin_id, const std::string& item_id, int quantity);
};

} // namespace web
