#include "order_service.h"
#include "inventory_service.h"
#include <iostream>

namespace store {

OrderService::OrderService(InventoryService& inv) : inventory_(inv) {}

bool OrderService::save(const Product& product) {
    std::cout << "order: saving product id=" << product.id << "\n";
    return inventory_.save(product);
}

bool OrderService::update(int id, double new_price) {
    Product p = inventory_.find(id);
    p.price = new_price;
    return inventory_.update(p);
}

bool OrderService::process(int product_id) {
    Product p = inventory_.find(product_id);
    std::cout << "order: processing product id=" << p.id << "\n";
    return true;
}

} // namespace store
