#include "connection.h"
#include "../core/logger.h"
#include <iostream>

namespace net {

Connection::Connection(Logger& logger, const std::string& host)
    : logger_(logger), host_(host) {}

bool Connection::send(const std::string& data) {
    log("sending: " + data);
    std::cout << "conn[" << host_ << "] >> " << data << "\n";
    return true;
}

void Connection::log(const std::string& msg) {
    logger_.write("[conn] " + msg);
}

} // namespace net
