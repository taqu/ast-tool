#pragma once
#include <string>

namespace gateway {

class PaymentGateway {
public:
    bool charge(const std::string& user_id, double amount);
    bool refund(const std::string& charge_id, double amount);
};

class BillingGateway {
public:
    bool charge(const std::string& user_id, double amount); // Unrelated class, must not be changed
};

} // namespace gateway
