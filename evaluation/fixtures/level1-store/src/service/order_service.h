#pragma once
#include "../model/product.h"

namespace store {

class InventoryService;

class OrderService {
    InventoryService& inventory_;
public:
    explicit OrderService(InventoryService& inv);
    bool save(const Product& product);
    bool update(int id, double new_price);
    bool process(int product_id);
};

} // namespace store
