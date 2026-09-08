#include "sync_processor.h"
#include <iostream>

namespace processor {

SyncProcessor::SyncProcessor(store::DatabaseStore& store) : store_(store) {}

bool SyncProcessor::process(const std::string& sync_id, const std::string& payload) {
    std::cout << "sync-processor: processing sync=" << sync_id << "\n";
    return store_.save(sync_id, payload);
}

} // namespace processor
