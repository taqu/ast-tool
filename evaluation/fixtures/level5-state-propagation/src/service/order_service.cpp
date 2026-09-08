#include "order_service.h"

namespace service {

bool OrderService::submit_order(const std::string& order_id, const std::string& user_id, const std::string& item_id, int quantity, const model::RequestContext& context) {
    if (!inventory_.reserve(item_id, quantity, context)) {
        return false;
    }

    // BUG: context is lost/ignored when saving order to the database.
    // An empty RequestContext is passed instead of the provided context.
    model::RequestContext empty_context;
    db_.save_order(order_id, user_id, empty_context);
    return true;
}

} // namespace service
