#pragma once
#include <string>
#include "repository/inventory_repository.h"

namespace service {

class InventoryService {
public:
    explicit InventoryService(repository::InventoryRepository& repo);
    bool reserve(const std::string& item_id, int quantity);
    bool release(const std::string& item_id, int quantity);

private:
    repository::InventoryRepository& repo_;
};

} // namespace service
