#include "scheduler.h"
#include "../job/sync_job.h"
#include "../job/cleanup_job.h"
#include "../job/report_job.h"
#include <iostream>

namespace scheduler {

Scheduler::Scheduler(job::SyncJob& sync, job::CleanupJob& cleanup, job::ReportJob& report)
    : sync_(sync), cleanup_(cleanup), report_(report) {}

void Scheduler::run() {
    std::cout << "scheduler: running\n";
    sync_.execute();
    cleanup_.execute();
    report_.generate();
}

} // namespace scheduler
