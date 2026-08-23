#include <iostream>
#include "api/api_handler.h"
#include "api/callback_registry.h"
#include "jobs/cron_job.h"
#include "processor/request_processor.h"
#include "processor/sync_processor.h"
#include "service/enrichment_service.h"
#include "service/validation_service.h"
#include "store/database_store.h"

int main() {
    store::DatabaseStore store;
    service::ValidationService validator;
    service::EnrichmentService enricher;
    processor::RequestProcessor processor(validator, enricher, store);
    processor::SyncProcessor sync_processor(store);
    api::CallbackRegistry registry;

    // Register a callback
    registry.registerCallback([](const std::string& id, const std::string& payload) {
        std::cout << "Callback invoked for " << id << " with payload " << payload << "\n";
    });

    api::ApiHandler handler(processor, registry);
    handler.handleRequest("hello");

    jobs::CronJob job(sync_processor);
    job.runJob("background_data");

    return 0;
}
