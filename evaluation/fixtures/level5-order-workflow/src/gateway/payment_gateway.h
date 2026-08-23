#pragma once
#include <string>
#include "model/currency.h"

namespace gateway {

class PaymentGateway {
public:
    virtual ~PaymentGateway() = default;
    virtual bool execute_charge(const std::string& user_id, double amount, Currency currency) = 0;
};

class MockPaymentGateway : public PaymentGateway {
public:
    bool execute_charge(const std::string& user_id, double amount, Currency currency) override {
        last_charged_amount_ = amount;
        last_charged_currency_ = currency;
        return amount <= 1000.0; // Fail for amounts > 1000.0 to simulate limit
    }

    double last_charged_amount_ = 0.0;
    Currency last_charged_currency_ = Currency::USD;
};

} // namespace gateway
