#include "order_repository.h"
#include <iostream>

namespace repository {

bool OrderRepository::save(const std::string& user_id,
                            const std::string& item_id,
                            int quantity) {
    std::cout << "order-repo: saving order user=" << user_id
              << " item=" << item_id << " qty=" << quantity << "\n";
    return true;
}

bool OrderRepository::update(const std::string& order_id, const std::string& status) {
    std::cout << "order-repo: updating order=" << order_id << " status=" << status << "\n";
    return true;
}

std::string OrderRepository::find(const std::string& order_id) {
    return order_id + "-data";
}

} // namespace repository
