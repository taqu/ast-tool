#include "admin_service.h"
#include <iostream>

namespace service {

bool AdminService::createOrder(const std::string& admin_id,
                               const std::string& item_id,
                               int quantity) {
    std::cout << "admin: creating order for " << item_id << " qty=" << quantity << "\n";
    return true;
}

} // namespace service
