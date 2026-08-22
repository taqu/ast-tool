#include "admin_order_handler.h"
#include "../service/admin_service.h"
#include <iostream>

namespace web {

AdminOrderHandler::AdminOrderHandler(service::AdminService& admin) : admin_(admin) {}

bool AdminOrderHandler::handle(const std::string& admin_id,
                               const std::string& item_id,
                               int quantity) {
    std::cout << "admin: order for " << item_id << "\n";
    return admin_.createOrder(admin_id, item_id, quantity);
}

} // namespace web
