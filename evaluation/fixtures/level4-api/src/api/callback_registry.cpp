#include "callback_registry.h"
#include <iostream>

namespace api {

void CallbackRegistry::registerCallback(RequestHandlerCallback callback) {
    callbacks_.push_back(callback);
}

void CallbackRegistry::trigger(const std::string& request_id, const std::string& payload) {
    std::cout << "registry: triggering callbacks for " << request_id << "\n";
    for (auto& cb : callbacks_) {
        cb(request_id, payload);
    }
}

} // namespace api
