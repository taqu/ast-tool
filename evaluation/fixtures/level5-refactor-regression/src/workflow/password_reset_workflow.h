#pragma once
#include <string>
#include "service/user_service.h"
#include "service/notification_service.h"

namespace workflow {

class PasswordResetWorkflow {
public:
    PasswordResetWorkflow(service::UserService& user_service, service::NotificationService& notification_service)
        : user_service_(user_service), notification_service_(notification_service) {}

    void execute(const std::string& user_id, const std::string& new_password);

private:
    service::UserService& user_service_;
    service::NotificationService& notification_service_;
};

} // namespace workflow
