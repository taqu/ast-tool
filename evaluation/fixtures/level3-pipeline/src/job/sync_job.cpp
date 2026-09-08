#include "sync_job.h"
#include "../loader/data_loader.h"
#include "../transform/transformer.h"
#include "../validator/data_validator.h"
#include "../store/data_store.h"
#include "../metrics/metrics_emitter.h"
#include <iostream>

namespace job {

SyncJob::SyncJob(loader::DataLoader& loader,
                 transform::Transformer& transformer,
                 validator::DataValidator& validator,
                 store::DataStore& store,
                 metrics::MetricsEmitter& emitter)
    : loader_(loader), transformer_(transformer), validator_(validator),
      store_(store), emitter_(emitter) {}

void SyncJob::execute() {
    std::cout << "sync: executing\n";
    std::string data = loader_.load();
    data = transformer_.transform(data);
    if (!validator_.validate(data)) return;
    store_.save(data);
    emitter_.emit("sync.completed");
}

} // namespace job
