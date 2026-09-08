#include "config/config_manager.h"
#include "app/application.h"

int main() {
    cfg::ConfigManager config;
    app::Application app(config);
    if (!app.init()) return 1;
    app.run();
    return 0;
}
