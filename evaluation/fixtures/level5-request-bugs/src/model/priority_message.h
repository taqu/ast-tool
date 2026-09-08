#pragma once
#include "message.h"

class PriorityMessage : public Message {
public:
    PriorityMessage(const std::string& id, const std::string& payload, MessagePriority priority, double sla_deadline_seconds)
        : Message(id, payload, priority), sla_deadline_seconds_(sla_deadline_seconds) {}

    double sla_deadline_seconds() const { return sla_deadline_seconds_; }

private:
    double sla_deadline_seconds_;
};
