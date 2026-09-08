#pragma once
#include <string>

namespace service {
class OrderService;
class NotificationService;

class CheckoutService {
    OrderService& orders_;
    NotificationService& notifications_;
public:
    CheckoutService(OrderService& orders, NotificationService& notifications);
    bool process(const std::string& user_id, const std::string& item_id);
};

} // namespace service
