#pragma once
#include "../model/product.h"

namespace store {

class InventoryService {
public:
    bool save(const Product& product);
    bool update(const Product& product);
    Product find(int id);
};

} // namespace store
