#include "token_cache.h"

namespace auth {

std::string TokenCache::get(const std::string& key) {
    auto it = store_.find(key);
    return it == store_.end() ? "" : it->second;
}

bool TokenCache::set(const std::string& key, const std::string& value) {
    store_[key] = value;
    return true;
}

bool TokenCache::update(const std::string& key, const std::string& value) {
    store_[key] = value;
    return true;
}

bool TokenCache::invalidate(const std::string& key) {
    return store_.erase(key) > 0;
}

} // namespace auth
