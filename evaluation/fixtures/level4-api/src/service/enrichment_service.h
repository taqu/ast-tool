#pragma once
#include <string>

namespace service {

class EnrichmentService {
public:
    std::string enrich(const std::string& input);
};

} // namespace service
