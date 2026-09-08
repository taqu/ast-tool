#pragma once
#include <string>

namespace metrics {

class MetricsEmitter {
public:
    void emit(const std::string& metric);
};

} // namespace metrics
