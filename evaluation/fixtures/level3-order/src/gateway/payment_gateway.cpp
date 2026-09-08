#include "payment_gateway.h"
#include <iostream>

namespace gateway {

bool PaymentGateway::charge(const std::string& user_id, double amount) {
    std::cout << "gateway: charging " << user_id << " $" << amount << "\n";
    return true;
}

} // namespace gateway
