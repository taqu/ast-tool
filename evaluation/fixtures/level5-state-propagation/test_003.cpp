#include <cassert>
#include <iostream>
#include "service/order_service.h"
#include "service/inventory_client.h"
#include "db/database_client.h"
#include "model/context.h"

int main() {
    service::InventoryClient inventory;
    db::DatabaseClient db;
    service::OrderService order_service(inventory, db);

    model::RequestContext context;
    context.request_id = "req_111";
    context.correlation_id = "corr_999";

    bool success = order_service.submit_order("ord_123", "usr_456", "item_abc", 2, context);
    assert(success);

    // Verify correlation ID is propagated to inventory client
    assert(inventory.last_correlation_id() == "corr_999");

    // Verify correlation ID is propagated to database client
    if (db.last_saved_correlation_id_ != "corr_999") {
        std::cerr << "FAIL: correlation_id not propagated to DB: " << db.last_saved_correlation_id_ << std::endl;
        return 1;
    }

    std::cout << "SUCCESS" << std::endl;
    return 0;
}
