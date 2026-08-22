#include "order_repository.h"
#include <iostream>

namespace repository {

bool OrderRepository::save(const std::string& user_id,
                           const std::string& item_id,
                           int quantity) {
    std::cout << "order_repo: saving order for " << user_id << "\n";
    return true;
}

} // namespace repository
