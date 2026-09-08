#include "admin_repository.h"
#include <iostream>

namespace repository {

bool AdminRepository::bulkSave(const std::string& batch_id, int count) {
    std::cout << "admin-repo: bulk saving batch=" << batch_id << " count=" << count << "\n";
    return true;
}

bool AdminRepository::adminUpdate(const std::string& order_id, const std::string& reason) {
    std::cout << "admin-repo: updating order=" << order_id << " reason=" << reason << "\n";
    return true;
}

std::string AdminRepository::adminFind(const std::string& order_id) {
    return order_id + "-admin-data";
}

} // namespace repository
