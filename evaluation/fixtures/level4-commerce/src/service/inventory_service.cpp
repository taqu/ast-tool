#include "inventory_service.h"
#include <iostream>

namespace service {

InventoryService::InventoryService(repository::InventoryRepository& repo) : repo_(repo) {}

bool InventoryService::reserve(const std::string& item_id, int quantity) {
    std::cout << "inventory: reserving item=" << item_id << " qty=" << quantity << "\n";
    return repo_.reserve(item_id, quantity);
}

bool InventoryService::release(const std::string& item_id, int quantity) {
    std::cout << "inventory: releasing item=" << item_id << " qty=" << quantity << "\n";
    return repo_.release(item_id, quantity);
}

} // namespace service
