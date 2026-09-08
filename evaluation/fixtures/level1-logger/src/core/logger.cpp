#include "logger.h"
#include <iostream>

namespace net {

void Logger::write(const std::string& msg) {
    std::cout << "[log] " << msg << "\n";
}

} // namespace net
