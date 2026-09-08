#include "payment_service.h"
#include <iostream>

namespace service {

PaymentService::PaymentService(gateway::PaymentGateway& gateway) : gateway_(gateway) {}

bool PaymentService::authorize(const std::string& user_id, double amount) {
    std::cout << "payment: authorizing user=" << user_id << " amount=" << amount << "\n";
    return gateway_.charge(user_id, amount);
}

bool PaymentService::refund(const std::string& order_id, double amount) {
    std::cout << "payment: refunding order=" << order_id << "\n";
    return gateway_.refund(order_id, amount);
}

} // namespace service
