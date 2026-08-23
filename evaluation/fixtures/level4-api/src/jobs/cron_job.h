#pragma once
#include <string>
#include "processor/sync_processor.h"

namespace jobs {

class CronJob {
public:
    explicit CronJob(processor::SyncProcessor& processor);
    void runJob(const std::string& data);
    bool validate(const std::string& job_id); // for selective refactoring

private:
    processor::SyncProcessor& processor_;
};

} // namespace jobs
