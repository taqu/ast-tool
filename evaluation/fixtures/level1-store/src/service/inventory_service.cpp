#include "inventory_service.h"
#include <iostream>

namespace store {

bool InventoryService::save(const Product& product) {
    std::cout << "inventory: saving " << product.name << "\n";
    return true;
}

bool InventoryService::update(const Product& product) {
    std::cout << "inventory: updating id=" << product.id << "\n";
    return true;
}

Product InventoryService::find(int id) {
    Product p;
    p.id = id;
    p.name = "unknown";
    p.price = 0.0;
    return p;
}

} // namespace store
