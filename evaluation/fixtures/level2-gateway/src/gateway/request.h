#pragma once
#include <map>
#include <string>

namespace gw {

struct Request {
    std::string method;
    std::string path;
    std::map<std::string, std::string> headers;
    std::string body;
};

} // namespace gw
