#pragma once
#include <map>
#include <string>

namespace cfg {

class ConfigManager {
    std::map<std::string, std::string> data_;
public:
    bool load(const std::string& path);
    bool validate();
    std::string get(const std::string& key) const;
};

} // namespace cfg
