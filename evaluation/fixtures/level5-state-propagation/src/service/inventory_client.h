#pragma once
#include <string>
#include "model/context.h"

namespace service {

class InventoryClient {
public:
    bool reserve(const std::string& item_id, int quantity, const model::RequestContext& context);
    std::string last_correlation_id() const { return last_correlation_id_; }

private:
    std::string last_correlation_id_;
};

} // namespace service
