#pragma once
#include <string>

namespace service { class CheckoutService; }

namespace web {

class MobileCheckoutHandler {
    service::CheckoutService& checkout_;
public:
    explicit MobileCheckoutHandler(service::CheckoutService& checkout);
    bool handle(const std::string& user_id, const std::string& item_id);
};

} // namespace web
