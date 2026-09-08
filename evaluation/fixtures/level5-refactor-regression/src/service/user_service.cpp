#include "user_service.h"

namespace service {

void UserService::change_password(const std::string& user_id, const std::string& new_password) {
    notification_service_.send_email(user_id, "Password Changed", "Your password has been successfully changed.");
}

} // namespace service
