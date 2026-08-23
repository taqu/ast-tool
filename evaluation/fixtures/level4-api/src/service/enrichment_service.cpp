#include "enrichment_service.h"
#include <iostream>

namespace service {

std::string EnrichmentService::enrich(const std::string& input) {
    return input + " [enriched]";
}

} // namespace service
