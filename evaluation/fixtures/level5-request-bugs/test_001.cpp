#include <cassert>
#include <iostream>
#include <vector>
#include "dispatcher/message_dispatcher.h"
#include "model/message.h"
#include "model/priority_message.h"
#include "queue/queue_service.h"

int main() {
    queue::StandardQueue sq;
    queue::PriorityQueue pq;
    queue::DeadLetterQueue dlq;
    dispatcher::MessageDispatcher dispatcher(sq, pq, dlq);

    Message low("1", "low payload", MessagePriority::LOW);
    PriorityMessage high("2", "high payload", MessagePriority::HIGH, 5.0);
    Message empty("3", "", MessagePriority::NORMAL);

    std::vector<const Message*> messages = { &low, &high, &empty };

    for (const auto* msg : messages) {
        dispatcher.route(*msg);
    }

    // Verify
    assert(sq.size() == 1);
    assert(sq.message_ids()[0] == "1");

    if (pq.size() != 1) {
        std::cerr << "FAIL: Priority message did not land in priority queue" << std::endl;
        return 1;
    }
    assert(pq.message_ids()[0] == "2");

    assert(dlq.size() == 1);
    assert(dlq.message_ids()[0] == "3");

    std::cout << "SUCCESS" << std::endl;
    return 0;
}
