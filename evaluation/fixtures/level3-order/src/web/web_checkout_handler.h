#pragma once
#include <string>

namespace service { class CheckoutService; }

namespace web {

class WebCheckoutHandler {
    service::CheckoutService& checkout_;
public:
    explicit WebCheckoutHandler(service::CheckoutService& checkout);
    bool handle(const std::string& user_id, const std::string& item_id);
};

} // namespace web
