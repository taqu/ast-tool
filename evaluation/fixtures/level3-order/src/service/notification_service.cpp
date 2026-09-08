#include "notification_service.h"
#include <iostream>

namespace service {

void NotificationService::notify(const std::string& user_id, const std::string& message) {
    std::cout << "notify: " << user_id << ": " << message << "\n";
}

} // namespace service
