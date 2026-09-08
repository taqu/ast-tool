#include "cron_job.h"
#include <iostream>

namespace jobs {

CronJob::CronJob(processor::SyncProcessor& processor) : processor_(processor) {}

void CronJob::runJob(const std::string& data) {
    std::cout << "job: running background job\n";
    processor_.process("job_sync", data);
}

bool CronJob::validate(const std::string& job_id) {
    std::cout << "job: validating job_id=" << job_id << "\n";
    return !job_id.empty();
}

} // namespace jobs
