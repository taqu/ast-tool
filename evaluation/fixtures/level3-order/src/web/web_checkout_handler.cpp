#include "web_checkout_handler.h"
#include "../service/checkout_service.h"
#include <iostream>

namespace web {

WebCheckoutHandler::WebCheckoutHandler(service::CheckoutService& checkout)
    : checkout_(checkout) {}

bool WebCheckoutHandler::handle(const std::string& user_id, const std::string& item_id) {
    std::cout << "web: checkout for " << user_id << "\n";
    return checkout_.process(user_id, item_id);
}

} // namespace web
