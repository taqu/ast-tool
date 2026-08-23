#pragma once
#include <string>

namespace model {

enum class OrderStatus {
    PENDING,
    PAID,
    FAILED
};

struct Order {
    std::string id;
    std::string user_id;
    std::string item_id;
    int quantity;
    double price_per_item;
    OrderStatus status = OrderStatus::PENDING;
};

} // namespace model
