#include "request_processor.h"
#include <iostream>

namespace processor {

RequestProcessor::RequestProcessor(service::ValidationService& validator,
                                   service::EnrichmentService& enricher,
                                   store::DatabaseStore& store)
    : validator_(validator), enricher_(enricher), store_(store) {}

bool RequestProcessor::process(const std::string& request_id, const std::string& payload) {
    std::cout << "processor: processing request=" << request_id << "\n";
    if (!validator_.validate(payload)) {
        return false;
    }
    std::string enriched = enricher_.enrich(payload);
    return store_.save(request_id, enriched);
}

} // namespace processor
