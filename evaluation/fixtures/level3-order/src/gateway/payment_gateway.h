#pragma once
#include <string>

namespace gateway {

class PaymentGateway {
public:
    bool charge(const std::string& user_id, double amount);
};

} // namespace gateway
