#pragma once
#include <string>
#include <vector>

namespace service {

class InventoryService {
public:
    bool reserve(const std::string& item_id, int quantity) {
        reserved_.push_back(std::make_pair(item_id, quantity));
        return true;
    }

    void release(const std::string& item_id, int quantity) {
        released_.push_back(std::make_pair(item_id, quantity));
    }

    std::vector<std::pair<std::string, int>> reserved_;
    std::vector<std::pair<std::string, int>> released_;
};

} // namespace service
