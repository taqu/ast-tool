#include "password_reset_workflow.h"

namespace workflow {

void PasswordResetWorkflow::execute(const std::string& user_id, const std::string& new_password) {
    user_service_.change_password(user_id, new_password);

    // BUG: Duplicate email notification is sent here.
    // user_service_.change_password(user_id, new_password) already sends the notification.
    notification_service_.send_email(user_id, "Password Changed", "Your password has been successfully changed.");
}

} // namespace workflow
