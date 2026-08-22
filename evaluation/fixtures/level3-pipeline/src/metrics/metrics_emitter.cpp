#include "metrics_emitter.h"
#include <iostream>

namespace metrics {

void MetricsEmitter::emit(const std::string& metric) {
    std::cout << "metrics: " << metric << "\n";
}

} // namespace metrics
