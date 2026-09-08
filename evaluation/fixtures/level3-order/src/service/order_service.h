#pragma once
#include <string>

namespace service { class InventoryService; class PaymentService; }
namespace repository { class OrderRepository; }

namespace service {

class OrderService {
    InventoryService& inventory_;
    PaymentService& payment_;
    repository::OrderRepository& repo_;
public:
    OrderService(InventoryService& inventory,
                 PaymentService& payment,
                 repository::OrderRepository& repo);
    bool submit(const std::string& user_id, const std::string& item_id, int quantity);
};

} // namespace service
