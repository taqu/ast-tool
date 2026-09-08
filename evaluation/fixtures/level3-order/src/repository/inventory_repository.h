#pragma once
#include <string>

namespace repository {

class InventoryRepository {
public:
    bool reserve(const std::string& item_id, int quantity);
};

} // namespace repository
