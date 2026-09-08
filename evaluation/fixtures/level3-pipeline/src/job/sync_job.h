#pragma once

namespace store { class DataStore; }
namespace loader { class DataLoader; }
namespace transform { class Transformer; }
namespace validator { class DataValidator; }
namespace metrics { class MetricsEmitter; }

namespace job {

class SyncJob {
    loader::DataLoader& loader_;
    transform::Transformer& transformer_;
    validator::DataValidator& validator_;
    store::DataStore& store_;
    metrics::MetricsEmitter& emitter_;
public:
    SyncJob(loader::DataLoader& loader,
            transform::Transformer& transformer,
            validator::DataValidator& validator,
            store::DataStore& store,
            metrics::MetricsEmitter& emitter);
    void execute();
};

} // namespace job
