#pragma once
#include <string>
#include "model/context.h"
#include "inventory_client.h"
#include "db/database_client.h"

namespace service {

class OrderService {
public:
    OrderService(InventoryClient& inventory, db::DatabaseClient& db)
        : inventory_(inventory), db_(db) {}

    bool submit_order(const std::string& order_id, const std::string& user_id, const std::string& item_id, int quantity, const model::RequestContext& context);

private:
    InventoryClient& inventory_;
    db::DatabaseClient& db_;
};

} // namespace service
