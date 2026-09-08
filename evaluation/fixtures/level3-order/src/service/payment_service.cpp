#include "payment_service.h"
#include "../gateway/payment_gateway.h"
#include <iostream>

namespace service {

PaymentService::PaymentService(gateway::PaymentGateway& gateway) : gateway_(gateway) {}

bool PaymentService::authorize(const std::string& user_id, double amount) {
    std::cout << "payment: authorizing " << user_id << " for " << amount << "\n";
    return gateway_.charge(user_id, amount);
}

} // namespace service
