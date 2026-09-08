#include "application.h"
#include "../config/config_manager.h"
#include <iostream>

namespace app {

Application::Application(cfg::ConfigManager& config) : config_(config) {}

bool Application::init() {
    config_.load("app.conf");
    config_.validate();
    std::string host = config_.get("host");
    std::string port = config_.get("port");
    std::cout << "starting on " << host << ":" << port << "\n";
    return true;
}

void Application::run() {
    std::cout << "running\n";
}

} // namespace app
