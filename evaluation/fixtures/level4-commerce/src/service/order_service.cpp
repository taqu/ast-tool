#include "order_service.h"
#include <iostream>

namespace service {

OrderService::OrderService(InventoryService& inventory,
                           PaymentService& payment,
                           repository::OrderRepository& repo)
    : inventory_(inventory), payment_(payment), repo_(repo) {}

bool OrderService::submit(const std::string& user_id,
                          const std::string& item_id,
                          int quantity) {
    std::cout << "order: submitting item=" << item_id << " for user=" << user_id << "\n";
    if (!inventory_.reserve(item_id, quantity)) return false;
    if (!payment_.authorize(user_id, 9.99 * quantity)) return false;
    repo_.save(user_id, item_id, quantity);
    return true;
}

bool OrderService::cancel(const std::string& order_id) {
    std::cout << "order: cancelling order=" << order_id << "\n";
    return repo_.update(order_id, "cancelled");
}

} // namespace service
