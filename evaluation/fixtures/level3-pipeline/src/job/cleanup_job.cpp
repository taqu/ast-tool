#include "cleanup_job.h"
#include "../store/data_store.h"
#include "../metrics/metrics_emitter.h"
#include <iostream>

namespace job {

CleanupJob::CleanupJob(store::DataStore& store, metrics::MetricsEmitter& emitter)
    : store_(store), emitter_(emitter) {}

void CleanupJob::execute() {
    std::cout << "cleanup: executing\n";
    store_.purge();
    emitter_.emit("cleanup.completed");
}

} // namespace job
