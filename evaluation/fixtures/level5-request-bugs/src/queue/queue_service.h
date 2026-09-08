#pragma once
#include <string>
#include <vector>
#include "model/message.h"

namespace queue {

class QueueService {
public:
    virtual ~QueueService() = default;
    virtual void enqueue(const Message& message) = 0;
    virtual std::string name() const = 0;
    virtual size_t size() const = 0;
    virtual const std::vector<std::string>& message_ids() const = 0;
};

class BaseQueue : public QueueService {
public:
    explicit BaseQueue(const std::string& name) : name_(name) {}
    void enqueue(const Message& message) override {
        ids_.push_back(message.id());
    }
    std::string name() const override { return name_; }
    size_t size() const override { return ids_.size(); }
    const std::vector<std::string>& message_ids() const override { return ids_; }

private:
    std::string name_;
    std::vector<std::string> ids_;
};

class StandardQueue : public BaseQueue {
public:
    StandardQueue() : BaseQueue("StandardQueue") {}
};

class PriorityQueue : public BaseQueue {
public:
    PriorityQueue() : BaseQueue("PriorityQueue") {}
};

class DeadLetterQueue : public BaseQueue {
public:
    DeadLetterQueue() : BaseQueue("DeadLetterQueue") {}
};

} // namespace queue
