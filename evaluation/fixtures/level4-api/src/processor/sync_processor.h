#pragma once
#include <string>
#include "store/database_store.h"

namespace processor {

class SyncProcessor {
public:
    explicit SyncProcessor(store::DatabaseStore& store);
    bool process(const std::string& sync_id, const std::string& payload);

private:
    store::DatabaseStore& store_;
};

} // namespace processor
