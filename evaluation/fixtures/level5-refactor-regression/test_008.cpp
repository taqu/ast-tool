#include <cassert>
#include <iostream>
#include "service/user_service.h"
#include "service/notification_service.h"
#include "workflow/password_reset_workflow.h"

int main() {
    service::NotificationService notification_service;
    service::UserService user_service(notification_service);
    workflow::PasswordResetWorkflow workflow(user_service, notification_service);

    // Scenario 1: Standard profile password change must still send exactly 1 email
    user_service.change_password("usr_1", "new_pass_1");
    assert(notification_service.sent_emails().size() == 1);
    assert(notification_service.sent_emails()[0].user_id == "usr_1");

    // Clear emails list
    notification_service = service::NotificationService();
    service::UserService user_service2(notification_service);
    workflow::PasswordResetWorkflow workflow2(user_service2, notification_service);

    // Scenario 2: Password reset workflow must send exactly 1 email
    workflow2.execute("usr_2", "new_pass_2");

    if (notification_service.sent_emails().size() != 1) {
        std::cerr << "FAIL: Password reset workflow sent " << notification_service.sent_emails().size() << " emails instead of 1" << std::endl;
        return 1;
    }

    std::cout << "SUCCESS" << std::endl;
    return 0;
}
