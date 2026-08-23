#pragma once
#include <string>
#include "order_service.h"
#include "notification_service.h"

namespace service {

class CheckoutService {
public:
    CheckoutService(OrderService& orders, NotificationService& notification);
    bool checkout(const std::string& user_id,
                  const std::string& item_id,
                  int quantity);

private:
    OrderService& orders_;
    NotificationService& notification_;
};

} // namespace service
