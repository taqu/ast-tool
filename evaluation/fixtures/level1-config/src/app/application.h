#pragma once

namespace cfg {
class ConfigManager;
}

namespace app {

class Application {
    cfg::ConfigManager& config_;
public:
    explicit Application(cfg::ConfigManager& config);
    bool init();
    void run();
};

} // namespace app
