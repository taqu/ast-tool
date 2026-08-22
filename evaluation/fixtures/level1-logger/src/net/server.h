#pragma once
#include <string>

namespace net {

class Logger;
class Connection;

class Server {
    Logger& logger_;
    std::string address_;
public:
    Server(Logger& logger, const std::string& address);
    bool handleRequest(const std::string& req);
    void log(const std::string& msg);
};

} // namespace net
