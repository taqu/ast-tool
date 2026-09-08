#include "inventory_client.h"

namespace service {

bool InventoryClient::reserve(const std::string& item_id, int quantity, const model::RequestContext& context) {
    last_correlation_id_ = context.correlation_id;
    return true;
}

} // namespace service
