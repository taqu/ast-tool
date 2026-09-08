#pragma once
#include <string>

namespace net {

class Logger;

class Connection {
    Logger& logger_;
    std::string host_;
public:
    Connection(Logger& logger, const std::string& host);
    bool send(const std::string& data);
    void log(const std::string& msg);
};

} // namespace net
