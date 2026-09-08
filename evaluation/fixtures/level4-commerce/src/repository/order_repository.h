#pragma once
#include <string>

namespace repository {

class OrderRepository {
public:
    bool save(const std::string& user_id, const std::string& item_id, int quantity);
    bool update(const std::string& order_id, const std::string& status);
    std::string find(const std::string& order_id);
};

} // namespace repository
