#include "checkout_service.h"
#include "order_service.h"
#include "notification_service.h"
#include <iostream>

namespace service {

CheckoutService::CheckoutService(OrderService& orders, NotificationService& notifications)
    : orders_(orders), notifications_(notifications) {}

bool CheckoutService::process(const std::string& user_id, const std::string& item_id) {
    std::cout << "checkout: processing " << user_id << "\n";
    bool ok = orders_.submit(user_id, item_id, 1);
    if (ok) notifications_.notify(user_id, "Order confirmed");
    return ok;
}

} // namespace service
