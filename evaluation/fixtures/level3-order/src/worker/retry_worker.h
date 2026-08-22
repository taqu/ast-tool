#pragma once
#include <string>

namespace service { class PaymentService; }

namespace worker {

class RetryWorker {
    service::PaymentService& payment_;
public:
    explicit RetryWorker(service::PaymentService& payment);
    bool retry(const std::string& order_id, double amount);
};

} // namespace worker
