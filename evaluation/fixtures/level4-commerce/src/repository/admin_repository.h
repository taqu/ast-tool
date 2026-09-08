#pragma once
#include <string>

namespace repository {

class AdminRepository {
public:
    bool bulkSave(const std::string& batch_id, int count);
    bool adminUpdate(const std::string& order_id, const std::string& reason);
    std::string adminFind(const std::string& order_id);
};

} // namespace repository
