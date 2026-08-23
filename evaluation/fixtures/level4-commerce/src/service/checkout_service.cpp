#include "checkout_service.h"
#include <iostream>

namespace service {

CheckoutService::CheckoutService(OrderService& orders, NotificationService& notification)
    : orders_(orders), notification_(notification) {}

bool CheckoutService::checkout(const std::string& user_id,
                                const std::string& item_id,
                                int quantity) {
    std::cout << "checkout: starting for user=" << user_id << "\n";
    if (!orders_.submit(user_id, item_id, quantity)) return false;
    notification_.notify(user_id, "order confirmed for " + item_id);
    return true;
}

} // namespace service
