#pragma once
#include <map>
#include <string>

namespace auth {

class TokenCache {
    std::map<std::string, std::string> store_;
public:
    std::string get(const std::string& key);
    bool set(const std::string& key, const std::string& value);
    bool update(const std::string& key, const std::string& value);
    bool invalidate(const std::string& key);
};

} // namespace auth
