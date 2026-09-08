#include "order_service.h"
#include "inventory_service.h"
#include "payment_service.h"
#include "../repository/order_repository.h"
#include <iostream>

namespace service {

OrderService::OrderService(InventoryService& inventory,
                           PaymentService& payment,
                           repository::OrderRepository& repo)
    : inventory_(inventory), payment_(payment), repo_(repo) {}

bool OrderService::submit(const std::string& user_id,
                          const std::string& item_id,
                          int quantity) {
    std::cout << "order: submitting " << item_id << "\n";
    if (!inventory_.reserve(item_id, quantity)) return false;
    if (!payment_.authorize(user_id, 9.99)) return false;
    repo_.save(user_id, item_id, quantity);
    return true;
}

} // namespace service
