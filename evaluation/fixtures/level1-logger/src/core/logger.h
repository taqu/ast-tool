#pragma once
#include <string>

namespace net {

class Logger {
public:
    void write(const std::string& msg);
};

} // namespace net
