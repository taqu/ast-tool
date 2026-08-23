#pragma once
#include <string>
#include "service/payment_service.h"

namespace jobs {

class SubscriptionBillingJob {
public:
    explicit SubscriptionBillingJob(service::PaymentService& payment) : payment_(payment) {}

    void execute_billing(const std::string& user_id, int monthly_fee_cents);

private:
    service::PaymentService& payment_;
};

} // namespace jobs
