#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include "model/order.h"

namespace repository {

class OrderRepository {
public:
    void save(const model::Order& order) {
        orders_[order.id] = order;
    }

    model::Order get(const std::string& id) {
        return orders_[id];
    }

    void update_status(const std::string& id, model::OrderStatus status) {
        orders_[id].status = status;
        status_updates_.push_back(std::make_pair(id, status));
    }

    std::unordered_map<std::string, model::Order> orders_;
    std::vector<std::pair<std::string, model::OrderStatus>> status_updates_;
};

} // namespace repository
