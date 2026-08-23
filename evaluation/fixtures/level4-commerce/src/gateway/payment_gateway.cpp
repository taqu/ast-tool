#include "payment_gateway.h"
#include <iostream>

namespace gateway {

bool PaymentGateway::charge(const std::string& user_id, double amount) {
    std::cout << "gateway: charging " << user_id << " amount=" << amount << "\n";
    return true;
}

bool PaymentGateway::refund(const std::string& charge_id, double amount) {
    std::cout << "gateway: refunding " << charge_id << " amount=" << amount << "\n";
    return true;
}

bool BillingGateway::charge(const std::string& user_id, double amount) {
    std::cout << "billing-gateway: charging " << user_id << " amount=" << amount << "\n";
    return true;
}

} // namespace gateway
