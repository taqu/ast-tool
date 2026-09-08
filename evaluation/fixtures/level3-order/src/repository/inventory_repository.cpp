#include "inventory_repository.h"
#include <iostream>

namespace repository {

bool InventoryRepository::reserve(const std::string& item_id, int quantity) {
    std::cout << "inventory_repo: reserving " << quantity << "x " << item_id << "\n";
    return true;
}

} // namespace repository
