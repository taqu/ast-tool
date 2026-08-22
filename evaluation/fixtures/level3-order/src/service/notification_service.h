#pragma once
#include <string>

namespace service {

class NotificationService {
public:
    void notify(const std::string& user_id, const std::string& message);
};

} // namespace service
