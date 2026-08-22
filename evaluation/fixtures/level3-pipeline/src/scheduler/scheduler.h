#pragma once

namespace job { class SyncJob; class CleanupJob; class ReportJob; }

namespace scheduler {

class Scheduler {
    job::SyncJob& sync_;
    job::CleanupJob& cleanup_;
    job::ReportJob& report_;
public:
    Scheduler(job::SyncJob& sync, job::CleanupJob& cleanup, job::ReportJob& report);
    void run();
};

} // namespace scheduler
