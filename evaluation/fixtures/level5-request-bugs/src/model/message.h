#pragma once
#include <string>

enum class MessagePriority {
    LOW,
    NORMAL,
    HIGH,
    CRITICAL
};

class Message {
public:
    Message(const std::string& id, const std::string& payload, MessagePriority priority)
        : id_(id), payload_(payload), priority_(priority) {}
    virtual ~Message() = default;

    std::string id() const { return id_; }
    std::string payload() const { return payload_; }
    MessagePriority priority() const { return priority_; }

private:
    std::string id_;
    std::string payload_;
    MessagePriority priority_;
};
