#pragma once
#include <string>

namespace gateway { class PaymentGateway; }

namespace service {

class PaymentService {
    gateway::PaymentGateway& gateway_;
public:
    explicit PaymentService(gateway::PaymentGateway& gateway);
    bool authorize(const std::string& user_id, double amount);
};

} // namespace service
