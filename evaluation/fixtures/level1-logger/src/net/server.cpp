#include "server.h"
#include "connection.h"
#include "../core/logger.h"

namespace net {

Server::Server(Logger& logger, const std::string& address)
    : logger_(logger), address_(address) {}

bool Server::handleRequest(const std::string& req) {
    if (req.empty()) {
        Connection err(logger_, address_);
        err.send("error: empty request");
        return false;
    }
    Connection conn(logger_, address_);
    conn.send("ok: " + req);
    return true;
}

void Server::log(const std::string& msg) {
    logger_.write("[server] " + msg);
}

} // namespace net
