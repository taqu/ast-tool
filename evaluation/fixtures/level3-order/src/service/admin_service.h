#pragma once
#include <string>

namespace service {

class AdminService {
public:
    bool createOrder(const std::string& admin_id,
                     const std::string& item_id,
                     int quantity);
};

} // namespace service
