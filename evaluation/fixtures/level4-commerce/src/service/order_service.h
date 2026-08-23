#pragma once
#include <string>
#include "inventory_service.h"
#include "payment_service.h"
#include "repository/order_repository.h"

namespace service {

class OrderService {
public:
    OrderService(InventoryService& inventory,
                 PaymentService& payment,
                 repository::OrderRepository& repo);
    bool submit(const std::string& user_id,
                const std::string& item_id,
                int quantity);
    bool cancel(const std::string& order_id);

private:
    InventoryService& inventory_;
    PaymentService& payment_;
    repository::OrderRepository& repo_;
};

} // namespace service
