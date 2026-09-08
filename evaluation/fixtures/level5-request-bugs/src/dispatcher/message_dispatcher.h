#pragma once
#include "model/message.h"
#include "queue/queue_service.h"

class PriorityMessage;

namespace dispatcher {

class MessageDispatcher {
public:
    MessageDispatcher(queue::QueueService& standard_queue,
                      queue::QueueService& priority_queue,
                      queue::QueueService& dlq);

    void route(const Message& message);
    void route(const PriorityMessage& message);

private:
    queue::QueueService& standard_queue_;
    queue::QueueService& priority_queue_;
    queue::QueueService& dlq_;
};

} // namespace dispatcher
