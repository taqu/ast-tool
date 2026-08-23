#include <cassert>
#include <iostream>
#include "gateway/payment_gateway.h"
#include "service/payment_service.h"
#include "service/checkout_service.h"
#include "jobs/subscription_billing_job.h"

int main() {
    gateway::MockPaymentGateway gateway;
    service::PaymentService payment(gateway);
    service::CheckoutService checkout_service(payment);
    jobs::SubscriptionBillingJob billing_job(payment);

    // Standard checkout should work
    bool checkout_ok = checkout_service.checkout("user_1", 29.99);
    assert(checkout_ok);
    assert(gateway.last_charged_amount_ == 29.99);

    // Run subscription billing with 1500 cents (should charge 15.00)
    billing_job.execute_billing("user_2", 1500);

    // Verify subscription billing correctly converted cents to dollars
    if (gateway.last_charged_amount_ != 15.00) {
        std::cerr << "FAIL: Subscription billing charged " << gateway.last_charged_amount_ << " instead of 15.00" << std::endl;
        return 1;
    }

    std::cout << "SUCCESS" << std::endl;
    return 0;
}
