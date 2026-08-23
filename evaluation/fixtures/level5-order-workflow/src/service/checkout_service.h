#pragma once
#include <string>
#include "payment_service.h"

namespace service {

class CheckoutService {
public:
    explicit CheckoutService(PaymentService& payment) : payment_(payment) {}

    bool checkout(const std::string& user_id, double amount) {
        // Checkout passes the amount directly in standard unit (e.g. 29.99)
        return payment_.charge(user_id, amount, Currency::USD);
    }

private:
    PaymentService& payment_;
};

} // namespace service
