#pragma once
#include <string>
#include "model/order.h"
#include "repository/order_repository.h"
#include "service/inventory_service.h"
#include "service/payment_service.h"

namespace workflow {

class OrderProcessingWorkflow {
public:
    OrderProcessingWorkflow(repository::OrderRepository& repo,
                            service::InventoryService& inventory,
                            service::PaymentService& payment)
        : repo_(repo), inventory_(inventory), payment_(payment) {}

    bool process(const model::Order& order);

private:
    repository::OrderRepository& repo_;
    service::InventoryService& inventory_;
    service::PaymentService& payment_;
};

} // namespace workflow
