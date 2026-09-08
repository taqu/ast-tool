#include <cassert>
#include <iostream>
#include "gateway/payment_gateway.h"
#include "service/payment_service.h"
#include "service/inventory_service.h"
#include "repository/order_repository.h"
#include "workflow/order_processing_workflow.h"

int main() {
    gateway::MockPaymentGateway gateway;
    service::PaymentService payment(gateway);
    service::InventoryService inventory;
    repository::OrderRepository repo;
    workflow::OrderProcessingWorkflow workflow(repo, inventory, payment);

    // Scenario 1: Successful workflow
    model::Order order1{"ord_1", "usr_1", "item_a", 2, 50.0};
    bool ok1 = workflow.process(order1);
    assert(ok1);
    assert(repo.get("ord_1").status == model::OrderStatus::PAID);

    // Scenario 2: Payment failure (amount is > 1000.0)
    model::Order order2{"ord_2", "usr_2", "item_b", 25, 50.0}; // total 1250.0 -> payment fails
    bool ok2 = workflow.process(order2);
    assert(!ok2);
    assert(repo.get("ord_2").status == model::OrderStatus::FAILED);

    // Check updates order: make sure there is no intermediate status PAID update recorded for ord_2
    for (const auto& update : repo.status_updates_) {
        if (update.first == "ord_2" && update.second == model::OrderStatus::PAID) {
            std::cerr << "FAIL: Order status was set to PAID despite payment failure" << std::endl;
            return 1;
        }
    }

    std::cout << "SUCCESS" << std::endl;
    return 0;
}
