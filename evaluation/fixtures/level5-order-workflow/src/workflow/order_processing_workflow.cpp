#include "order_processing_workflow.h"

namespace workflow {

bool OrderProcessingWorkflow::process(const model::Order& order) {
    repo_.save(order);

    if (!inventory_.reserve(order.item_id, order.quantity)) {
        repo_.update_status(order.id, model::OrderStatus::FAILED);
        return false;
    }

    // BUG: The order status is prematurely updated to PAID here,
    // before verify and authorization from the payment service is completed.
    repo_.update_status(order.id, model::OrderStatus::PAID);

    double total_amount = order.quantity * order.price_per_item;
    if (!payment_.charge(order.user_id, total_amount, Currency::USD)) {
        inventory_.release(order.item_id, order.quantity);
        repo_.update_status(order.id, model::OrderStatus::FAILED);
        return false;
    }

    return true;
}

} // namespace workflow
