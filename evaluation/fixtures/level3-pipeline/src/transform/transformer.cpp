#include "transformer.h"
#include <iostream>

namespace transform {

std::string Transformer::transform(const std::string& data) {
    std::cout << "transform: transforming\n";
    return data + "-transformed";
}

} // namespace transform
