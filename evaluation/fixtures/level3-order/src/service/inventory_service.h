#pragma once
#include <string>

namespace repository { class InventoryRepository; }

namespace service {

class InventoryService {
    repository::InventoryRepository& repo_;
public:
    explicit InventoryService(repository::InventoryRepository& repo);
    bool reserve(const std::string& item_id, int quantity);
};

} // namespace service
