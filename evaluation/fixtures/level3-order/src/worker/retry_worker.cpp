#include "retry_worker.h"
#include "../service/payment_service.h"
#include <iostream>

namespace worker {

RetryWorker::RetryWorker(service::PaymentService& payment) : payment_(payment) {}

bool RetryWorker::retry(const std::string& order_id, double amount) {
    std::cout << "retry: retrying order " << order_id << "\n";
    return payment_.authorize(order_id, amount);
}

} // namespace worker
