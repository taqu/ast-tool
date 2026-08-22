#pragma once
#include <string>

namespace repository {

class OrderRepository {
public:
    bool save(const std::string& user_id,
              const std::string& item_id,
              int quantity);
};

} // namespace repository
