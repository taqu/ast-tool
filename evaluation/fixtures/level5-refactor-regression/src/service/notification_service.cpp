#include "notification_service.h"

namespace service {

void NotificationService::send_email(const std::string& user_id, const std::string& subject, const std::string& body) {
    sent_emails_.push_back(EmailRecord{user_id, subject, body});
}

} // namespace service
