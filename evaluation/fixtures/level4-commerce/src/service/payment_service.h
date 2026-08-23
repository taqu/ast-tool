#pragma once
#include <string>
#include "gateway/payment_gateway.h"

namespace service {

class PaymentService {
public:
    explicit PaymentService(gateway::PaymentGateway& gateway);
    bool authorize(const std::string& user_id, double amount);
    bool refund(const std::string& order_id, double amount);

private:
    gateway::PaymentGateway& gateway_;
};

} // namespace service
