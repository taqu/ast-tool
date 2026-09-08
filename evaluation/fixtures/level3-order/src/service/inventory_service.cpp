#include "inventory_service.h"
#include "../repository/inventory_repository.h"
#include <iostream>

namespace service {

InventoryService::InventoryService(repository::InventoryRepository& repo) : repo_(repo) {}

bool InventoryService::reserve(const std::string& item_id, int quantity) {
    std::cout << "inventory: reserving " << quantity << "x " << item_id << "\n";
    return repo_.reserve(item_id, quantity);
}

} // namespace service
