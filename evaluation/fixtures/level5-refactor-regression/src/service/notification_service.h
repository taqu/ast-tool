#pragma once
#include <string>
#include <vector>

namespace service {

class NotificationService {
public:
    void send_email(const std::string& user_id, const std::string& subject, const std::string& body);

    struct EmailRecord {
        std::string user_id;
        std::string subject;
        std::string body;
    };

    const std::vector<EmailRecord>& sent_emails() const { return sent_emails_; }

private:
    std::vector<EmailRecord> sent_emails_;
};

} // namespace service
