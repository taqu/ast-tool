#include "subscription_billing_job.h"

namespace jobs {

void SubscriptionBillingJob::execute_billing(const std::string& user_id, int monthly_fee_cents) {
    // BUG: Passing raw cents directly to a function expecting dollars.
    // It should perform: payment_.charge(user_id, monthly_fee_cents / 100.0, Currency::USD);
    payment_.charge(user_id, monthly_fee_cents, Currency::USD);
}

} // namespace jobs
