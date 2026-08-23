#pragma once
#include <string>
#include <functional>
#include <vector>

namespace api {

using RequestHandlerCallback = std::function<void(const std::string&, const std::string&)>;

class CallbackRegistry {
public:
    void registerCallback(RequestHandlerCallback callback);
    void trigger(const std::string& request_id, const std::string& payload);

private:
    std::vector<RequestHandlerCallback> callbacks_;
};

} // namespace api
