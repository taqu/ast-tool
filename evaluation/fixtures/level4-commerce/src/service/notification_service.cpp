#include "notification_service.h"
#include <iostream>

namespace service {

void NotificationService::notify(const std::string& user_id, const std::string& message) {
    std::cout << "notify: user=" << user_id << " msg=" << message << "\n";
}

void NotificationService::notifyAdmin(const std::string& admin_id,
                                       const std::string& message) {
    std::cout << "notify-admin: admin=" << admin_id << " msg=" << message << "\n";
}

} // namespace service
