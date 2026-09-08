#pragma once
#include <string>
#include "service/validation_service.h"
#include "service/enrichment_service.h"
#include "store/database_store.h"

namespace processor {

class RequestProcessor {
public:
    RequestProcessor(service::ValidationService& validator,
                     service::EnrichmentService& enricher,
                     store::DatabaseStore& store);
    bool process(const std::string& request_id, const std::string& payload);

private:
    service::ValidationService& validator_;
    service::EnrichmentService& enricher_;
    store::DatabaseStore& store_;
};

} // namespace processor
