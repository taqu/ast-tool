#pragma once

namespace store { class DataStore; }
namespace metrics { class MetricsEmitter; }

namespace job {

class CleanupJob {
    store::DataStore& store_;
    metrics::MetricsEmitter& emitter_;
public:
    CleanupJob(store::DataStore& store, metrics::MetricsEmitter& emitter);
    void execute();
};

} // namespace job
