#include "message_dispatcher.h"
#include "model/priority_message.h"
#include <iostream>

namespace dispatcher {

MessageDispatcher::MessageDispatcher(queue::QueueService& standard_queue,
                                     queue::QueueService& priority_queue,
                                     queue::QueueService& dlq)
    : standard_queue_(standard_queue), priority_queue_(priority_queue), dlq_(dlq) {}

void MessageDispatcher::route(const Message& message) {
    if (message.payload().empty()) {
        dlq_.enqueue(message);
        return;
    }
    // BUG: Standard queue gets everything because route(message) resolves statically.
    // If the message priority is HIGH or CRITICAL, it should route to the priority queue instead.
    standard_queue_.enqueue(message);
}

void MessageDispatcher::route(const PriorityMessage& message) {
    if (message.payload().empty()) {
        dlq_.enqueue(message);
        return;
    }
    priority_queue_.enqueue(message);
}

} // namespace dispatcher
