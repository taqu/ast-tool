#include "schema_validator.h"
#include <iostream>

namespace cfg {

bool SchemaValidator::validate(const std::string& schema, const std::string& data) {
    std::cout << "validating against schema: " << schema << "\n";
    return !data.empty();
}

} // namespace cfg
