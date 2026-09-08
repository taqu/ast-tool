#include "config_manager.h"
#include <iostream>

namespace cfg {

bool ConfigManager::load(const std::string& path) {
    std::cout << "loading config from " << path << "\n";
    data_["host"] = "localhost";
    data_["port"] = "8080";
    return true;
}

bool ConfigManager::validate() {
    return data_.count("host") > 0 && data_.count("port") > 0;
}

std::string ConfigManager::get(const std::string& key) const {
    auto it = data_.find(key);
    if (it == data_.end()) return "";
    return it->second;
}

} // namespace cfg
