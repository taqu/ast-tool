#pragma once
#include <string>

namespace cfg {

class SchemaValidator {
public:
    bool validate(const std::string& schema, const std::string& data);
};

} // namespace cfg
