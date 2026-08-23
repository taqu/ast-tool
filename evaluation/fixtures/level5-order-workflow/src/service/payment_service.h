#pragma once
#include <string>
#include "gateway/payment_gateway.h"
#include "model/currency.h"

namespace service {

class PaymentService {
public:
    explicit PaymentService(gateway::PaymentGateway& gateway) : gateway_(gateway) {}

    // Charges in standard unit (e.g. 10.50 for $10.50)
    bool charge(const std::string& user_id, double amount, Currency currency) {
        return gateway_.execute_charge(user_id, amount, currency);
    }

private:
    gateway::PaymentGateway& gateway_;
};

} // namespace service
