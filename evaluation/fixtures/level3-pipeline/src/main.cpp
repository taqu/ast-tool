#include "audit/audit_logger.h"
#include "job/cleanup_job.h"
#include "job/report_job.h"
#include "job/sync_job.h"
#include "loader/data_loader.h"
#include "metrics/metrics_emitter.h"
#include "scheduler/scheduler.h"
#include "store/data_store.h"
#include "transform/transformer.h"
#include "validator/data_validator.h"
#include "writer/report_writer.h"

int main() {
    audit::AuditLogger auditor;
    store::DataStore store(auditor);
    metrics::MetricsEmitter emitter;
    loader::DataLoader loader;
    transform::Transformer transformer;
    validator::DataValidator validator;
    writer::ReportWriter writer;

    job::SyncJob sync(loader, transformer, validator, store, emitter);
    job::CleanupJob cleanup(store, emitter);
    job::ReportJob report(store, writer);

    scheduler::Scheduler sched(sync, cleanup, report);
    sched.run();

    return 0;
}
