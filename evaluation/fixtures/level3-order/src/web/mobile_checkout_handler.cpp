#include "mobile_checkout_handler.h"
#include "../service/checkout_service.h"
#include <iostream>

namespace web {

MobileCheckoutHandler::MobileCheckoutHandler(service::CheckoutService& checkout)
    : checkout_(checkout) {}

bool MobileCheckoutHandler::handle(const std::string& user_id, const std::string& item_id) {
    std::cout << "mobile: checkout for " << user_id << "\n";
    return checkout_.process(user_id, item_id);
}

} // namespace web
