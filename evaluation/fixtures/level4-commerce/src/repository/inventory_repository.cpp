#include "inventory_repository.h"
#include <iostream>

namespace repository {

bool InventoryRepository::reserve(const std::string& item_id, int quantity) {
    std::cout << "inventory-repo: reserving item=" << item_id << " qty=" << quantity << "\n";
    return true;
}

bool InventoryRepository::release(const std::string& item_id, int quantity) {
    std::cout << "inventory-repo: releasing item=" << item_id << " qty=" << quantity << "\n";
    return true;
}

int InventoryRepository::available(const std::string& item_id) {
    return 100;
}

} // namespace repository
