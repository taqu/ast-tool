#pragma once
#include <string>

namespace repository {

class InventoryRepository {
public:
    bool reserve(const std::string& item_id, int quantity);
    bool release(const std::string& item_id, int quantity);
    int available(const std::string& item_id);
};

} // namespace repository
