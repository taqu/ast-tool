#pragma once
#include <string>

namespace model {

class LogMessage {
public:
    LogMessage(const std::string& message) : message_(message) {}
    virtual ~LogMessage() = default;
    virtual std::string get_text() const { return message_; }

private:
    std::string message_;
};

} // namespace model
