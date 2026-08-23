#pragma once
#include <string>
#include "notification_service.h"

namespace service {

class UserService {
public:
    explicit UserService(NotificationService& notification_service)
        : notification_service_(notification_service) {}

    void change_password(const std::string& user_id, const std::string& new_password);

private:
    NotificationService& notification_service_;
};

} // namespace service
