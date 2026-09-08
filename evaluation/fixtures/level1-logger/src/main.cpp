#include "core/logger.h"
#include "net/server.h"

int main() {
    net::Logger logger;
    net::Server server(logger, "localhost:8080");

    server.handleRequest("GET /index");
    server.handleRequest("");
    server.handleRequest("POST /data");

    return 0;
}
